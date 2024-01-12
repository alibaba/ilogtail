// Copyright 2022 iLogtail Authors
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
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/util"
)

var (
	tagPrefix     = "__tag__:"
	fileOffsetKey = tagPrefix + "__file_offset__"
	contentKey    = "content"
)

type pluginv2Runner struct {
	// pipeline v2 fields
	InputPipeContext     pipeline.PipelineContext
	ProcessPipeContext   pipeline.PipelineContext
	AggregatePipeContext pipeline.PipelineContext
	FlushPipeContext     pipeline.PipelineContext
	InputControl         *pipeline.AsyncControl
	ProcessControl       *pipeline.AsyncControl
	AggregateControl     *pipeline.AsyncControl
	FlushControl         *pipeline.AsyncControl

	MetricPlugins     []*MetricWrapperV2
	ServicePlugins    []*ServiceWrapperV2
	ProcessorPlugins  []*ProcessorWrapperV2
	AggregatorPlugins []*AggregatorWrapperV2
	FlusherPlugins    []*FlusherWrapperV2
	ExtensionPlugins  map[string]pipeline.Extension
	TimerRunner       []*timerRunner

	FlushOutStore  *FlushOutStore[models.PipelineGroupEvents]
	LogstoreConfig *LogstoreConfig
}

func (p *pluginv2Runner) Init(inputQueueSize int, flushQueueSize int) error {
	p.InputControl = pipeline.NewAsyncControl()
	p.ProcessControl = pipeline.NewAsyncControl()
	p.AggregateControl = pipeline.NewAsyncControl()
	p.FlushControl = pipeline.NewAsyncControl()
	p.MetricPlugins = make([]*MetricWrapperV2, 0)
	p.ServicePlugins = make([]*ServiceWrapperV2, 0)
	p.ProcessorPlugins = make([]*ProcessorWrapperV2, 0)
	p.AggregatorPlugins = make([]*AggregatorWrapperV2, 0)
	p.FlusherPlugins = make([]*FlusherWrapperV2, 0)
	p.ExtensionPlugins = make(map[string]pipeline.Extension, 0)
	p.InputPipeContext = helper.NewObservePipelineConext(inputQueueSize)
	p.ProcessPipeContext = helper.NewGroupedPipelineConext()
	p.AggregatePipeContext = helper.NewObservePipelineConext(flushQueueSize)
	p.FlushPipeContext = helper.NewNoopPipelineConext()
	p.FlushOutStore.Write(p.AggregatePipeContext.Collector().Observe())
	return nil
}

func (p *pluginv2Runner) AddDefaultAggregatorIfEmpty() error {
	if len(p.AggregatorPlugins) == 0 {
		pluginID := p.LogstoreConfig.genEmbeddedPluginID()
		pluginNodeID, pluginChildNodeID := p.LogstoreConfig.genNodeID(false)
		logger.Debug(p.LogstoreConfig.Context.GetRuntimeContext(), "add default aggregator")
		if err := loadAggregator("aggregator_default", pluginID, pluginNodeID, pluginChildNodeID, p.LogstoreConfig, nil); err != nil {
			return err
		}
	}
	return nil
}

func (p *pluginv2Runner) AddDefaultFlusherIfEmpty() error {
	return nil
}

func (p *pluginv2Runner) AddPlugin(pluginName string, pluginID string, pluginNodeID string, pluginChildNodeID string, category pluginCategory, plugin interface{}, config map[string]interface{}) error {
	switch category {
	case pluginMetricInput:
		if metric, ok := plugin.(pipeline.MetricInputV2); ok {
			return p.addMetricInput(pluginName, pluginID, pluginNodeID, pluginChildNodeID, metric, config["interval"].(int))
		}
	case pluginServiceInput:
		if service, ok := plugin.(pipeline.ServiceInputV2); ok {
			return p.addServiceInput(pluginName, pluginID, pluginNodeID, pluginChildNodeID, service)
		}
	case pluginProcessor:
		if processor, ok := plugin.(pipeline.ProcessorV2); ok {
			return p.addProcessor(pluginName, pluginID, pluginNodeID, pluginChildNodeID, processor, config["priority"].(int))
		}
	case pluginAggregator:
		if aggregator, ok := plugin.(pipeline.AggregatorV2); ok {
			return p.addAggregator(pluginName, pluginID, pluginNodeID, pluginChildNodeID, aggregator)
		}
	case pluginFlusher:
		if flusher, ok := plugin.(pipeline.FlusherV2); ok {
			return p.addFlusher(pluginName, pluginID, pluginNodeID, pluginChildNodeID, flusher)
		}
	case pluginExtension:
		if extension, ok := plugin.(pipeline.Extension); ok {
			return p.addExtension(pluginName, extension)
		}
	default:
		return pluginCategoryUndefinedError(category)
	}
	return pluginUnImplementError(category, v2, pluginName)
}

