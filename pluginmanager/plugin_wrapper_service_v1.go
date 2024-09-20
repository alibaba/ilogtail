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

	eventCached []*protocol.LogEvent
	tagCached   []map[string]string
	ctxCached   []map[string]interface{}
	timer       *time.Timer
	pbBuffer    []byte
}

func (p *ServiceWrapperV1) Init(pluginMeta *pipeline.PluginMeta) error {
	p.InitMetricRecord(pluginMeta)

	_, err := p.Input.Init(p.Config.Context)
	return err
}

func (p *ServiceWrapperV1) Run(cc *pipeline.AsyncControl) {
	logger.Info(p.Config.Context.GetRuntimeContext(), "start run service", p.Input)

	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		p.LogsCachedChan = make(chan *pipeline.LogEventWithContext, 10)
		p.ShutdownCachedChan = make(chan struct{})
		p.eventCached = make([]*protocol.LogEvent, 0, p.MaxCachedSize+1)
		p.tagCached = make([]map[string]string, 0, p.MaxCachedSize+1)
		p.ctxCached = make([]map[string]interface{}, 0, p.MaxCachedSize+1)
		p.timer = time.NewTimer(p.PushNativeTimeout)
		go p.runPushNativeProcessQueueInternal()
	}

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
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		p.ShutdownCachedChan <- struct{}{}
	}
	err := p.Input.Stop()
	if err != nil {
		logger.Error(p.Config.Context.GetRuntimeContext(), "PLUGIN_ALARM", "stop service error, err", err)
	}
	if p.Config.GlobalConfig.GoInputToNativeProcessor {
		p.timer.Stop()
		close(p.LogsCachedChan)
		close(p.ShutdownCachedChan)
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
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Tags: tags, Context: ctx}
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
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Tags: tags, Context: ctx}
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
		p.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Context: ctx}
		return
	}
	p.LogsChan <- &pipeline.LogWithContext{Log: log, Context: ctx}
}

func (p *ServiceWrapperV1) runPushNativeProcessQueueInternal() {
	var event *pipeline.LogEventWithContext
	var isValidToPushNativeProcessQueue bool = true

	for {
		if isValidToPushNativeProcessQueue {
			select {
			case <-p.timer.C:
				isValidToPushNativeProcessQueue = p.pushNativeProcessQueue()
			case event = <-p.LogsCachedChan:
				p.eventCached = append(p.eventCached, event.LogEvent)
				p.tagCached = append(p.tagCached, event.Tags)
				p.ctxCached = append(p.ctxCached, event.Context)
				if len(p.eventCached) < p.MaxCachedSize {
					continue
				}
				isValidToPushNativeProcessQueue = p.pushNativeProcessQueue()
			case <-p.ShutdownCachedChan:
				for event = range p.LogsCachedChan {
				}
				return
			}
		} else {
			select {
			case <-p.timer.C:
				isValidToPushNativeProcessQueue = p.pushNativeProcessQueue()
			case <-p.ShutdownCachedChan:
				for event = range p.LogsCachedChan {
				}
				return
			}
		}
	}

}

func (p *ServiceWrapperV1) pushNativeProcessQueue() bool {
	if len(p.eventCached) == 0 {
		return true
	}

	// create pipelineEventGroup and marshal to pbBuffer
	pushSize := 0
	if len(p.pbBuffer) == 0 {
		tag := p.tagCached[0]
		ctx := p.ctxCached[0]
		i := 1
		for ; i < len(p.eventCached); i++ {
			same := true
			for k, v := range p.tagCached[i] {
				if tag[k] != v {
					same = false
					break
				}
			}
			if !same || !reflect.DeepEqual(ctx, p.ctxCached[i]) {
				break
			}
		}
		pushSize = i
		group, _ := helper.CreatePipelineEventGroupV1(p.eventCached[:pushSize], p.Tags, tag, ctx)
		pbSize := group.Size()
		if cap(p.pbBuffer) < pbSize {
			if cap(p.pbBuffer)*2 >= pbSize {
				p.pbBuffer = make([]byte, cap(p.pbBuffer)*2)
			} else {
				p.pbBuffer = make([]byte, pbSize)
			}
		}
		n, _ := group.MarshalTo(p.pbBuffer)
		p.pbBuffer = p.pbBuffer[:n]
	}

	// try to pushNativeProcessQueue
	var rst int = 0
	switch p.Input.GetMode() {
	case pipeline.PUSH:
		for i := 0; i < 5; i++ {
			if logtail.IsValidToProcess(p.Config.ConfigName) {
				if rst = logtail.PushQueue(p.Config.ConfigName, p.pbBuffer); rst == 0 {
					break
				}
			}
			time.Sleep(time.Duration(10) * time.Millisecond)
		}
		// if logtail.IsValidToProcess(p.Config.ConfigName) {
		// 	rst = logtail.PushQueue(p.Config.ConfigName, p.pbBuffer)
		// } else {
		// 	rst = -1
		// }
	case pipeline.PULL:
		logtail.PushQueue(p.Config.ConfigName, p.pbBuffer)
	default:
	}

	// clear buffer and reset timer
	if !p.timer.Stop() {
		select {
		case <-p.timer.C:
		default:
		}
	}
	p.timer.Reset(p.PushNativeTimeout)
	if rst != 0 {
		return false
	}
	if pushSize != 0 {
		i := 0
		for ; i < pushSize; i++ {
			helper.LogEventPool.Put(p.eventCached[i])
		}
		for ; i < len(p.eventCached); i++ {
			p.eventCached[i-pushSize] = p.eventCached[i]
			p.tagCached[i-pushSize] = p.tagCached[i]
			p.ctxCached[i-pushSize] = p.ctxCached[i]
		}
		p.eventCached = p.eventCached[:len(p.eventCached)-pushSize]
		p.tagCached = p.tagCached[:len(p.tagCached)-pushSize]
		p.ctxCached = p.ctxCached[:len(p.ctxCached)-pushSize]
	}
	p.pbBuffer = p.pbBuffer[:0]
	return true
}
