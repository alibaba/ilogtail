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

	"github.com/alibaba/ilogtail/pkg/protocol"
)

type CommonContext struct {
	Project    string
	Logstore   string
	ConfigName string
}

type LabelPair = Label

type MetricsRecord struct {
	Labels []LabelPair

	MetricCollectors []MetricCollector
}

func (m *MetricsRecord) NewMetricProtocol() *protocol.Log {
	log := &protocol.Log{}
	for _, v := range m.Labels {
		log.Contents = append(log.Contents, &protocol.Log_Content{Key: v.Key, Value: v.Value})
	}
	return log
}

func (m *MetricsRecord) RegisterMetricVector(collecter MetricCollector) {
	m.MetricCollectors = append(m.MetricCollectors, collecter)
}

func (m *MetricsRecord) Collect() []Metric {
	metrics := make([]Metric, 0)
	for _, counterMetric := range m.MetricCollectors {
		metrics = append(metrics, counterMetric.Collect()...)
	}
	return metrics
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
	GetExtension(name string, cfg any) (Extension, error)
	// RegisterCounterMetric(metric CounterMetric)
	// RegisterStringMetric(metric StringMetric)
	// RegisterLatencyMetric(metric LatencyMetric)

	SaveCheckPoint(key string, value []byte) error
	GetCheckPoint(key string) (value []byte, exist bool)
	SaveCheckPointObject(key string, obj interface{}) error
	GetCheckPointObject(key string, obj interface{}) (exist bool)

	// APIs for self monitor
	GetMetricRecord() *MetricsRecord // for v1.8.8 compatible
	MetricSerializeToPB(logGroup *protocol.LogGroup)
}
