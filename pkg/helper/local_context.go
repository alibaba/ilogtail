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
	"context"
	"encoding/json"
	"strconv"
	"sync"

	"github.com/alibaba/ilogtail/pkg"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

type LocalContext struct {
	MetricsRecords             []*pipeline.MetricsRecord
	logstoreConfigMetricRecord *pipeline.MetricsRecord

	AllCheckPoint map[string][]byte

	ctx         context.Context
	pluginNames string
	common      *pkg.LogtailContextMeta
}

var contextMutex sync.Mutex

func (p *LocalContext) GetConfigName() string {
	return p.common.GetConfigName()
}
func (p *LocalContext) GetProject() string {
	return p.common.GetProject()
}
func (p *LocalContext) GetLogstore() string {
	return p.common.GetLogStore()
}

func (p *LocalContext) AddPlugin(name string) {
	if len(p.pluginNames) != 0 {
		p.pluginNames += "," + name
	} else {
		p.pluginNames = name
	}
}

func (p *LocalContext) InitContext(project, logstore, configName string) {
	// bind metadata information.
	p.ctx, p.common = pkg.NewLogtailContextMetaWithoutAlarm(project, logstore, configName)
	p.AllCheckPoint = make(map[string][]byte)
}

func (p *LocalContext) GetRuntimeContext() context.Context {
	return p.ctx
}

func (p *LocalContext) GetExtension(name string, cfg any) (pipeline.Extension, error) {
	return nil, nil
}

func (p *LocalContext) RegisterMetricRecord(labels []pipeline.LabelPair) *pipeline.MetricsRecord {
	contextMutex.Lock()
	defer contextMutex.Unlock()

	counterMetrics := make([]pipeline.CounterMetric, 0)
	stringMetrics := make([]pipeline.StringMetric, 0)
	latencyMetric := make([]pipeline.LatencyMetric, 0)

	metricRecord := pipeline.MetricsRecord{
		Labels:         labels,
		CounterMetrics: counterMetrics,
		StringMetrics:  stringMetrics,
		LatencyMetrics: latencyMetric,
	}
	p.MetricsRecords = append(p.MetricsRecords, &metricRecord)
	return &metricRecord
}

func (p *LocalContext) RegisterLogstoreConfigMetricRecord(labels []pipeline.LabelPair) *pipeline.MetricsRecord {
	counterMetrics := make([]pipeline.CounterMetric, 0)
	stringMetrics := make([]pipeline.StringMetric, 0)
	latencyMetric := make([]pipeline.LatencyMetric, 0)

	p.logstoreConfigMetricRecord = &pipeline.MetricsRecord{
		Labels:         labels,
		CounterMetrics: counterMetrics,
		StringMetrics:  stringMetrics,
		LatencyMetrics: latencyMetric,
	}
	return p.logstoreConfigMetricRecord
}

func (p *LocalContext) GetLogstoreConfigMetricRecord() *pipeline.MetricsRecord {
	return p.logstoreConfigMetricRecord
}

func (p *LocalContext) ExportMetricRecords() (results []map[string]string) {
	contextMutex.Lock()
	defer contextMutex.Unlock()

	results = make([]map[string]string, 0)
	for _, metricRecord := range p.MetricsRecords {
		oneResult := make(map[string]string)
		for _, label := range metricRecord.Labels {
			oneResult["label."+label.Key] = label.Value
		}
		for _, counterMetric := range metricRecord.CounterMetrics {
			oneResult["value."+counterMetric.Name()] = strconv.FormatInt(counterMetric.GetAndReset(), 10)
		}
		for _, stringMetric := range metricRecord.StringMetrics {
			oneResult["value."+stringMetric.Name()] = stringMetric.GetAndReset()
		}
		for _, latencyMetric := range metricRecord.LatencyMetrics {
			oneResult["value."+latencyMetric.Name()] = strconv.FormatInt(latencyMetric.GetAndReset(), 10)
		}
		results = append(results, oneResult)
	}
	return results
}

func (p *LocalContext) GetMetricRecord() *pipeline.MetricsRecord {
	if len(p.MetricsRecords) > 0 {
		return p.MetricsRecords[len(p.MetricsRecords)-1]
	}
	return nil
}

func (p *LocalContext) RegisterCounterMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.CounterMetric) {
	metricsRecord.CounterMetrics = append(metricsRecord.CounterMetrics, metric)
}

func (p *LocalContext) RegisterStringMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.StringMetric) {
	metricsRecord.StringMetrics = append(metricsRecord.StringMetrics, metric)
}

func (p *LocalContext) RegisterLatencyMetric(metricsRecord pipeline.MetricsRecord, metric pipeline.LatencyMetric) {
	metricsRecord.LatencyMetrics = append(metricsRecord.LatencyMetrics, metric)
}

func (p *LocalContext) SaveCheckPoint(key string, value []byte) error {
	logger.Debug(p.ctx, "save checkpoint, key", key, "value", string(value))
	p.AllCheckPoint[key] = value
	return nil
}

func (p *LocalContext) GetCheckPoint(key string) (value []byte, exist bool) {
	value, exist = p.AllCheckPoint[key]
	logger.Debug(p.ctx, "get checkpoint, key", key, "value", string(value))
	return value, exist
}

func (p *LocalContext) SaveCheckPointObject(key string, obj interface{}) error {
	val, err := json.Marshal(obj)
	if err != nil {
		logger.Debug(p.ctx, "CHECKPOINT_INVALID_ALARM", "save checkpoint error, invalid checkpoint, key", key, "val", util.CutString(string(val), 1024), "error", err)
		return err
	}
	return p.SaveCheckPoint(key, val)
}

func (p *LocalContext) GetCheckPointObject(key string, obj interface{}) (exist bool) {
	val, ok := p.GetCheckPoint(key)
	if !ok {
		return false
	}
	err := json.Unmarshal(val, obj)
	if err != nil {
		logger.Debug(p.ctx, "CHECKPOINT_INVALID_ALARM", "get checkpoint error, invalid checkpoint, key", key, "val", util.CutString(string(val), 1024), "error", err)
		return false
	}
	return true
}
