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

package pluginmanager

import (
	"context"
	"encoding/json"
	"fmt"
	"strconv"
	"sync"

	"github.com/alibaba/ilogtail/pkg"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

type ContextImp struct {
	MetricsRecords             []*pipeline.MetricsRecord
	logstoreConfigMetricRecord *pipeline.MetricsRecord
	common                     *pkg.LogtailContextMeta
	pluginNames                string
	ctx                        context.Context
	logstoreC                  *LogstoreConfig
}

var contextMutex sync.Mutex

func (p *ContextImp) GetRuntimeContext() context.Context {
	return p.ctx
}

func (p *ContextImp) GetExtension(name string, cfg any) (pipeline.Extension, error) {
	if p.logstoreC == nil || p.logstoreC.PluginRunner == nil {
		return nil, fmt.Errorf("pipeline not initialized")
	}
	// try to find in extensions that explicitly defined in pipeline
	exists, ok := p.logstoreC.PluginRunner.GetExtension(name)
	if ok {
		return exists, nil
	}

	// if it's a naming extension, we won't do further create
	if isPluginTypeWithID(name) {
		return nil, fmt.Errorf("not found extension: %s", name)
	}

	// create if not found
	typeWithID := p.logstoreC.genEmbeddedPluginName(getPluginType(name))
	rawType, pluginID := getPluginTypeAndID(typeWithID)
	err := loadExtension(typeWithID, rawType, pluginID, "", p.logstoreC, cfg)
	if err != nil {
		return nil, err
	}

	// get the new created extension
	exists, ok = p.logstoreC.PluginRunner.GetExtension(typeWithID)
	if !ok {
		return nil, fmt.Errorf("failed to load extension: %s", typeWithID)
	}
	return exists, nil
}

func (p *ContextImp) GetConfigName() string {
	return p.common.GetConfigName()
}
func (p *ContextImp) GetProject() string {
	return p.common.GetProject()
}
func (p *ContextImp) GetLogstore() string {
	return p.common.GetLogStore()
}

func (p *ContextImp) AddPlugin(name string) {
	if len(p.pluginNames) != 0 {
		p.pluginNames += "," + name
	} else {
		p.pluginNames = name
	}
}

func (p *ContextImp) InitContext(project, logstore, configName string) {
	// bind metadata information.
	p.ctx, p.common = pkg.NewLogtailContextMeta(project, logstore, configName)
}

func (p *ContextImp) RegisterMetricRecord(labels map[string]string) *pipeline.MetricsRecord {
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

func (p *ContextImp) RegisterLogstoreConfigMetricRecord(labels map[string]string) *pipeline.MetricsRecord {
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

func (p *ContextImp) GetLogstoreConfigMetricRecord() *pipeline.MetricsRecord {
	return p.logstoreConfigMetricRecord
}

func (p *ContextImp) ExportMetricRecords() (results []map[string]string) {
	contextMutex.Lock()
	defer contextMutex.Unlock()

	results = make([]map[string]string, 0)
	for _, metricRecord := range p.MetricsRecords {
		oneResult := make(map[string]string)
		for key, value := range metricRecord.Labels {
			oneResult["label."+key] = value
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

func (p *ContextImp) GetMetricRecord() *pipeline.MetricsRecord {
	if len(p.MetricsRecords) > 0 {
		return p.MetricsRecords[len(p.MetricsRecords)-1]
	}
	return nil
}

func (p *ContextImp) RegisterCounterMetric(metricsRecord *pipeline.MetricsRecord, metric pipeline.CounterMetric) {
	metricsRecord.CounterMetrics = append(metricsRecord.CounterMetrics, metric)
}

func (p *ContextImp) RegisterStringMetric(metricsRecord *pipeline.MetricsRecord, metric pipeline.StringMetric) {
	metricsRecord.StringMetrics = append(metricsRecord.StringMetrics, metric)
}

func (p *ContextImp) RegisterLatencyMetric(metricsRecord *pipeline.MetricsRecord, metric pipeline.LatencyMetric) {
	metricsRecord.LatencyMetrics = append(metricsRecord.LatencyMetrics, metric)
}

func (p *ContextImp) SaveCheckPoint(key string, value []byte) error {
	logger.Debug(p.ctx, "save checkpoint, key", key, "value", string(value))
	return CheckPointManager.SaveCheckpoint(p.GetConfigName(), key, value)
}

func (p *ContextImp) GetCheckPoint(key string) (value []byte, exist bool) {
	value, err := CheckPointManager.GetCheckpoint(p.GetConfigName(), key)
	logger.Debug(p.ctx, "get checkpoint, key", key, "value", string(value), "error", err)
	return value, value != nil
}

func (p *ContextImp) SaveCheckPointObject(key string, obj interface{}) error {
	val, err := json.Marshal(obj)
	if err != nil {
		return err
	}
	return p.SaveCheckPoint(key, val)
}

func (p *ContextImp) GetCheckPointObject(key string, obj interface{}) (exist bool) {
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
