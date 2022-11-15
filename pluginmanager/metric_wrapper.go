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
	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"

	"sync"
	"time"
)

type MetricWrapper struct {
	Input    ilogtail.MetricInput
	Config   *LogstoreConfig
	Tags     map[string]string
	Interval time.Duration

	LogsChan      chan *ilogtail.LogWithContext
	LatencyMetric ilogtail.LatencyMetric

	PipeContext ilogtail.PipelineContext

	shutdown  chan struct{}
	waitgroup sync.WaitGroup
}

func (p *MetricWrapper) Run() {
	logger.Info(p.Config.Context.GetRuntimeContext(), "start run metric ", p.Input)
	p.shutdown = make(chan struct{}, 1)
	p.waitgroup.Add(1)
	defer p.waitgroup.Done()
	defer panicRecover(p.Input.Description())
	for {
		if !util.RandomSleep(p.Interval, 0.1, p.shutdown) {
			p.LatencyMetric.Begin()
			var err error
			if slsInput, ok := p.Input.(ilogtail.SlsMetricInput); ok {
				err = slsInput.Collect(p)
			} else if pipeInput, ok := p.Input.(ilogtail.PipelineMetricInput); ok {
				err = pipeInput.Collect(p.PipeContext)
			}
			p.LatencyMetric.End()
			if err != nil {
				logger.Error(p.Config.Context.GetRuntimeContext(), "INPUT_COLLECT_ALARM", "error", err)
			}
			continue
		}
		return
	}
}

func (p *MetricWrapper) Stop() {
	close(p.shutdown)
	p.waitgroup.Wait()
	logger.Info(p.Config.Context.GetRuntimeContext(), "stop metric success", p.Input)
}

func (p *MetricWrapper) AddData(tags map[string]string, fields map[string]string, t ...time.Time) {
	p.AddDataWithContext(tags, fields, nil, t...)
}

func (p *MetricWrapper) AddDataArray(tags map[string]string,
	columns []string,
	values []string,
	t ...time.Time) {
	p.AddDataArrayWithContext(tags, columns, values, nil, t...)
}

func (p *MetricWrapper) AddRawLog(log *protocol.Log) {
	p.AddRawLogWithContext(log, nil)
}

func (p *MetricWrapper) AddDataWithContext(tags map[string]string, fields map[string]string, ctx map[string]interface{}, t ...time.Time) {
	var logTime time.Time
	if len(t) == 0 {
		logTime = time.Now()
	} else {
		logTime = t[0]
	}
	slsLog, _ := util.CreateLog(logTime, p.Tags, tags, fields)
	p.LogsChan <- &ilogtail.LogWithContext{Log: slsLog, Context: ctx}
}

func (p *MetricWrapper) AddDataArrayWithContext(tags map[string]string,
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
	slsLog, _ := util.CreateLogByArray(logTime, p.Tags, tags, columns, values)
	p.LogsChan <- &ilogtail.LogWithContext{Log: slsLog, Context: ctx}
}

func (p *MetricWrapper) AddRawLogWithContext(log *protocol.Log, ctx map[string]interface{}) {
	p.LogsChan <- &ilogtail.LogWithContext{Log: log, Context: ctx}
}
