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
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"time"
)

type ServiceWrapperV1 struct {
	pipeline.PluginContext
	Input    pipeline.ServiceInputV1
	Config   *LogstoreConfig
	Tags     map[string]string
	Interval time.Duration

	LogsChan chan *pipeline.LogWithContext

	procInRecordsTotal  pipeline.CounterMetric
	procOutRecordsTotal pipeline.CounterMetric
	procTimeMS          pipeline.CounterMetric
}

func (p *ServiceWrapperV1) Init(name string, pluginID string, childPluginID string) error {
	labels := pipeline.GetCommonLabels(p.Config.Context, name, pluginID, childPluginID)
	p.MetricRecord = p.Config.Context.RegisterMetricRecord(labels)

	p.procInRecordsTotal = helper.NewCounterMetric("proc_in_records_total")
	p.procOutRecordsTotal = helper.NewCounterMetric("proc_out_records_total")
	p.procTimeMS = helper.NewCounterMetric("proc_time_ms")

	p.MetricRecord.RegisterCounterMetric(p.procInRecordsTotal)
	p.MetricRecord.RegisterCounterMetric(p.procOutRecordsTotal)
	p.MetricRecord.RegisterCounterMetric(p.procTimeMS)

	_, err := p.Input.Init(p.Config.Context)
	return err
}

func (p *ServiceWrapperV1) Run(cc *pipeline.AsyncControl) {
	logger.Info(p.Config.Context.GetRuntimeContext(), "start run service", p.Input)

	go func() {
		defer panicRecover(p.Input.Description())
		err := p.Input.Start(p)
		if err != nil {
			logger.Error(p.Config.Context.GetRuntimeContext(), "PLUGIN_ALARM", "start service error, err", err)
		}
		logger.Info(p.Config.Context.GetRuntimeContext(), "service done", p.Input.Description())
	}()

}

func (p *ServiceWrapperV1) Stop() error {
	err := p.Input.Stop()
	if err != nil {
		logger.Error(p.Config.Context.GetRuntimeContext(), "PLUGIN_ALARM", "stop service error, err", err)
	}
	return err
}

func (p *ServiceWrapperV1) AddData(tags map[string]string, fields map[string]string, t ...time.Time) {
	p.AddDataWithContext(tags, fields, nil, t...)
}

func (p *ServiceWrapperV1) AddDataArray(tags map[string]string,
	columns []string,
	values []string,
	t ...time.Time) {
	p.AddDataArrayWithContext(tags, columns, values, nil, t...)
}

func (p *ServiceWrapperV1) AddRawLog(log *protocol.Log) {
	p.AddRawLogWithContext(log, nil)
}

func (p *ServiceWrapperV1) AddDataWithContext(tags map[string]string, fields map[string]string, ctx map[string]interface{}, t ...time.Time) {
	var logTime time.Time
	if len(t) == 0 {
		logTime = time.Now()
	} else {
		logTime = t[0]
	}
	slsLog, _ := helper.CreateLog(logTime, p.Tags, tags, fields)
	p.procInRecordsTotal.Add(1)
	p.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
	p.procOutRecordsTotal.Add(1)
}

func (p *ServiceWrapperV1) AddDataArrayWithContext(tags map[string]string,
	columns []string,
	values []string,
	ctx map[string]interface{},
	t ...time.Time) {
	var logTime time.Time
	if len(t) == 0 {
		logTime = time.Now()
	} else {
		logTime = t[0]
	}
	slsLog, _ := helper.CreateLogByArray(logTime, p.Tags, tags, columns, values)
	p.procInRecordsTotal.Add(1)
	p.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
	p.procOutRecordsTotal.Add(1)
}

func (p *ServiceWrapperV1) AddRawLogWithContext(log *protocol.Log, ctx map[string]interface{}) {
	p.procInRecordsTotal.Add(1)
	p.LogsChan <- &pipeline.LogWithContext{Log: log, Context: ctx}
	p.procOutRecordsTotal.Add(1)
}
