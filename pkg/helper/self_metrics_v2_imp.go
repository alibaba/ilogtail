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
	"strconv"
	"sync"
	"sync/atomic"

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

var (
	_ pipeline.Counter      = (*counterImp)(nil)
	_ pipeline.Gauge        = (*gaugeImp)(nil)
	_ pipeline.Average      = (*averageImp)(nil)
	_ pipeline.Latency      = (*latencyImp)(nil)
	_ pipeline.StringMetric = (*strMetricImp)(nil)

	_ pipeline.Counter      = (*errorCounter)(nil)
	_ pipeline.Gauge        = (*errorGauge)(nil)
	_ pipeline.Average      = (*errorAverage)(nil)
	_ pipeline.Latency      = (*errorLatency)(nil)
	_ pipeline.StringMetric = (*errorStrMetric)(nil)
)

func NewMetric(metricType pipeline.SelfMetricType, metricSet pipeline.MetricSet, index []string) pipeline.Metric {
	switch metricType {
	case pipeline.CounterType:
		return NewCounter(metricSet, index)
	case pipeline.AverageType:
		return NewAverage(metricSet, index)
	case pipeline.GaugeType:
		return NewGauge(metricSet, index)
	case pipeline.StringType:
		return NewString(metricSet, index)
	case pipeline.LatencyType:
		return NewLatency(metricSet, index)
	}
	return NewErrorMetric(metricType, nil)
}

// ErrorMetrics always return error.
func NewErrorMetric(metricType pipeline.SelfMetricType, err error) pipeline.Metric {
	switch metricType {
	case pipeline.CounterType:
		return NewErrorCounter(err)
	case pipeline.AverageType:
		return NewErrorAverage(err)
	case pipeline.GaugeType:
		return NewErrorGauge(err)
	case pipeline.StringType:
		return NewErrorString(err)
	case pipeline.LatencyType:
		return NewErrorLatency(err)
	default:
		return NewErrorCounter(err)
	}
}

type counterImp struct {
	value int64
	prev  int64
	Series
}

func NewCounter(ms pipeline.MetricSet, index []string) pipeline.Counter {
	c := &counterImp{
		Series: newSeries(ms, index),
	}
	return c
}

func (c *counterImp) Add(delta int64) error {
	atomic.AddInt64(&c.value, delta)
	return nil
}

func (c *counterImp) Get() pipeline.MetricValue[float64] {
	value := atomic.LoadInt64(&c.value)
	atomic.StoreInt64(&c.prev, value)
	return pipeline.MetricValue[float64]{Name: c.Name(), Value: float64(value)}
}

func (c *counterImp) Clear() {
	atomic.StoreInt64(&c.value, 0)
	atomic.StoreInt64(&c.prev, 0)
}

func (c *counterImp) Serialize(log *protocol.Log) {
	metricValue := c.Get()
	c.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

type gaugeImp struct {
	value float64
	Series
}

func NewGauge(ms pipeline.MetricSet, index []string) pipeline.Gauge {
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
	g.Series.SerializeWithStr(log, metricValue.Name, strconv.FormatFloat(metricValue.Value, 'f', 4, 64))
}

type averageImp struct {
	sync.RWMutex
	value   int64
	count   int64
	prevAvg float64
	Series
}

func NewAverage(ms pipeline.MetricSet, index []string) pipeline.Counter {
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

type latencyImp struct {
	sync.RWMutex
	count      int64
	latencySum float64
	Series
}

func NewLatency(ms pipeline.MetricSet, index []string) pipeline.Latency {
	l := &latencyImp{
		Series: newSeries(ms, index),
	}
	return l
}

func (l *latencyImp) Observe(f float64) error {
	l.Lock()
	defer l.Unlock()
	l.count++
	l.latencySum += f
	return nil
}

func (l *latencyImp) Get() pipeline.MetricValue[float64] {
	l.RLock()
	defer l.RUnlock()
	if l.count == 0 {
		return pipeline.MetricValue[float64]{Name: l.Name(), Value: 0}
	}
	avg := l.latencySum / float64(l.count)
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

type strMetricImp struct {
	sync.RWMutex
	value string
	Series
}

func NewString(ms pipeline.MetricSet, index []string) pipeline.StringMetric {
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
type errorMetricBase struct {
	err error
}

func (e *errorMetricBase) Add(f int64) error {
	return e.err
}

func (e *errorMetricBase) Set(f float64) error {
	return e.err
}

func (e *errorMetricBase) Observe(f float64) error {
	return e.err
}

func (e *errorMetricBase) Serialize(log *protocol.Log) {}

func (e *errorMetricBase) Get() pipeline.MetricValue[float64] {
	return pipeline.MetricValue[float64]{Name: "", Value: 0}
}
func (e *errorMetricBase) Clear() {}

type errorCounter struct {
	errorMetricBase
}

func NewErrorCounter(err error) pipeline.Counter {
	return &errorCounter{errorMetricBase: errorMetricBase{err: err}}
}

type errorGauge struct {
	errorMetricBase
}

func NewErrorGauge(err error) pipeline.Gauge {
	return &errorGauge{errorMetricBase: errorMetricBase{err: err}}
}

type errorAverage struct {
	errorMetricBase
}

func NewErrorAverage(err error) pipeline.Average {
	return &errorAverage{errorMetricBase: errorMetricBase{err: err}}
}

type errorLatency struct {
	errorMetricBase
}

func NewErrorLatency(err error) pipeline.Latency {
	return &errorLatency{errorMetricBase: errorMetricBase{err: err}}
}

type errorStrMetric struct {
	errorMetricBase
}

func (e errorStrMetric) Set(s string) error {
	return e.err
}

func (e errorStrMetric) Get() pipeline.MetricValue[string] {
	return pipeline.MetricValue[string]{Name: "", Value: ""}
}

func NewErrorString(err error) pipeline.StringMetric {
	return &errorStrMetric{errorMetricBase: errorMetricBase{err: err}}
}