func (p *pluginv2Runner) GetExtension(name string) (pipeline.Extension, bool) {
	extension, ok := p.ExtensionPlugins[name]
	return extension, ok
}

func (p *pluginv2Runner) Run() {
	p.runFlusher()
	p.runAggregator()
	p.runProcessor()
	p.runInput()
}

func (p *pluginv2Runner) RunPlugins(category pluginCategory, control *pipeline.AsyncControl) {
	switch category {
	case pluginMetricInput:
		p.runMetricInput(control)
	default:
	}
}

func (p *pluginv2Runner) addMetricInput(name string, pluginID string, pluginNodeID string, pluginChildNodeID string, input pipeline.MetricInputV2, inputInterval int) error {
	var wrapper MetricWrapperV2
	wrapper.Config = p.LogstoreConfig
	wrapper.Input = input
	err := wrapper.Init(name, pluginID, pluginNodeID, pluginChildNodeID, inputInterval)
	if err != nil {
		return err
	}
	p.MetricPlugins = append(p.MetricPlugins, &wrapper)
	p.TimerRunner = append(p.TimerRunner, &timerRunner{
		state:         input,
		interval:      wrapper.Interval * time.Millisecond,
		context:       p.LogstoreConfig.Context,
		latencyMetric: p.LogstoreConfig.Statistics.CollecLatencytMetric,
	})
	return err
}

func (p *pluginv2Runner) addServiceInput(name string, pluginID string, pluginNodeID string, pluginChildNodeID string, input pipeline.ServiceInputV2) error {
	var wrapper ServiceWrapperV2
	wrapper.Config = p.LogstoreConfig
	wrapper.Input = input
	p.ServicePlugins = append(p.ServicePlugins, &wrapper)
	err := wrapper.Init(name, pluginID, pluginNodeID, pluginChildNodeID)
	return err
}

func (p *pluginv2Runner) addProcessor(name string, pluginID string, pluginNodeID string, pluginChildNodeID string, processor pipeline.ProcessorV2, _ int) error {
	var wrapper ProcessorWrapperV2
	wrapper.Config = p.LogstoreConfig
	wrapper.Processor = processor
	p.ProcessorPlugins = append(p.ProcessorPlugins, &wrapper)
	return wrapper.Init(name, pluginID, pluginNodeID, pluginChildNodeID)
}

func (p *pluginv2Runner) addAggregator(name string, pluginID string, pluginNodeID string, pluginChildNodeID string, aggregator pipeline.AggregatorV2) error {
	var wrapper AggregatorWrapperV2
	wrapper.Config = p.LogstoreConfig
	wrapper.Aggregator = aggregator
	err := wrapper.Init(name, pluginID, pluginNodeID, pluginChildNodeID)
	if err != nil {
		logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "AGGREGATOR_INIT_ERROR", "Aggregator failed to initialize", aggregator.Description(), "error", err)
		return err
	}
	p.AggregatorPlugins = append(p.AggregatorPlugins, &wrapper)
	p.TimerRunner = append(p.TimerRunner, &timerRunner{
		state:         aggregator,
		interval:      time.Millisecond * wrapper.Interval,
		context:       p.LogstoreConfig.Context,
		latencyMetric: p.LogstoreConfig.Statistics.CollecLatencytMetric,
	})
	return nil
}

func (p *pluginv2Runner) addFlusher(name string, pluginID string, pluginNodeID string, pluginChildNodeID string, flusher pipeline.FlusherV2) error {
	var wrapper FlusherWrapperV2
	wrapper.Config = p.LogstoreConfig
	wrapper.Flusher = flusher
	wrapper.Interval = time.Millisecond * time.Duration(p.LogstoreConfig.GlobalConfig.FlushIntervalMs)
	p.FlusherPlugins = append(p.FlusherPlugins, &wrapper)
	return wrapper.Init(name, pluginID, pluginNodeID, pluginChildNodeID, p.FlushPipeContext)
}

func (p *pluginv2Runner) addExtension(name string, extension pipeline.Extension) error {
	p.ExtensionPlugins[name] = extension
	return nil
}

func (p *pluginv2Runner) runInput() {
	p.InputControl.Reset()
	p.runMetricInput(p.InputControl)
	for _, input := range p.ServicePlugins {
		service := input
		p.InputControl.Run(func(c *pipeline.AsyncControl) {
			logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "start run service", service)
			defer panicRecover(service.Input.Description())
			if err := service.StartService(p.InputPipeContext); err != nil {
				logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "PLUGIN_ALARM", "start service error, err", err)
			}
			logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "service done", service.Input.Description())
		})
	}
}

