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
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

type ProcessorWrapperV2 struct {
	ProcessorWrapper
	Processor pipeline.ProcessorV2
}

func (wrapper *ProcessorWrapperV2) Init(name string, pluginID string, childPluginID string) error {
	labels := pipeline.GetCommonLabels(wrapper.Config.Context, name, pluginID, childPluginID)
	wrapper.MetricRecord = wrapper.Config.Context.RegisterMetricRecord(labels)

	wrapper.procInRecordsTotal = helper.NewCounterMetric("proc_in_records_total")
	wrapper.procOutRecordsTotal = helper.NewCounterMetric("proc_out_records_total")
	wrapper.procTimeMS = helper.NewCounterMetric("proc_time_ms")

	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procInRecordsTotal)
	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procOutRecordsTotal)
	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procTimeMS)

	wrapper.Config.Context.SetMetricRecord(wrapper.MetricRecord)
	return wrapper.Processor.Init(wrapper.Config.Context)
}

func (wrapper *ProcessorWrapperV2) Process(in *models.PipelineGroupEvents, context pipeline.PipelineContext) {
	wrapper.procInRecordsTotal.Add(int64(len(in.Events)))
	startTime := time.Now()
	wrapper.Processor.Process(in, context)
	wrapper.procTimeMS.Add(int64(time.Since(startTime).Milliseconds()))
	wrapper.procOutRecordsTotal.Add(int64(len(in.Events)))
}
