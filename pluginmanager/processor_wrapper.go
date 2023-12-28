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
	"fmt"
	"time"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type CommonWrapper struct {
	pipeline.PluginContext

	Config              *LogstoreConfig
	procInRecordsTotal  pipeline.CounterMetric
	procOutRecordsTotal pipeline.CounterMetric
	procTimeMS          pipeline.CounterMetric
}

type ProcessorWrapperV1 struct {
	CommonWrapper
	Processor pipeline.ProcessorV1
	LogsChan  chan *pipeline.LogWithContext
}

type ProcessorWrapperV2 struct {
	CommonWrapper
	Processor pipeline.ProcessorV2
	LogsChan  chan *pipeline.LogWithContext
}

func (wrapper *ProcessorWrapperV1) Init(name string) error {
	labels := make(map[string]string)
	labels["project"] = wrapper.Config.Context.GetProject()
	labels["logstore"] = wrapper.Config.Context.GetLogstore()
	labels["configName"] = wrapper.Config.Context.GetConfigName()
	labels["plugin_name"] = name
	wrapper.MetricRecord = wrapper.Config.Context.RegisterMetricRecord(labels)

	fmt.Println("wrapper :", wrapper.MetricRecord)

	wrapper.procInRecordsTotal = wrapper.MetricRecord.CommonMetrics.ProcInRecordsTotal
	wrapper.procOutRecordsTotal = wrapper.MetricRecord.CommonMetrics.ProcOutRecordsTotal
	wrapper.procTimeMS = wrapper.MetricRecord.CommonMetrics.ProcTimeMS

	wrapper.Config.Context.SetMetricRecord(wrapper.MetricRecord)
	return wrapper.Processor.Init(wrapper.Config.Context)
}

func (wrapper *ProcessorWrapperV1) Process(logArray []*protocol.Log) []*protocol.Log {
	wrapper.procInRecordsTotal.Add(int64(len(logArray)))
	startTime := time.Now()
	result := wrapper.Processor.ProcessLogs(logArray)
	wrapper.procTimeMS.Add(int64(time.Since(startTime)))
	wrapper.procOutRecordsTotal.Add(int64(len(result)))
	return result
}

func (wrapper *ProcessorWrapperV2) Init(name string) error {
	return wrapper.Processor.Init(wrapper.Config.Context)
}

func (wrapper *ProcessorWrapperV2) Process(in *models.PipelineGroupEvents, context pipeline.PipelineContext) {
	wrapper.Processor.Process(in, context)
}
