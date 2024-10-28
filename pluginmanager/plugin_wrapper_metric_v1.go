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
	"github.com/alibaba/ilogtail/pkg/logtail"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"

	"time"
)

type MetricWrapperV1 struct {
	MetricWrapper
	LogsChan chan *pipeline.LogWithContext
	Input    pipeline.MetricInputV1

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

func (wrapper *MetricWrapperV1) Init(pluginMeta *pipeline.PluginMeta, inputInterval int) error {
	wrapper.InitMetricRecord(pluginMeta)

	interval, err := wrapper.Input.Init(wrapper.Config.Context)
	if err != nil {
		return err
	}
	if interval == 0 {
		interval = inputInterval
	}
	wrapper.Interval = time.Duration(interval) * time.Millisecond
	return nil
}

func (wrapper *MetricWrapperV1) AddData(tags map[string]string, fields map[string]string, t ...time.Time) {
	wrapper.AddDataWithContext(tags, fields, nil, t...)
}

func (wrapper *MetricWrapperV1) AddDataArray(tags map[string]string,
	columns []string,
	values []string,
	t ...time.Time) {
	wrapper.AddDataArrayWithContext(tags, columns, values, nil, t...)
}

func (wrapper *MetricWrapperV1) AddRawLog(log *protocol.Log) {
	wrapper.AddRawLogWithContext(log, nil)
}

func (wrapper *MetricWrapperV1) AddDataWithContext(tags map[string]string, fields map[string]string, ctx map[string]interface{}, t ...time.Time) {
	var logTime time.Time
	if len(t) == 0 {
		logTime = time.Now()
	} else {
		logTime = t[0]
	}
	// need push to native processor
	if wrapper.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEvent(logTime, wrapper.Config.GlobalConfig.EnableTimestampNanosecond, fields)
		wrapper.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Tags: tags, Context: ctx}
		wrapper.outEventsTotal.Add(1)
		wrapper.outEventGroupsTotal.Add(1)
		wrapper.outSizeBytes.Add(int64(logEvent.Size()))
		return
	}
	slsLog, _ := helper.CreateLog(logTime, len(t) != 0, wrapper.Tags, tags, fields)
	wrapper.outEventsTotal.Add(1)
	wrapper.outEventGroupsTotal.Add(1)
	wrapper.outSizeBytes.Add(int64(slsLog.Size()))
	wrapper.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
}

func (wrapper *MetricWrapperV1) AddDataArrayWithContext(tags map[string]string,
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
	if wrapper.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEventByArray(logTime, wrapper.Config.GlobalConfig.EnableTimestampNanosecond, columns, values)
		wrapper.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Tags: tags, Context: ctx}
		wrapper.outEventsTotal.Add(1)
		wrapper.outEventGroupsTotal.Add(1)
		wrapper.outSizeBytes.Add(int64(logEvent.Size()))
		return
	}
	slsLog, _ := helper.CreateLogByArray(logTime, len(t) != 0, wrapper.Tags, tags, columns, values)
	wrapper.outEventsTotal.Add(1)
	wrapper.outEventGroupsTotal.Add(1)
	wrapper.outSizeBytes.Add(int64(slsLog.Size()))
	wrapper.LogsChan <- &pipeline.LogWithContext{Log: slsLog, Context: ctx}
}

func (wrapper *MetricWrapperV1) AddRawLogWithContext(log *protocol.Log, ctx map[string]interface{}) {
	wrapper.outEventsTotal.Add(1)
	wrapper.outEventGroupsTotal.Add(1)
	wrapper.outSizeBytes.Add(int64(log.Size()))
	if wrapper.Config.GlobalConfig.GoInputToNativeProcessor {
		logEvent, _ := helper.CreateLogEventByLegacyRawLog(log)
		wrapper.LogsCachedChan <- &pipeline.LogEventWithContext{LogEvent: logEvent, Context: ctx}
		return
	}
	wrapper.LogsChan <- &pipeline.LogWithContext{Log: log, Context: ctx}
}

func (wrapper *MetricWrapperV1) RunPushNativeProcessQueueInternal() {
	wrapper.LogsCachedChan = make(chan *pipeline.LogEventWithContext, 10)
	wrapper.ShutdownCachedChan = make(chan struct{})
	wrapper.eventCached = make([]*protocol.LogEvent, 0, wrapper.MaxCachedSize+10)
	wrapper.tagCached = make([]map[string]string, 0, wrapper.MaxCachedSize+10)
	wrapper.ctxCached = make([]map[string]interface{}, 0, wrapper.MaxCachedSize+10)
	wrapper.timer = time.NewTimer(wrapper.PushNativeTimeout)
	go wrapper.runPushNativeProcessQueueInternal()
}

func (wrapper *MetricWrapperV1) StopPushNativeProcessQueueInternal() {
	wrapper.ShutdownCachedChan <- struct{}{}
}