func (p *pluginv2Runner) runMetricInput(control *pipeline.AsyncControl) {
	for _, t := range p.TimerRunner {
		if plugin, ok := t.state.(MetricWrapperV2); ok {
			metric := plugin
			timer := t
			control.Run(func(cc *pipeline.AsyncControl) {
				timer.Run(func(state interface{}) error {
					return metric.Read(p.InputPipeContext)
				}, cc)
			})
		}
	}
}

func (p *pluginv2Runner) runProcessor() {
	p.ProcessControl.Reset()
	p.ProcessControl.Run(p.runProcessorInternal)
}

func (p *pluginv2Runner) runProcessorInternal(cc *pipeline.AsyncControl) {
	defer panicRecover(p.LogstoreConfig.ConfigName)
	pipeContext := p.ProcessPipeContext
	pipeChan := p.InputPipeContext.Collector().Observe()
	for {
		select {
		case <-cc.CancelToken():
			if len(pipeChan) == 0 {
				return
			}
		case group := <-pipeChan:
			p.LogstoreConfig.Statistics.RawLogMetric.Add(int64(len(group.Events)))
			pipeEvents := []*models.PipelineGroupEvents{group}
			for _, processor := range p.ProcessorPlugins {
				for _, in := range pipeEvents {
					processor.Process(in, pipeContext)
				}
				pipeEvents = pipeContext.Collector().ToArray()
				if len(pipeEvents) == 0 {
					break
				}
			}
			if len(pipeEvents) == 0 {
				break
			}
			for _, aggregator := range p.AggregatorPlugins {
				for _, pipeEvent := range pipeEvents {
					if len(pipeEvent.Events) == 0 {
						continue
					}
					p.LogstoreConfig.Statistics.SplitLogMetric.Add(int64(len(pipeEvent.Events)))
					for tryCount := 1; true; tryCount++ {
						err := aggregator.Record(pipeEvent, p.AggregatePipeContext)
						if err == nil {
							break
						}
						// wait until shutdown is active
						if tryCount%100 == 0 {
							logger.Warning(p.LogstoreConfig.Context.GetRuntimeContext(), "AGGREGATOR_ADD_ALARM", "error", err)
						}
						time.Sleep(time.Millisecond * 10)
					}
				}
			}
		}
	}
}

func (p *pluginv2Runner) runAggregator() {
	p.AggregateControl.Reset()
	for _, t := range p.TimerRunner {
		if plugin, ok := t.state.(pipeline.AggregatorV2); ok {
			aggregator := plugin
			timer := t
			p.AggregateControl.Run(func(cc *pipeline.AsyncControl) {
				timer.Run(func(state interface{}) error {
					return aggregator.GetResult(p.AggregatePipeContext)
				}, cc)
			})
		}
	}
}

func (p *pluginv2Runner) runFlusher() {
	p.FlushControl.Reset()
	p.FlushControl.Run(p.runFlusherInternal)
}

func (p *pluginv2Runner) runFlusherInternal(cc *pipeline.AsyncControl) {
	defer panicRecover(p.LogstoreConfig.ConfigName)
	pipeChan := p.AggregatePipeContext.Collector().Observe()
	for {
		select {
		case <-cc.CancelToken():
			if len(pipeChan) == 0 {
				return
			}
		case <-p.LogstoreConfig.pauseChan:
			p.LogstoreConfig.waitForResume()

		case event := <-pipeChan:
			if event == nil {
				continue
			}

			// Check pause status if config is still alive, if paused, wait for resume.
			select {
			case <-p.LogstoreConfig.pauseChan:
				p.LogstoreConfig.waitForResume()
			default:
			}

			dataSize := len(pipeChan) + 1
			data := make([]*models.PipelineGroupEvents, dataSize)
			data[0] = event
			for i := 1; i < dataSize; i++ {
				data[i] = <-pipeChan
			}
			p.LogstoreConfig.Statistics.FlushLogGroupMetric.Add(int64(len(data)))

			// Add tags for each non-empty LogGroup, includes: default hostname tag,
			// env tags and global tags in config.
			for _, item := range data {
				if len(item.Events) == 0 {
					continue
				}
				p.LogstoreConfig.Statistics.FlushLogMetric.Add(int64(len(item.Events)))
				item.Group.GetTags().Merge(loadAdditionalTags(p.LogstoreConfig.GlobalConfig))
			}

			// Flush LogGroups to all flushers.
			// Note: multiple flushers is unrecommended, because all flushers will
			//   be blocked if one of them is unready.
			for {
				allReady := true
				for _, flusher := range p.FlusherPlugins {
					if !flusher.Flusher.IsReady(p.LogstoreConfig.ProjectName,
						p.LogstoreConfig.LogstoreName, p.LogstoreConfig.LogstoreKey) {
						allReady = false
						break
					}
				}
				if allReady {
					for _, flusher := range p.FlusherPlugins {
						p.LogstoreConfig.Statistics.FlushReadyMetric.Add(1)
						p.LogstoreConfig.Statistics.FlushLatencyMetric.Begin()
						err := flusher.Export(data, p.FlushPipeContext)
						p.LogstoreConfig.Statistics.FlushLatencyMetric.End()
						if err != nil {
							logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "FLUSH_DATA_ALARM", "flush data error",
								p.LogstoreConfig.ProjectName, p.LogstoreConfig.LogstoreName, err)
						}
					}
					break
				}
				if !p.LogstoreConfig.FlushOutFlag {
					time.Sleep(time.Duration(10) * time.Millisecond)
					continue
				}

				// Config is stopping, move unflushed LogGroups to FlushOutLogGroups.
				logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "flush loggroup to slice, loggroup count", dataSize)
				p.FlushOutStore.Add(data...)
				break
			}
		}
	}
}

