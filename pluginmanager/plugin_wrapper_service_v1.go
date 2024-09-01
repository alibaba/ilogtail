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
	"reflect"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/logtail"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"time"
)

type ServiceWrapperV1 struct {
	ServiceWrapper
	LogsChan chan *pipeline.LogWithContext
	Input    pipeline.ServiceInputV1

	MaxCachedSize      int
	PushNativeTimeout  time.Duration
	LogsCachedChan     chan *pipeline.LogEventWithContext
	ShutdownCachedChan chan struct{}
}

func (p *ServiceWrapperV1) Init(pluginMeta *pipeline.PluginMeta) error {
	p.InitMetricRecord(pluginMeta)

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

	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		go p.runPushNativeProcessQueueInternal()
	}
}

func (p *ServiceWrapperV1) Stop() error {
	err := p.Input.Stop()
	if err != nil {
		logger.Error(p.Config.Context.GetRuntimeContext(), "PLUGIN_ALARM", "stop service error, err", err)
	}
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		p.ShutdownCachedChan <- struct{}{}
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
	// need push to native processor
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEvent(logTime, p.Config.GlobalConfig.EnableTimestampNanosecond, fields)
		ctx["tags"] = tags
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Context: ctx}
		p.inputRecordsTotal.Add(1)
		p.inputRecordsSizeBytes.Add(int64(logEvent.Size()))
		return
	}
	slsLog, _ := helper.CreateLog(logTime, len(t) != 0, p.Tags, tags, fields)
	p.inputRecordsTotal.Add(1)
	p.inputRecordsSizeBytes.Add(int64(slsLog.Size()))
	p.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
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
	// need push to native processor
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEventByArray(logTime, p.Config.GlobalConfig.EnableTimestampNanosecond, columns, values)
		ctx["tags"] = tags
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Context: ctx}
		p.inputRecordsTotal.Add(1)
		p.inputRecordsSizeBytes.Add(int64(logEvent.Size()))
		return
	}
	slsLog, _ := helper.CreateLogByArray(logTime, len(t) != 0, p.Tags, tags, columns, values)
	p.inputRecordsTotal.Add(1)
	p.inputRecordsSizeBytes.Add(int64(slsLog.Size()))
	p.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
}

func (p *ServiceWrapperV1) AddRawLogWithContext(log *protocol.Log, ctx map[string]interface{}) {
	p.inputRecordsTotal.Add(1)
	p.inputRecordsSizeBytes.Add(int64(log.Size()))
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEventByRawLogV1(log)
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Context: nil}
		return
	}
	p.LogsChan <- &pipeline.LogWithContext{Log: log, Context: ctx}
}

func (p *ServiceWrapperV1) runPushNativeProcessQueueInternal() {
	eventCached := make([]*protocol.LogEvent, 0, p.MaxCachedSize+1)
	var ctxCached map[string]interface{}
	var event *pipeline.LogEventWithContext

	timer := time.NewTimer(p.PushNativeTimeout)
	defer timer.Stop()

	for {
		select {
		case <-timer.C:
			if len(eventCached) != 0 {
				p.pushNativeProcessQueue(eventCached, ctxCached)
				eventCached = eventCached[:0]
				ctxCached = make(map[string]interface{})
			}
			timer.Reset(p.PushNativeTimeout)
		case event = <-p.LogsCachedChan:
			logTags, okLog := event.Context["tags"].(map[string]string)
			cachedTags, okCached := ctxCached["tags"].(map[string]string)
			if okLog && okCached {
				if !reflect.DeepEqual(logTags, cachedTags) {
					p.pushNativeProcessQueue(eventCached, ctxCached)
					eventCached = eventCached[:0]
					ctxCached = event.Context
				}
			}
			eventCached = append(eventCached, event.LogEvent)
			if len(ctxCached) == 0 {
				ctxCached = event.Context
			}
			if len(eventCached) >= p.MaxCachedSize {
				p.pushNativeProcessQueue(eventCached, ctxCached)
				eventCached = eventCached[:0]
				ctxCached = make(map[string]interface{})
			}
			timer.Reset(p.PushNativeTimeout)
		case <-p.ShutdownCachedChan:
			if len(eventCached) != 0 {
				p.pushNativeProcessQueue(eventCached, ctxCached)
			}
			for len(p.LogsCachedChan) > 0 {
				<-p.LogsCachedChan
			}
			close(p.LogsCachedChan)
			close(p.ShutdownCachedChan)
			return
		}
	}
}

func (p *ServiceWrapperV1) pushNativeProcessQueue(events []*protocol.LogEvent, ctx map[string]interface{}) {
	tags, ok := ctx["tags"].(map[string]string)
	if ok {
		for k, v := range p.Tags {
			tags[k] = v
		}
	}
	group, _ := helper.CreatePipelineEventGroupV1(events, ctx)
	buffer, err := group.Marshal()
	if err != nil {
		logger.Error(p.Config.Context.GetRuntimeContext(), "INPUT_COLLECT_ALARM", "marshal log failed", err)
		return
	}
	pushRst := -1
	switch p.Input.InputMode() {
	case pipeline.PUSH:
		for i := 0; i < 5; i++ {
			if logtail.IsValidToProcess(p.Config.ConfigName) && logtail.PushQueue(p.Config.ConfigName, buffer) == 0 {
				break
			}
			time.Sleep(time.Duration(10) * time.Millisecond)
		}
	case pipeline.PULL:
		pushRst = logtail.PushQueue(p.Config.ConfigName, buffer)
	default:
	}
	if pushRst != 0 {
		logger.Error(p.Config.Context.GetRuntimeContext(), "INPUT_COLLECT_ALARM", "push queue failed", nil)
	}
}
