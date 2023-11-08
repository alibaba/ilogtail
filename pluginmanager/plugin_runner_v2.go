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

	MetricPlugins     []pipeline.MetricInputV2
	ServicePlugins    []pipeline.ServiceInputV2
	ProcessorPlugins  []pipeline.ProcessorV2
	AggregatorPlugins []pipeline.AggregatorV2
	FlusherPlugins    []pipeline.FlusherV2
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
	p.MetricPlugins = make([]pipeline.MetricInputV2, 0)
	p.ServicePlugins = make([]pipeline.ServiceInputV2, 0)
	p.ProcessorPlugins = make([]pipeline.ProcessorV2, 0)
	p.AggregatorPlugins = make([]pipeline.AggregatorV2, 0)
	p.FlusherPlugins = make([]pipeline.FlusherV2, 0)
	p.ExtensionPlugins = make(map[string]pipeline.Extension, 0)
	p.InputPipeContext = pipeline.NewObservePipelineConext(inputQueueSize)
	p.ProcessPipeContext = pipeline.NewGroupedPipelineConext()
	p.AggregatePipeContext = pipeline.NewObservePipelineConext(flushQueueSize)
	p.FlushPipeContext = pipeline.NewNoopPipelineConext()
	p.FlushOutStore.Write(p.AggregatePipeContext.Collector().Observe())
	return nil
}

func (p *pluginv2Runner) Initialized() error {
	if len(p.AggregatorPlugins) == 0 {
		logger.Debug(p.LogstoreConfig.Context.GetRuntimeContext(), "add default aggregator")
		if err := loadAggregator("aggregator_default", p.LogstoreConfig, nil); err != nil {
			return err
		}
	}
	// TODO Implement default flusher v2
	return nil
}

func (p *pluginv2Runner) AddPlugin(pluginName string, category pluginCategory, plugin interface{}, config map[string]interface{}) error {
	switch category {
	case pluginMetricInput:
		if metric, ok := plugin.(pipeline.MetricInputV2); ok {
			return p.addMetricInput(metric, config["interval"].(int))
		}
	case pluginServiceInput:
		if service, ok := plugin.(pipeline.ServiceInputV2); ok {
			return p.addServiceInput(service)
		}
	case pluginProcessor:
		if processor, ok := plugin.(pipeline.ProcessorV2); ok {
			return p.addProcessor(processor, config["priority"].(int))
		}
	case pluginAggregator:
		if aggregator, ok := plugin.(pipeline.AggregatorV2); ok {
			return p.addAggregator(aggregator)
		}
	case pluginFlusher:
		if flusher, ok := plugin.(pipeline.FlusherV2); ok {
			return p.addFlusher(flusher)
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

func (p *pluginv2Runner) addMetricInput(input pipeline.MetricInputV2, interval int) error {
	p.MetricPlugins = append(p.MetricPlugins, input)
	p.TimerRunner = append(p.TimerRunner, &timerRunner{
		state:         input,
		interval:      time.Duration(interval) * time.Millisecond,
		context:       p.LogstoreConfig.Context,
		latencyMetric: p.LogstoreConfig.Statistics.CollecLatencytMetric,
	})
	return nil
}

func (p *pluginv2Runner) addServiceInput(input pipeline.ServiceInputV2) error {
	p.ServicePlugins = append(p.ServicePlugins, input)
	return nil
}

func (p *pluginv2Runner) addProcessor(processor pipeline.ProcessorV2, _ int) error {
	p.ProcessorPlugins = append(p.ProcessorPlugins, processor)
	return nil
}

func (p *pluginv2Runner) addAggregator(aggregator pipeline.AggregatorV2) error {
	p.AggregatorPlugins = append(p.AggregatorPlugins, aggregator)
	interval, err := aggregator.Init(p.LogstoreConfig.Context, &AggregatorWrapper{})
	if err != nil {
		logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "AGGREGATOR_INIT_ERROR", "Aggregator failed to initialize", aggregator.Description(), "error", err)
		return err
	}
	if interval == 0 {
		interval = p.LogstoreConfig.GlobalConfig.AggregatIntervalMs
	}
	p.TimerRunner = append(p.TimerRunner, &timerRunner{
		state:         aggregator,
		interval:      time.Millisecond * time.Duration(interval),
		context:       p.LogstoreConfig.Context,
		latencyMetric: p.LogstoreConfig.Statistics.CollecLatencytMetric,
	})
	return nil
}

func (p *pluginv2Runner) addFlusher(flusher pipeline.FlusherV2) error {
	p.FlusherPlugins = append(p.FlusherPlugins, flusher)
	return nil
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
			defer panicRecover(service.Description())
			if err := service.StartService(p.InputPipeContext); err != nil {
				logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "PLUGIN_ALARM", "start service error, err", err)
			}
			logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "service done", service.Description())
		})
	}
}

func (p *pluginv2Runner) runMetricInput(control *pipeline.AsyncControl) {
	for _, t := range p.TimerRunner {
		if plugin, ok := t.state.(pipeline.MetricInputV2); ok {
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
					if !flusher.IsReady(p.LogstoreConfig.ProjectName,
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
		flusher.SetUrgent(exit)
	}
	for _, serviceInput := range p.ServicePlugins {
		_ = serviceInput.Stop()
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
		logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flushout group events, count", p.FlushOutStore.Len())
		rst := flushOutStore(p.LogstoreConfig, p.FlushOutStore, p.FlusherPlugins, func(lc *LogstoreConfig, pf pipeline.FlusherV2, store *FlushOutStore[models.PipelineGroupEvents]) error {
			return pf.Export(store.Get(), p.FlushPipeContext)
		})
		logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flushout group events, result", rst)
	}
	for idx, flusher := range p.FlusherPlugins {
		if err := flusher.Stop(); err != nil {
			logger.Warningf(p.LogstoreConfig.Context.GetRuntimeContext(), "STOP_FLUSHER_ALARM",
				"Failed to stop %vth flusher (description: %v): %v",
				idx, flusher.Description(), err)
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

// TODO: Design the ReceiveRawLogV2, which is passed in a PipelineGroupEvents not pipeline.LogWithContext, and tags should be added in the PipelineGroupEvents.
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
