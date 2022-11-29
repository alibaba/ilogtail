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
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
)

type pluginv2Runner struct {
	// pipeline v2 fields
	InputPipeContext     ilogtail.PipelineContext
	ProcessPipeContext   ilogtail.PipelineContext
	AggregatePipeContext ilogtail.PipelineContext
	FlushPipeContext     ilogtail.PipelineContext
	InputControl         *ilogtail.CancellationControl
	ProcessControl       *ilogtail.CancellationControl
	AggregateControl     *ilogtail.CancellationControl
	FlushControl         *ilogtail.CancellationControl

	MetricPlugins     []ilogtail.MetricInput2
	ServicePlugins    []ilogtail.ServiceInput2
	ProcessorPlugins  []ilogtail.Processor2
	AggregatorPlugins []ilogtail.Aggregator2
	FlusherPlugins    []ilogtail.Flusher2
	TimerRunner       []*timerRunner

	FlushOutStore  *FlushOutStore[models.PipelineGroupEvents]
	LogstoreConfig *LogstoreConfig
}

func (p *pluginv2Runner) Init(inputQueueSize int, flushQueueSize int) error {
	p.InputControl = ilogtail.NewCancellationControl()
	p.ProcessControl = ilogtail.NewCancellationControl()
	p.AggregateControl = ilogtail.NewCancellationControl()
	p.FlushControl = ilogtail.NewCancellationControl()
	p.MetricPlugins = make([]ilogtail.MetricInput2, 0)
	p.ServicePlugins = make([]ilogtail.ServiceInput2, 0)
	p.ProcessorPlugins = make([]ilogtail.Processor2, 0)
	p.AggregatorPlugins = make([]ilogtail.Aggregator2, 0)
	p.FlusherPlugins = make([]ilogtail.Flusher2, 0)
	p.InputPipeContext = ilogtail.NewObservePipelineConext(inputQueueSize)
	p.ProcessPipeContext = ilogtail.NewGroupedPipelineConext()
	p.AggregatePipeContext = ilogtail.NewObservePipelineConext(flushQueueSize)
	p.FlushPipeContext = ilogtail.NewVoidPipelineConext()
	p.FlushOutStore.Write(p.AggregatePipeContext.Collector().Observe())
	return nil
}

func (p *pluginv2Runner) AddMetricInput(input ilogtail.MetricInput, interval int) {
	p.MetricPlugins = append(p.MetricPlugins, input.(ilogtail.MetricInput2))
	p.TimerRunner = append(p.TimerRunner, &timerRunner{
		state:         input,
		interval:      time.Duration(interval) * time.Millisecond,
		context:       p.LogstoreConfig.Context,
		latencyMetric: p.LogstoreConfig.Statistics.CollecLatencytMetric,
	})
}

func (p *pluginv2Runner) AddServiceInput(input ilogtail.ServiceInput) {
	p.ServicePlugins = append(p.ServicePlugins, input.(ilogtail.ServiceInput2))
}

func (p *pluginv2Runner) AddProcessor(processor ilogtail.Processor, priority int) {
	p.ProcessorPlugins = append(p.ProcessorPlugins, processor.(ilogtail.Processor2))
}

func (p *pluginv2Runner) AddAggregator(aggregator ilogtail.Aggregator) {
	p.AggregatorPlugins = append(p.AggregatorPlugins, aggregator.(ilogtail.Aggregator2))
	interval, err := aggregator.Init(p.LogstoreConfig.Context, nil)
	if err != nil {
		logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "AGGREGATOR_INIT_ERROR", "Aggregator failed to initialize", aggregator.Description(), "error", err)
		return
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
}

func (p *pluginv2Runner) AddFlusher(flusher ilogtail.Flusher) {
	p.FlusherPlugins = append(p.FlusherPlugins, flusher.(ilogtail.Flusher2))
}

func (p *pluginv2Runner) RunInput() {
	p.InputControl.Reset()
	p.RunMetricInputOnce(p.InputControl)
	for _, input := range p.ServicePlugins {
		service := input
		p.InputControl.Run(func(c *ilogtail.CancellationControl) {
			logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "start run service", service)
			defer panicRecover(service.Description())
			if err := service.StartService(p.InputPipeContext, c); err != nil {
				logger.Error(p.LogstoreConfig.Context.GetRuntimeContext(), "PLUGIN_ALARM", "start service error, err", err)
			}
			logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "service done", service.Description())
		})
	}
}

func (p *pluginv2Runner) RunMetricInputOnce(control *ilogtail.CancellationControl) {
	for _, t := range p.TimerRunner {
		if plugin, ok := t.state.(ilogtail.MetricInput2); ok {
			metric := plugin
			timer := t
			control.Run(func(cc *ilogtail.CancellationControl) {
				timer.Run(func(state interface{}) error {
					return metric.Read(p.InputPipeContext)
				}, cc)
			})
		}
	}
}

func (p *pluginv2Runner) RunProcessor() {
	p.ProcessControl.Reset()
	p.ProcessControl.Run(p.runProcessorInternal)
}

func (p *pluginv2Runner) runProcessorInternal(cc *ilogtail.CancellationControl) {
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
				pipeEvents = pipeContext.Collector().Dump()
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

func (p *pluginv2Runner) RunAggregator() {
	p.AggregateControl.Reset()
	if len(p.AggregatorPlugins) == 0 {
		logger.Debug(p.LogstoreConfig.Context.GetRuntimeContext(), "add default aggregator")
		_ = loadAggregator("aggregator_default", p.LogstoreConfig, nil)
	}
	for _, t := range p.TimerRunner {
		if plugin, ok := t.state.(ilogtail.Aggregator2); ok {
			aggregator := plugin
			timer := t
			p.AggregateControl.Run(func(cc *ilogtail.CancellationControl) {
				timer.Run(func(state interface{}) error {
					return aggregator.GetResult(p.AggregatePipeContext)
				}, cc)
			})
		}
	}
}

func (p *pluginv2Runner) RunFlusher() {
	p.FlushControl.Reset()
	p.FlushControl.Run(p.runFlusherInternal)
}

func (p *pluginv2Runner) runFlusherInternal(cc *ilogtail.CancellationControl) {
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
				item.Group.Tags.Merge(loadAdditionalTags(p.LogstoreConfig.GlobalConfig))
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
	p.InputControl.Cancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "metric plugins stop", "done", "service plugins stop", "done")

	p.ProcessControl.Cancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "processor plugins stop", "done")

	p.AggregateControl.Cancel()
	logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "aggregator plugins stop", "done")

	p.LogstoreConfig.FlushOutFlag = true
	p.FlushControl.Cancel()

	if exit && p.FlushOutStore.Len() > 0 {
		logger.Info(p.LogstoreConfig.Context.GetRuntimeContext(), "Flushout group events, count", p.FlushOutStore.Len())
		rst := flushOutStore(p.LogstoreConfig, p.FlushOutStore, p.FlusherPlugins, func(lc *LogstoreConfig, pf ilogtail.Flusher2, store *FlushOutStore[models.PipelineGroupEvents]) error {
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
	return nil
}

func (p *pluginv2Runner) ReceiveRawLog(log *ilogtail.LogWithContext) {
	// TODO
}

func (p *pluginv2Runner) Merge(r PluginRunner) {
	if other, ok := r.(*pluginv2Runner); ok {
		p.FlushOutStore.Merge(other.FlushOutStore)
	}
}
