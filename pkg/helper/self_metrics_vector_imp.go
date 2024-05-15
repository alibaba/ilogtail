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
	"fmt"
	"sync"
	"sync/atomic"
	"unsafe"

	"github.com/alibaba/ilogtail/pkg/pipeline"
)

const (
	defaultTagValue = "-"
)

var (
	NewCounterMetricVector = func(metricName string, constLabels map[string]string, labelNames []string) CounterMetricVector {
		return NewMetricVector[pipeline.Counter](metricName, pipeline.CounterType, constLabels, labelNames)
	}

	NewAverageMetricVector = func(metricName string, constLabels map[string]string, labelNames []string) AverageMetricVector {
		return NewMetricVector[pipeline.Average](metricName, pipeline.AverageType, constLabels, labelNames)
	}

	NewGaugeMetricVector = func(metricName string, constLabels map[string]string, labelNames []string) GaugeMetricVector {
		return NewMetricVector[pipeline.Gauge](metricName, pipeline.GaugeType, constLabels, labelNames)
	}

	NewLatencyMetricVector = func(metricName string, constLabels map[string]string, labelNames []string) LatencyMetricVector {
		return NewMetricVector[pipeline.Latency](metricName, pipeline.LatencyType, constLabels, labelNames)
	}

	NewStringMetricVector = func(metricName string, constLabels map[string]string, labelNames []string) StringMetricVector {
		return NewMetricVector[pipeline.StrMetric](metricName, pipeline.StringType, constLabels, labelNames)
	}
)

func NewCounterMetricVectorAndRegister(mr *pipeline.MetricsRecord, metricName string, constLabels map[string]string, labelNames []string) CounterMetricVector {
	v := NewMetricVector[pipeline.Counter](metricName, pipeline.CounterType, constLabels, labelNames)
	mr.RegisterMetricVector(v)
	return v
}

func NewAverageMetricVectorAndRegister(mr *pipeline.MetricsRecord, metricName string, constLabels map[string]string, labelNames []string) AverageMetricVector {
	v := NewMetricVector[pipeline.Average](metricName, pipeline.AverageType, constLabels, labelNames)
	mr.RegisterMetricVector(v)
	return v
}

func NewGaugeMetricVectorAndRegister(mr *pipeline.MetricsRecord, metricName string, constLabels map[string]string, labelNames []string) GaugeMetricVector {
	v := NewMetricVector[pipeline.Gauge](metricName, pipeline.GaugeType, constLabels, labelNames)
	mr.RegisterMetricVector(v)
	return v
}

func NewLatencyMetricVectorAndRegister(mr *pipeline.MetricsRecord, metricName string, constLabels map[string]string, labelNames []string) LatencyMetricVector {
	v := NewMetricVector[pipeline.Latency](metricName, pipeline.LatencyType, constLabels, labelNames)
	mr.RegisterMetricVector(v)
	return v
}

func NewStringMetricVectorAndRegister(mr *pipeline.MetricsRecord, metricName string, constLabels map[string]string, labelNames []string) StringMetricVector {
	v := NewMetricVector[pipeline.StrMetric](metricName, pipeline.StringType, constLabels, labelNames)
	mr.RegisterMetricVector(v)
	return v
}

// MetricVector 是一个泛型接口，定义了所有 MetricVector 实现所需的 WithLabels 方法。
type MetricVector[T any] interface {
	WithLabels(labels ...pipeline.Label) T
	pipeline.MetricCollector
}

type (
	CounterMetricVector = MetricVector[pipeline.Counter]
	AverageMetricVector = MetricVector[pipeline.Average]
	GaugeMetricVector   = MetricVector[pipeline.Gauge]
	LatencyMetricVector = MetricVector[pipeline.Latency]
	StringMetricVector  = MetricVector[pipeline.StrMetric]
	Label               = pipeline.Label
)

type MetricMap interface {
	pipeline.MetricVector
	pipeline.MetricCollector
	pipeline.MetricSet
}

type MetricVectorImpl[T pipeline.Metric] struct {
	MetricMap
}

