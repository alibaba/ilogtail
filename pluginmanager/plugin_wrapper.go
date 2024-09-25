// Copyright 2024 iLogtail Authors
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
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

/*---------------------
Plugin Input
The input plugin is used for reading data.
---------------------*/

type InputWrapper struct {
	pipeline.PluginContext
	Config   *LogstoreConfig
	Tags     map[string]string
	Interval time.Duration

	outEventsTotal     pipeline.CounterMetric
	outEventsSizeBytes pipeline.CounterMetric
}

func (w *InputWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.outEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.outEventsSizeBytes = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsSizeBytes)
}

// The service plugin is an input plugin used for passively receiving data.
type ServiceWrapper struct {
	InputWrapper
}

// metric plugin is an input plugin used for actively pulling data.
type MetricWrapper struct {
	InputWrapper
	LatencyMetric pipeline.LatencyMetric
}

/*---------------------
Plugin Processor
The processor plugin is used for reading data.
---------------------*/

type ProcessorWrapper struct {
	pipeline.PluginContext
	Config *LogstoreConfig

	inEventsTotal  pipeline.CounterMetric
	outEventsTotal pipeline.CounterMetric
	costTimeMs     pipeline.CounterMetric
}

func (w *ProcessorWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.inEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.outEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.costTimeMs = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
}

/*---------------------
Plugin Aggregator
The aggregator plugin is used for aggregating data.
---------------------*/

type AggregatorWrapper struct {
	pipeline.PluginContext
	Config   *LogstoreConfig
	Interval time.Duration

	inEventsTotal  pipeline.CounterMetric
	outEventsTotal pipeline.CounterMetric
	costTimeMs     pipeline.CounterMetric
}

func (w *AggregatorWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.inEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.outEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.costTimeMs = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
}

/*---------------------
Plugin Flusher
The flusher plugin is used for sending data.
---------------------*/

type FlusherWrapperInterface interface {
	Init(pluginMeta *pipeline.PluginMeta) error
	IsReady(projectName string, logstoreName string, logstoreKey int64) bool
}

type FlusherWrapper struct {
	pipeline.PluginContext
	Config   *LogstoreConfig
	Interval time.Duration

	inEventsTotal      pipeline.CounterMetric
	inEventsSizeBytes  pipeline.CounterMetric
	discardEventsTotal pipeline.CounterMetric
	errorTotal         pipeline.CounterMetric
	costTimeMs         pipeline.LatencyMetric
}

func (w *FlusherWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.inEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.inEventsSizeBytes = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsSizeBytes)
	w.discardEventsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginDiscardEventsTotal)
	w.errorTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginErrorTotal)
	w.costTimeMs = helper.NewLatencyMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
}