func (p *pluginv2Runner) Stop(exit bool) error {
	for _, flusher := range p.FlusherPlugins {
		flusher.Flusher.SetUrgent(exit)
	}
	for _, serviceInput := range p.ServicePlugins {
		_ = serviceInput.Input.Stop()
	}
	p.InputControl.WaitCancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "metric plugins stop", "done", "service plugins stop", "done")

	p.ProcessControl.WaitCancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "processor plugins stop", "done")

	p.AggregateControl.WaitCancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "aggregator plugins stop", "done")

	p.LogstoreConfig.FlushOutFlag = true
	p.FlushControl.WaitCancel()

	if exit && p.FlushOutStore.Len() > 0 {
		flushers := make([]pipeline.FlusherV2, len(p.FlusherPlugins))
		for idx, flusher := range p.FlusherPlugins {
			flushers[idx] = flusher.Flusher
		}
		logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flushout group events, count", p.FlushOutStore.Len())
		rst := flushOutStoreV2(p.LogstoreConfig, p.FlushOutStore, p.FlusherPlugins, func(lc *LogstoreConfig, pf *FlusherWrapperV2, store *FlushOutStore[models.PipelineGroupEvents]) error {
			return pf.Export(store.Get(), p.FlushPipeContext)
		})
		logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flushout group events, result", rst)
	}
	for idx, flusher := range p.FlusherPlugins {
		if err := flusher.Flusher.Stop(); err != nil {
			logger.Warningf(p.LogstoreConfig.Context.GetRuntimeContext(), "STOP_FLUSHER_ALARM",
				"Failed to stop %vth flusher (description: %v): %v",
				idx, flusher.Flusher.Description(), err)
		}
	}
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flusher plugins stop", "done")

	for _, extension := range p.ExtensionPlugins {
		err := extension.Stop()
		if err != nil {
			logger.Warningf(p.LogstoreConfig.Context.GetRuntimeContext(), "STOP_EXTENSION_ALARM",
				"failed to stop extension (description: %v): %v", extension.Description(), err)
		}
	}
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "extension plugins stop", "done")

	return nil
}

func (p *pluginv2Runner) ReceiveRawLog(in *pipeline.LogWithContext) {
	md := models.NewMetadata()
	if in.Context != nil {
		for k, v := range in.Context {
			md.Add(k, v.(string))
		}
	}
	log := &models.Log{}
	log.Tags = models.NewTags()
	for i, content := range in.Log.Contents {
		switch {
		case content.Key == contentKey || i == 0:
			log.SetBody(util.ZeroCopyStringToBytes(content.Value))
		case content.Key == fileOffsetKey:
			if offset, err := strconv.ParseInt(content.Value, 10, 64); err == nil {
				log.Offset = uint64(offset)
			}
		case strings.Contains(content.Key, tagPrefix):
			log.Tags.Add(content.Key[len(tagPrefix):], content.Value)
		default:
			log.Tags.Add(content.Key, content.Value)
		}
	}
	if in.Log.Time != 0 {
		log.Timestamp = uint64(time.Second * time.Duration(in.Log.Time))
		if in.Log.TimeNs != nil {
			log.Timestamp += uint64(*in.Log.TimeNs)
		}
	} else {
		log.Timestamp = uint64(time.Now().UnixNano())
	}
	group := models.NewGroup(md, models.NewTags())
	p.InputPipeContext.Collector().Collect(group, log)
}

func (p *pluginv2Runner) Merge(r PluginRunner) {
	if other, ok := r.(*pluginv2Runner); ok {
		p.FlushOutStore.Merge(other.FlushOutStore)
	}
}
