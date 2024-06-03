// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package helper

import (
	"math"
	"strconv"
	"sync"
	"sync/atomic"
	"time"

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

var (
	_ pipeline.Counter      = (*cumulativeCounterImp)(nil)
	_ pipeline.Counter      = (*counterImp)(nil)
	_ pipeline.Counter      = (*averageImp)(nil)
	_ pipeline.Gauge        = (*gaugeImp)(nil)
	_ pipeline.Latency      = (*latencyImp)(nil)
	_ pipeline.StringMetric = (*strMetricImp)(nil)

	_ pipeline.Counter      = (*errorNumericMetric)(nil)
	_ pipeline.Gauge        = (*errorNumericMetric)(nil)
	_ pipeline.Latency      = (*errorNumericMetric)(nil)
	_ pipeline.StringMetric = (*errorStrMetric)(nil)
)

func newMetric(metricType pipeline.SelfMetricType, metricSet pipeline.MetricSet, index []string) pipeline.Metric {
	switch metricType {
	case pipeline.CumulativeCounterType:
		return newCumulativeCounter(metricSet, index)
	case pipeline.AverageType:
		return newAverage(metricSet, index)
	case pipeline.CounterType:
		return newDeltaCounter(metricSet, index)
	case pipeline.GaugeType:
		return newGauge(metricSet, index)
	case pipeline.StringType:
		return newStringMetric(metricSet, index)
	case pipeline.LatencyType:
		return newLatency(metricSet, index)
	}
	return newErrorMetric(metricType, nil)
}

// ErrorMetrics always return error.
func newErrorMetric(metricType pipeline.SelfMetricType, err error) pipeline.Metric {
	switch metricType {
	case pipeline.StringType:
		return newErrorStringMetric(err)
	default:
		return newErrorNumericMetric(err)
	}
}

// Deprecated: Use deltaImp instead.
// cumulativeCounterImp is a counter metric that can be incremented or decremented.
// It gets the cumulative value of the counter.
type cumulativeCounterImp struct {
	value int64
	Series
}

func newCumulativeCounter(ms pipeline.MetricSet, index []string) pipeline.Counter {
	c := &cumulativeCounterImp{
		Series: newSeries(ms, index),
	}
	return c
}

func (c *cumulativeCounterImp) Add(delta int64) error {
	atomic.AddInt64(&c.value, delta)
	return nil
}

func (c *cumulativeCounterImp) Get() pipeline.MetricValue[float64] {
	value := atomic.LoadInt64(&c.value)
	return pipeline.MetricValue[float64]{Name: c.Name(), Value: float64(value)}
}

func (c *cumulativeCounterImp) Clear() {
	atomic.StoreInt64(&c.value, 0)
}

func (c *cumulativeCounterImp) Serialize(log *protocol.Log) {
	metricValue := c.Get()
	c.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

// delta is a counter metric that can be incremented or decremented.
// It gets the increased value in the last window.
type counterImp struct {
	value int64
	Series
}

func newDeltaCounter(ms pipeline.MetricSet, index []string) pipeline.Counter {
	d := &counterImp{
		Series: newSeries(ms, index),
	}
	return d
}

func (d *counterImp) Add(delta int64) error {
	atomic.AddInt64(&d.value, delta)
	return nil
}

func (d *counterImp) Get() pipeline.MetricValue[float64] {
	value := atomic.SwapInt64(&d.value, 0)
	return pipeline.MetricValue[float64]{Name: d.Name(), Value: float64(value)}
}

func (d *counterImp) Clear() {
	atomic.StoreInt64(&d.value, 0)
}

func (d *counterImp) Serialize(log *protocol.Log) {
	metricValue := d.Get()
	d.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

// gauge is a metric that represents a single numerical value that can arbitrarily go up and down.
type gaugeImp struct {
	value float64
	Series
}

func newGauge(ms pipeline.MetricSet, index []string) pipeline.Gauge {
	g := &gaugeImp{
		Series: newSeries(ms, index),
	}
	return g
}

func (g *gaugeImp) Set(f float64) error {
	AtomicStoreFloat64(&g.value, f)
	return nil
}

func (g *gaugeImp) Get() pipeline.MetricValue[float64] {
	return pipeline.MetricValue[float64]{Name: g.Name(), Value: AtomicLoadFloat64(&g.value)}
}

func (g *gaugeImp) Clear() {
	AtomicStoreFloat64(&g.value, 0)
}

func (g *gaugeImp) Serialize(log *protocol.Log) {
	metricValue := g.Get()
	if math.IsNaN(metricValue.Value) {
		return
	}
	g.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

// averageImp is a metric to compute the average value of a series of values in the last window.
// if there is no value added in the last window, the previous average value will be returned.
type averageImp struct {
	sync.RWMutex
	value   int64
	count   int64
	prevAvg float64
	Series
}

func newAverage(ms pipeline.MetricSet, index []string) pipeline.Counter {
	a := &averageImp{
		Series: newSeries(ms, index),
	}
	return a
}

func (a *averageImp) Add(f int64) error {
	a.Lock()
	defer a.Unlock()
	a.value += f
	a.count++
	return nil
}

func (a *averageImp) Get() pipeline.MetricValue[float64] {
	a.RLock()
	defer a.RUnlock()
	if a.count == 0 {
		return pipeline.MetricValue[float64]{Name: a.Name(), Value: a.prevAvg}
	}
	avg := float64(a.value) / float64(a.count)
	a.prevAvg, a.value, a.count = avg, 0, 0
	return pipeline.MetricValue[float64]{Name: a.Name(), Value: avg}
}

func (a *averageImp) Clear() {
	a.Lock()
	a.value = 0
	a.count = 0
	a.prevAvg = 0
	a.Unlock()
}

func (a *averageImp) Serialize(log *protocol.Log) {
	metricValue := a.Get()
	a.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

// latencyImp is a metric to compute the average latency of a series of values in the last window.
type latencyImp struct {
	sync.Mutex
	count      int64
	latencySum float64
	Series
}

func newLatency(ms pipeline.MetricSet, index []string) pipeline.Latency {
	l := &latencyImp{
		Series: newSeries(ms, index),
	}
	return l
}

func (l *latencyImp) Record(d time.Duration) error {
	return l.Observe(float64(d))
}

func (l *latencyImp) Observe(f float64) error {
	l.Lock()
	defer l.Unlock()
	l.count++
	l.latencySum += f
	return nil
}

func (l *latencyImp) Get() pipeline.MetricValue[float64] {
	l.Lock()
	defer l.Unlock()
	if l.count == 0 {
		return pipeline.MetricValue[float64]{Name: l.Name(), Value: 0}
	}
	avg := l.latencySum / float64(l.count)
	l.count, l.latencySum = 0, 0
	return pipeline.MetricValue[float64]{Name: l.Name(), Value: avg}
}

func (l *latencyImp) Clear() {
	l.Lock()
	defer l.Unlock()
	l.count = 0
	l.latencySum = 0
}

func (l *latencyImp) Serialize(log *protocol.Log) {
	metricValue := l.Get()
	l.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value/1000, 'f', 4, 64))
}

// strMetricImp is a metric that represents a single string value.
type strMetricImp struct {
	sync.RWMutex
	value string
	Series
}

func newStringMetric(ms pipeline.MetricSet, index []string) pipeline.StringMetric {
	s := &strMetricImp{
		Series: newSeries(ms, index),
	}
	return s
}

func (s *strMetricImp) Set(str string) error {
	s.Lock()
	defer s.Unlock()
	s.value = str
	return nil
}

func (s *strMetricImp) Get() pipeline.MetricValue[string] {
	s.RLock()
	defer s.RUnlock()
	return pipeline.MetricValue[string]{Name: s.Name(), Value: s.value}
}

func (s *strMetricImp) Clear() {
	s.Lock()
	s.value = ""
	s.Unlock()
}

func (s *strMetricImp) Serialize(log *protocol.Log) {
	metricValue := s.Get()
	s.Series.SerializeWithStr(log, metricValue.Name, metricValue.Value)
}

type Series struct {
	pipeline.MetricSet
	index []string
}

func newSeries(ms pipeline.MetricSet, index []string) Series {
	var indexToStore []string
	if index != nil {
		indexToStore = make([]string, len(index))
		copy(indexToStore, index)
	}

	return Series{
		MetricSet: ms,
		index:     indexToStore,
	}
}

func (s Series) SerializeWithStr(log *protocol.Log, metricName, metricValueStr string) {
	log.Contents = append(log.Contents,
		&protocol.Log_Content{Key: metricName, Value: metricValueStr},
		&protocol.Log_Content{Key: SelfMetricNameKey, Value: metricName})

	for _, v := range s.ConstLabels() {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: v.Key, Value: v.Value})
	}

	labelNames := s.LabelKeys()
	for i, v := range s.index {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: labelNames[i], Value: v})
	}
}

/*
Following are the metrics returned when WithLabel encountered an error.
*/
type errorNumericMetric struct {
	err error
}

func (e *errorNumericMetric) Add(f int64) error {
	return e.err
}

func (e *errorNumericMetric) Set(f float64) error {
	return e.err
}

func (e *errorNumericMetric) Observe(f float64) error {
	return e.err
}

func (e *errorNumericMetric) Serialize(log *protocol.Log) {}

func (e *errorNumericMetric) Get() pipeline.MetricValue[float64] {
	return pipeline.MetricValue[float64]{Name: "", Value: 0}
}
func (e *errorNumericMetric) Clear() {}

func newErrorNumericMetric(err error) *errorNumericMetric {
	return &errorNumericMetric{err: err}
}

type errorStrMetric struct {
	errorNumericMetric
}

func (e errorStrMetric) Set(s string) error {
	return e.err
}

func (e errorStrMetric) Get() pipeline.MetricValue[string] {
	return pipeline.MetricValue[string]{Name: "", Value: ""}
}

func newErrorStringMetric(err error) pipeline.StringMetric {
	return &errorStrMetric{errorNumericMetric: errorNumericMetric{err: err}}
}
