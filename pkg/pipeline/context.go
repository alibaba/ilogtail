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

package pipeline

import (
	"context"
	"sync"

	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type CommonContext struct {
	Project    string
	Logstore   string
	ConfigName string
}

type LabelPair = Label

const SelfMetricNameKey = "__name__"

type MetricsRecord struct {
	Context Context
	Labels  []LabelPair

	sync.RWMutex
	MetricCollectors []MetricCollector
}

func (m *MetricsRecord) appendLabels(log *protocol.Log) {
	for _, label := range m.Labels {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: label.Key, Value: label.Value})
	}
}

func (m *MetricsRecord) insertLabels(record map[string]string) {
	for _, label := range m.Labels {
		record["label."+label.Key] = label.Value
	}
}

func (m *MetricsRecord) RegisterMetricCollector(collector MetricCollector) {
	m.Lock()
	defer m.Unlock()
	m.MetricCollectors = append(m.MetricCollectors, collector)
}

func (m *MetricsRecord) Serialize(logGroup *protocol.LogGroup) {
	m.RLock()
	defer m.RUnlock()
	for _, metricCollector := range m.MetricCollectors {
		metrics := metricCollector.Collect()
		for _, metric := range metrics {
			log := &protocol.Log{}
			metric.Serialize(log)
			if len(log.Contents) == 0 {
				continue
			}
			m.appendLabels(log) // append metrics record labels
			logGroup.Logs = append(logGroup.Logs, log)
		}
	}
}

// ExportMetricRecords is used for exporting metrics records.
// It will replace Serialize in the future.
func (m *MetricsRecord) ExportMetricRecords() []map[string]string {
	m.RLock()
	defer m.RUnlock()

	records := make([]map[string]string, 0)
	for _, metricCollector := range m.MetricCollectors {
		metrics := metricCollector.Collect()
		record := map[string]string{}
		m.insertLabels(record)
		for _, metric := range metrics {
			singleMetricRecord := metric.Export()
			if len(singleMetricRecord) == 0 {
				continue
			}
			valueName := singleMetricRecord[SelfMetricNameKey]
			valueValue := singleMetricRecord[valueName]
			record["value."+valueName] = valueValue
		}
		records = append(records, record)
	}
	return records
}

func GetCommonLabels(context Context, pluginMeta *PluginMeta) []LabelPair {
	labels := make([]LabelPair, 0)
	labels = append(labels, LabelPair{Key: "project", Value: context.GetProject()})
	labels = append(labels, LabelPair{Key: "logstore", Value: context.GetLogstore()})
	labels = append(labels, LabelPair{Key: "config_name", Value: context.GetConfigName()})

	if len(pluginMeta.PluginID) > 0 {
		labels = append(labels, LabelPair{Key: "plugin_id", Value: pluginMeta.PluginID})
	}
	if len(pluginMeta.NodeID) > 0 {
		labels = append(labels, LabelPair{Key: "node_id", Value: pluginMeta.NodeID})
	}
	if len(pluginMeta.ChildNodeID) > 0 {
		labels = append(labels, LabelPair{Key: "child_node_id", Value: pluginMeta.ChildNodeID})
	}
	if len(pluginMeta.PluginType) > 0 {
		labels = append(labels, LabelPair{Key: "plugin_name", Value: pluginMeta.PluginType})
	}
	return labels
}

// Context for plugin
type Context interface {
	InitContext(project, logstore, configName string)

	GetConfigName() string
	GetProject() string
	GetLogstore() string
	GetRuntimeContext() context.Context
	GetPipelineScopeConfig() *config.GlobalConfig
	GetExtension(name string, cfg any) (Extension, error)
	// RegisterCounterMetric(metric CounterMetric)
	// RegisterStringMetric(metric StringMetric)
	// RegisterLatencyMetric(metric LatencyMetric)

	SaveCheckPoint(key string, value []byte) error
	GetCheckPoint(key string) (value []byte, exist bool)
	SaveCheckPointObject(key string, obj interface{}) error
	GetCheckPointObject(key string, obj interface{}) (exist bool)

	// APIs for self monitor
	RegisterMetricRecord(labels []LabelPair) *MetricsRecord // for v1.8.8 compatible
	GetMetricRecord() *MetricsRecord                        // for v1.8.8 compatible
	ExportMetricRecords() []map[string]string               // for v1.8.8 compatible
	MetricSerializeToPB(logGroup *protocol.LogGroup)
}
