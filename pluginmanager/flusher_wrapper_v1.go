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
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"time"
)

type FlusherWrapper interface {
	Init(name string, pluginID string, childPluginID string) error
	IsReady(projectName string, logstoreName string, logstoreKey int64) bool
}

type CommonWrapper struct {
	pipeline.PluginContext
	Config              *LogstoreConfig
	procInRecordsTotal  pipeline.CounterMetric
	procOutRecordsTotal pipeline.CounterMetric
	procTimeMS          pipeline.CounterMetric
	LogGroupsChan       chan *protocol.LogGroup
	Interval            time.Duration
}

type FlusherWrapperV1 struct {
	CommonWrapper
	Flusher pipeline.FlusherV1
}

func (wrapper *FlusherWrapperV1) Init(name string, pluginID string, childPluginID string) error {
	labels := pipeline.GetCommonLabels(wrapper.Config.Context, name, pluginID, childPluginID)

	wrapper.MetricRecord = wrapper.Config.Context.RegisterMetricRecord(labels)

	wrapper.procInRecordsTotal = helper.NewCounterMetric("proc_in_records_total")
	wrapper.procOutRecordsTotal = helper.NewCounterMetric("proc_out_records_total")
	wrapper.procTimeMS = helper.NewCounterMetric("proc_time_ms")

	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procInRecordsTotal)
	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procOutRecordsTotal)
	wrapper.MetricRecord.RegisterCounterMetric(wrapper.procTimeMS)

	return wrapper.Flusher.Init(wrapper.Config.Context)
}

func (wrapper *FlusherWrapperV1) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return wrapper.Flusher.IsReady(projectName, logstoreName, logstoreKey)
}

func (wrapper *FlusherWrapperV1) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	total := 0
	for _, logGroup := range logGroupList {
		total += len(logGroup.Logs)
	}

	wrapper.procInRecordsTotal.Add(int64(total))
	startTime := time.Now()
	err := wrapper.Flusher.Flush(projectName, logstoreName, configName, logGroupList)
	if err == nil {
		wrapper.procOutRecordsTotal.Add(int64(total))
	}
	wrapper.procTimeMS.Add(time.Since(startTime).Milliseconds())
	return err
}
