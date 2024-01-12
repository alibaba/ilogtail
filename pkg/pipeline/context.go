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
)

type CommonContext struct {
	Project    string
	Logstore   string
	ConfigName string
}

type MetricsRecord struct {
	Labels map[string]string

	CounterMetrics []CounterMetric
	StringMetrics  []StringMetric
	LatencyMetrics []LatencyMetric
}

func (m *MetricsRecord) RegisterCounterMetric(metric CounterMetric) {
	m.CounterMetrics = append(m.CounterMetrics, metric)

}

func (m *MetricsRecord) RegisterStringMetric(metric StringMetric) {
	m.StringMetrics = append(m.StringMetrics, metric)
}

func (m *MetricsRecord) RegisterLatencyMetric(metric LatencyMetric) {
	m.LatencyMetrics = append(m.LatencyMetrics, metric)
}

func GetCommonLabels(context Context, pluginName string, pluginID string, nodeID string, childNodeID string) map[string]string {
	labels := make(map[string]string)
	labels["project"] = context.GetProject()
	labels["logstore"] = context.GetLogstore()
	labels["config_name"] = context.GetConfigName()
	if len(pluginID) > 0 {
		labels["plugin_id"] = pluginID
	}
	if len(pluginName) > 0 {
		labels["plugin_name"] = pluginName
	}
	if len(nodeID) > 0 {
		labels["node_id"] = nodeID
	}
	if len(childNodeID) > 0 {
		labels["child_node_id"] = childNodeID
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
	GetExtension(name string, cfg any) (Extension, error)

	GetMetricRecords() []map[string]string
	RegisterMetricRecord(labels map[string]string) *MetricsRecord

	SetMetricRecord(metricsRecord *MetricsRecord)
	GetMetricRecord() *MetricsRecord

	SaveCheckPoint(key string, value []byte) error
	GetCheckPoint(key string) (value []byte, exist bool)
	SaveCheckPointObject(key string, obj interface{}) error
	GetCheckPointObject(key string, obj interface{}) (exist bool)
}