func (wrapper *MetricWrapperV1) runPushNativeProcessQueueInternal() {
	var event *pipeline.LogEventWithContext
	isValidToPushNativeProcessQueue := true

	defer wrapper.timer.Stop()
	defer close(wrapper.LogsCachedChan)
	defer close(wrapper.ShutdownCachedChan)

	for {
		if isValidToPushNativeProcessQueue {
			select {
			case <-wrapper.timer.C:
				isValidToPushNativeProcessQueue = wrapper.pushNativeProcessQueue(5)
				wrapper.timer.Reset(wrapper.PushNativeTimeout)
			case event = <-wrapper.LogsCachedChan:
				wrapper.eventCached = append(wrapper.eventCached, event.LogEvent)
				wrapper.tagCached = append(wrapper.tagCached, event.Tags)
				wrapper.ctxCached = append(wrapper.ctxCached, event.Context)
				if len(wrapper.eventCached) >= wrapper.MaxCachedSize {
					isValidToPushNativeProcessQueue = wrapper.pushNativeProcessQueue(5)
					if !wrapper.timer.Stop() {
						<-wrapper.timer.C
					}
					wrapper.timer.Reset(wrapper.PushNativeTimeout)
				}
			case <-wrapper.ShutdownCachedChan:
				for event = range wrapper.LogsCachedChan {
					wrapper.eventCached = append(wrapper.eventCached, event.LogEvent)
					wrapper.tagCached = append(wrapper.tagCached, event.Tags)
					wrapper.ctxCached = append(wrapper.ctxCached, event.Context)
				}
				endTime := time.Now().Add(time.Duration(30) * time.Second)
				for {
					if time.Now().After(endTime) || (len(wrapper.eventCached) == 0 && len(wrapper.pbBuffer) == 0) {
						return
					}
					wrapper.pushNativeProcessQueue(1)
					time.Sleep(time.Duration(10) * time.Millisecond)
				}
			}
		} else {
			select {
			case <-wrapper.timer.C:
				isValidToPushNativeProcessQueue = wrapper.pushNativeProcessQueue(5)
				wrapper.timer.Reset(wrapper.PushNativeTimeout)
			case <-wrapper.ShutdownCachedChan:
				for event = range wrapper.LogsCachedChan {
					wrapper.eventCached = append(wrapper.eventCached, event.LogEvent)
					wrapper.tagCached = append(wrapper.tagCached, event.Tags)
					wrapper.ctxCached = append(wrapper.ctxCached, event.Context)
				}
				endTime := time.Now().Add(time.Duration(30) * time.Second)
				for {
					if time.Now().After(endTime) || (len(wrapper.eventCached) == 0 && len(wrapper.pbBuffer) == 0) {
						return
					}
					wrapper.pushNativeProcessQueue(1)
					time.Sleep(time.Duration(10) * time.Millisecond)
				}
			}
		}
	}

}

func (wrapper *MetricWrapperV1) pushNativeProcessQueue(retryCnt int) bool {
	if len(wrapper.pbBuffer) == 0 && len(wrapper.eventCached) == 0 {
		return true
	}

	// create pipelineEventGroup and marshal to pbBuffer
	if len(wrapper.pbBuffer) == 0 {
		// find logs with same tag and ctx as much as possible
		tag := wrapper.tagCached[0]
		ctx := wrapper.ctxCached[0]
		pushSize := 1
		for ; pushSize < len(wrapper.eventCached); pushSize++ {
			same := true
			for k, v := range wrapper.tagCached[pushSize] {
				if tag[k] != v {
					same = false
					break
				}
			}
			if !same || !reflect.DeepEqual(ctx, wrapper.ctxCached[pushSize]) {
				break
			}
		}
		group, _ := helper.CreatePipelineEventGroupByLegacyRawLog(wrapper.eventCached[:pushSize], wrapper.Tags, tag, ctx)
		pbSize := group.Size()
		if cap(wrapper.pbBuffer) < pbSize {
			if cap(wrapper.pbBuffer)*2 >= pbSize {
				wrapper.pbBuffer = make([]byte, cap(wrapper.pbBuffer)*2)
			} else {
				wrapper.pbBuffer = make([]byte, pbSize)
			}
		}
		n, _ := group.MarshalTo(wrapper.pbBuffer)
		wrapper.pbBuffer = wrapper.pbBuffer[:n]

		// clear eventCached, tagCached and ctxCached
		for i := 0; i < pushSize; i++ {
			helper.LogEventPool.Put(wrapper.eventCached[i])
		}
		for i := pushSize; i < len(wrapper.eventCached); i++ {
			wrapper.eventCached[i-pushSize] = wrapper.eventCached[i]
			wrapper.tagCached[i-pushSize] = wrapper.tagCached[i]
			wrapper.ctxCached[i-pushSize] = wrapper.ctxCached[i]
		}
		wrapper.eventCached = wrapper.eventCached[:len(wrapper.eventCached)-pushSize]
		wrapper.tagCached = wrapper.tagCached[:len(wrapper.tagCached)-pushSize]
		wrapper.ctxCached = wrapper.ctxCached[:len(wrapper.ctxCached)-pushSize]
	}

	// try to pushNativeProcessQueue
	rst := -1
	switch wrapper.Input.GetMode() {
	case pipeline.PUSH:
		i := 0
		for {
			if retryCnt > 0 && i >= retryCnt {
				break
			}
			if logtail.IsValidToProcess(wrapper.Config.ConfigName) {
				if rst = logtail.PushQueue(wrapper.Config.ConfigName, wrapper.pbBuffer); rst == 0 {
					break
				}
			}
			time.Sleep(time.Duration(10) * time.Millisecond)
		}
	case pipeline.PULL:
		logtail.PushQueue(wrapper.Config.ConfigName, wrapper.pbBuffer)
		rst = 0
	default:
	}

	// clear pbBuffer
	if rst != 0 {
		return false
	}
	wrapper.pbBuffer = wrapper.pbBuffer[:0]
	return true
}
