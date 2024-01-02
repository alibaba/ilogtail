// Copyright 2021 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package helper

import (
	"sync"
	"sync/atomic"
	"time"

	"github.com/alibaba/ilogtail/pkg/pipeline"
)

var mu sync.Mutex

type StrMetric struct {
	name  string
	value string
}

func (s *StrMetric) Name() string {
	return s.name
}

func (s *StrMetric) Set(v string) {
	mu.Lock()
	s.value = v
	mu.Unlock()
}

func (s *StrMetric) Get() string {
	mu.Lock()
	v := s.value
	mu.Unlock()
	return v
}

type NormalMetric struct {
	name  string
	value int64
}

func (s *NormalMetric) Add(v int64) {
	atomic.AddInt64(&s.value, v)
}

func (s *NormalMetric) Clear(v int64) {
	atomic.StoreInt64(&s.value, v)
}

func (s *NormalMetric) Get() int64 {
	return atomic.LoadInt64(&s.value)
}

func (s *NormalMetric) Name() string {
	return s.name
}

type AvgMetric struct {
	name    string
	value   int64
	count   int64
	prevAvg float64
}

func (s *AvgMetric) Add(v int64) {
	mu.Lock()
	s.value += v
	s.count++
	mu.Unlock()
}

func (s *AvgMetric) Clear(v int64) {
	mu.Lock()
	s.value = 0
	s.count = 0
	s.prevAvg = 0.0
	mu.Unlock()
}

func (s *AvgMetric) Get() int64 {
	return int64(s.GetAvg())
}

func (s *AvgMetric) GetAvg() float64 {
	var avg float64
	mu.Lock()
	if s.count > 0 {
		s.prevAvg, avg = float64(s.value)/float64(s.count), float64(s.value)/float64(s.count)
		s.value = 0
		s.count = 0
	} else {
		avg = s.prevAvg
	}
	mu.Unlock()
	return avg
}

func (s *AvgMetric) Name() string {
	return s.name
}

type LatMetric struct {
	name       string
	t          time.Time
	count      int
	latencySum time.Duration
}

func (s *LatMetric) Name() string {
	return s.name
}

func (s *LatMetric) Begin() {
	mu.Lock()
	s.t = time.Now()
	mu.Unlock()
}

func (s *LatMetric) End() {
	endT := time.Now()
	mu.Lock()
	s.count++
	s.latencySum += endT.Sub(s.t)
	mu.Unlock()
}

func (s *LatMetric) Clear() {
	mu.Lock()
	s.count = 0
	s.latencySum = 0
	s.t = time.Unix(0, 0)
	mu.Unlock()
}

func (s *LatMetric) Get() int64 {
	mu.Lock()
	v := int64(0)
	if s.count != 0 {
		v = int64(s.latencySum) / int64(s.count)
	}
	mu.Unlock()
	return v
}

func NewCounterMetric(n string) pipeline.CounterMetric {
	return &NormalMetric{name: n}
}

func NewAverageMetric(n string) pipeline.CounterMetric {
	return &AvgMetric{name: n}
}

func NewStringMetric(n string) pipeline.StringMetric {
	return &StrMetric{name: n}
}

func NewLatencyMetric(n string) pipeline.LatencyMetric {
	return &LatMetric{name: n}
}

func NewCounterMetricAndRegister(metricsRecord *pipeline.MetricsRecord, n string) pipeline.CounterMetric {
	metric := &NormalMetric{name: n}
	metricsRecord.RegisterCounterMetric(metric)
	return metric
}

func NewAverageMetricAndRegister(metricsRecord *pipeline.MetricsRecord, n string) pipeline.CounterMetric {
	metric := &AvgMetric{name: n}
	metricsRecord.RegisterCounterMetric(metric)
	return metric
}

func NewStringMetricAndRegister(metricsRecord *pipeline.MetricsRecord, n string) pipeline.StringMetric {
	metric := &StrMetric{name: n}
	metricsRecord.RegisterStringMetric(metric)
	return metric
}

func NewLatencyMetricAndRegister(metricsRecord *pipeline.MetricsRecord, n string) pipeline.LatencyMetric {
	metric := &LatMetric{name: n}
	metricsRecord.RegisterLatencyMetric(metric)
	return metric
}
