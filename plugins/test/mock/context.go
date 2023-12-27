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

package mock

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
	"sync"

	"github.com/alibaba/ilogtail/pkg"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

func NewEmptyContext(project, logstore, configName string) *EmptyContext {
	ctx, c := pkg.NewLogtailContextMetaWithoutAlarm(project, logstore, configName)
	return &EmptyContext{
		ctx:        ctx,
		common:     c,
		checkpoint: make(map[string][]byte),
	}
}

type EmptyContext struct {
	MetricsRecords []pipeline.MetricsRecord
	common         *pkg.LogtailContextMeta
	ctx            context.Context
	checkpoint     map[string][]byte
	pluginNames    string
}

var contextMutex sync.Mutex

func (p *EmptyContext) GetConfigName() string {
	return p.common.GetConfigName()
}
func (p *EmptyContext) GetProject() string {
	return p.common.GetProject()
}
func (p *EmptyContext) GetLogstore() string {
	return p.common.GetLogStore()
}

func (p *EmptyContext) AddPlugin(name string) {
	if len(p.pluginNames) != 0 {
		p.pluginNames += "," + name
	} else {
		p.pluginNames = name
	}
}

func (p *EmptyContext) InitContext(project, logstore, configName string) {
	p.ctx, p.common = pkg.NewLogtailContextMetaWithoutAlarm(project, logstore, configName)
}

func (p *EmptyContext) RegisterMetricRecord(labels map[string]string) pipeline.MetricsRecord {
	contextMutex.Lock()
	defer contextMutex.Unlock()
	procInRecordsTotal := helper.NewCounterMetric("proc_in_records_total")
	procOutRecordsTotal := helper.NewCounterMetric("proc_out_records_total")
	procTimeMS := helper.NewCounterMetric("proc_time_ms")

	counterMetrics := make([]pipeline.CounterMetric, 0)
	stringMetrics := make([]pipeline.StringMetric, 0)
	latencyMetric := make([]pipeline.LatencyMetric, 0)

	counterMetrics = append(counterMetrics, procInRecordsTotal, procOutRecordsTotal, procTimeMS)

	metricRecord := pipeline.MetricsRecord{
		Labels: labels,
		CommonMetrics: pipeline.CommonMetrics{
			ProcInRecordsTotal:  procInRecordsTotal,
			ProcOutRecordsTotal: procOutRecordsTotal,
			ProcTimeMS:          procTimeMS,
		},
		CounterMetrics: counterMetrics,
		StringMetrics:  stringMetrics,
		LatencyMetrics: latencyMetric,
	}
	p.MetricsRecords = append(p.MetricsRecords, metricRecord)
	return metricRecord
}

func (p *EmptyContext) GetMetricRecords() (results []map[string]string) {
	contextMutex.Lock()
	defer contextMutex.Unlock()

	results = make([]map[string]string, 0)
	for _, metricRecord := range p.MetricsRecords {
		oneResult := make(map[string]string)
		for key, value := range metricRecord.Labels {
			oneResult[key] = value
		}
		for _, counterMetric := range metricRecord.CounterMetrics {
			oneResult[counterMetric.Name()] = strconv.FormatInt(counterMetric.Get(), 10)
		}
		results = append(results, oneResult)
	}
	return results
}

func (p *EmptyContext) RegisterCounterMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.CounterMetric) {
	metricsRecord.CounterMetrics = append(metricsRecord.CounterMetrics, metric)
}

func (p *EmptyContext) RegisterStringMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.StringMetric) {
	metricsRecord.StringMetrics = append(metricsRecord.StringMetrics, metric)
}

func (p *EmptyContext) RegisterLatencyMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.LatencyMetric) {
	metricsRecord.LatencyMetrics = append(metricsRecord.LatencyMetrics, metric)
}

func (p *EmptyContext) SaveCheckPoint(key string, value []byte) error {
	logger.Debug(p.ctx, "save checkpoint, key", key, "value", string(value))
	p.checkpoint[key] = value
	return nil
}

func (p *EmptyContext) GetCheckPoint(key string) (value []byte, exist bool) {
	value, ok := p.checkpoint[key]
	logger.Debug(p.ctx, "get checkpoint, key", key, "value", string(value), "ok", ok)
	return value, ok
}

func (p *EmptyContext) SaveCheckPointObject(key string, obj interface{}) error {
	val, err := json.Marshal(obj)
	if err != nil {
		return err
	}
	return p.SaveCheckPoint(key, val)
}

func (p *EmptyContext) GetCheckPointObject(key string, obj interface{}) (exist bool) {
	val, ok := p.GetCheckPoint(key)
	if !ok {
		return false
	}
	err := json.Unmarshal(val, obj)
	if err != nil {
		logger.Error(p.ctx, "CHECKPOINT_INVALID_ALARM", "invalid checkpoint, key", key, "val", util.CutString(string(val), 1024), "error", err)
		return false
	}
	return true
}

func (p *EmptyContext) GetRuntimeContext() context.Context {
	return p.ctx
}

func (p *EmptyContext) GetExtension(name string, cfg any) (pipeline.Extension, error) {
	creator, ok := pipeline.Extensions[name]
	if !ok {
		return nil, fmt.Errorf("extension %s not found", name)
	}
	extension := creator()
	config, err := json.Marshal(cfg)
	if err != nil {
		return nil, err
	}
	err = json.Unmarshal(config, extension)
	if err != nil {
		return nil, err
	}
	err = extension.Init(p)
	if err != nil {
		return nil, err
	}
	return extension, nil
}
