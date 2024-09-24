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

	inputRecordsTotal     pipeline.CounterMetric
	inputRecordsSizeBytes pipeline.CounterMetric
}

func (w *InputWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.inputRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.inputRecordsSizeBytes = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsSizeBytes)
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

	procInRecordsTotal  pipeline.CounterMetric
	procOutRecordsTotal pipeline.CounterMetric
	procTimeMS          pipeline.CounterMetric
}

func (w *ProcessorWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.procInRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.procOutRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.procTimeMS = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
}

/*---------------------
Plugin Aggregator
The aggregator plugin is used for aggregating data.
---------------------*/

type AggregatorWrapper struct {
	pipeline.PluginContext
	Config   *LogstoreConfig
	Interval time.Duration

	aggrInRecordsTotal  pipeline.CounterMetric
	aggrOutRecordsTotal pipeline.CounterMetric
	aggrTimeMS          pipeline.CounterMetric
}

func (w *AggregatorWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.aggrInRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.aggrOutRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginOutEventsTotal)
	w.aggrTimeMS = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
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

	flusherInRecordsTotal      pipeline.CounterMetric
	flusherInRecordsSizeBytes  pipeline.CounterMetric
	flusherSuccessRecordsTotal pipeline.CounterMetric
	flusherDiscardRecordsTotal pipeline.CounterMetric
	flusherSuccessTimeMs       pipeline.LatencyMetric
	flusherErrorTimeMs         pipeline.LatencyMetric
	flusherErrorTotal          pipeline.CounterMetric
}

func (w *FlusherWrapper) InitMetricRecord(pluginMeta *pipeline.PluginMeta) {
	labels := helper.GetCommonLabels(w.Config.Context, pluginMeta)
	w.MetricRecord = w.Config.Context.RegisterMetricRecord(labels)

	w.flusherInRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsTotal)
	w.flusherInRecordsSizeBytes = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginInEventsSizeBytes)
	w.flusherDiscardRecordsTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginDiscardEventsTotal)
	w.flusherSuccessTimeMs = helper.NewLatencyMetricAndRegister(w.MetricRecord, helper.MetricPluginCostTimeMs)
	w.flusherErrorTotal = helper.NewCounterMetricAndRegister(w.MetricRecord, helper.MetricPluginErrorTotal)

}