func NewMetricVector[T pipeline.Metric](metricName string, metricType pipeline.SelfMetricType, constLabels map[string]string, labelNames []string) MetricVector[T] {
	return &MetricVectorImpl[T]{
		MetricMap: newMetricVector(metricName, metricType, constLabels, labelNames),
	}
}

func (m *MetricVectorImpl[T]) WithLabels(labels ...pipeline.Label) T {
	return m.MetricMap.WithLabels(labels).(T)
}

var (
	_ pipeline.MetricCollector = (*metricVector)(nil)
)

type metricVector struct {
	name        string // metric name
	metricType  pipeline.SelfMetricType
	constLabels []pipeline.Label // constLabels is the labels that are not changed when the metric is created.
	labelKeys   []string         // labelNames is the names of the labels. The values of the labels can be changed.

	indexPool   GenericPool[string] // index is []string, which is sorted according to labelNames.
	bytesPool   GenericPool[byte]   // bytesPool is the bytes pool for the index.
	collector   sync.Map            // collector is a map[string]pipeline.Metric, key is the index of the metric.
	seriesCount int64               // the number of metrics in the vector
}

func newMetricVector(
	metricName string,
	metricType pipeline.SelfMetricType,
	constLabels map[string]string,
	labelNames []string,
) *metricVector {
	mv := &metricVector{
		name:       metricName,
		metricType: metricType,
		labelKeys:  labelNames,
		indexPool:  NewGenericPool(func() []string { return make([]string, 0, 10) }),
		bytesPool:  NewGenericPool(func() []byte { return make([]byte, 0, 128) }),
	}

	for k, v := range constLabels {
		mv.constLabels = append(mv.constLabels, pipeline.Label{Key: k, Value: v})
	}
	return mv
}

func (v *metricVector) Name() string {
	return v.name
}

func (v *metricVector) ConstLabels() []pipeline.Label {
	return v.constLabels
}

func (v *metricVector) LabelKeys() []string {
	return v.labelKeys
}

func (v *metricVector) WithLabels(labels []pipeline.Label) pipeline.Metric {
	index, err := v.buildIndex(labels)
	if err != nil {
		return NewErrorMetric(v.metricType, err)
	}

	buffer := v.bytesPool.Get()
	for _, tagValue := range *index {
		*buffer = append(*buffer, '|')
		*buffer = append(*buffer, tagValue...)
	}

	/* #nosec G103 */
	k := *(*string)(unsafe.Pointer(buffer))
	acV, loaded := v.collector.Load(k)
	if loaded {
		metric := acV.(pipeline.Metric)
		v.indexPool.Put(index)
		v.bytesPool.Put(buffer)
		return metric
	}

	newMetric := NewMetric(v.metricType, v, *index)
	acV, loaded = v.collector.LoadOrStore(k, newMetric)
	if !loaded {
		atomic.AddInt64(&v.seriesCount, 1)
	} else {
		v.bytesPool.Put(buffer)
	}
	return acV.(pipeline.Metric)
}

func (v *metricVector) Collect() []pipeline.Metric {
	res := make([]pipeline.Metric, 0, 10)
	v.collector.Range(func(key, value interface{}) bool {
		res = append(res, value.(pipeline.Metric))
		return true
	})
	return res
}

// buildIndex return the index
func (v *metricVector) buildIndex(labels []pipeline.Label) (*[]string, error) {
	if len(labels) > len(v.labelKeys) {
		return nil, fmt.Errorf("too many labels, expected %d, got %d. defined labels: %v",
			len(v.labelKeys), len(labels), v.labelKeys)
	}

	index := v.indexPool.Get()
	for range v.labelKeys {
		*index = append(*index, defaultTagValue)
	}

	for d, tag := range labels {
		if v.labelKeys[d] == tag.Key { // fast path
			(*index)[d] = tag.Value
		} else {
			err := v.slowConstructIndex(index, tag)
			if err != nil {
				v.indexPool.Put(index)
				return nil, err
			}
		}
	}

	return index, nil
}

func (v *metricVector) slowConstructIndex(index *[]string, tag pipeline.Label) error {
	for i, tagName := range v.labelKeys {
		if tagName == tag.Key {
			(*index)[i] = tag.Value
			return nil
		}
	}
	return fmt.Errorf("undefined label: %s in %v", tag.Key, v.labelKeys)
}
