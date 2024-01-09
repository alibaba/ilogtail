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
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

// AggregatorWrapperV2 wrappers Aggregator.
// It implements LogGroupQueue interface, and is passed to associated Aggregator.
// Aggregator uses Add function to pass log groups to wrapper, and then wrapper
// passes log groups to associated LogstoreConfig through channel LogGroupsChan.
// In fact, LogGroupsChan == (associated) LogstoreConfig.LogGroupsChan.
type AggregatorWrapperV2 struct {
	pipeline.PluginContext

	Aggregator    pipeline.AggregatorV2
	Config        *LogstoreConfig
	LogGroupsChan chan *protocol.LogGroup
	Interval      time.Duration

	procInRecordsTotal  pipeline.CounterMetric
	procOutRecordsTotal pipeline.CounterMetric
	procTimeMS          pipeline.CounterMetric
}

func (p *AggregatorWrapperV2) Init(name string, pluginID string) error {
	labels := pipeline.GetCommonLabels(p.Config.Context, name, pluginID)
	p.MetricRecord = p.Config.Context.RegisterMetricRecord(labels)

	p.procInRecordsTotal = helper.NewCounterMetric("proc_in_records_total")
	p.procOutRecordsTotal = helper.NewCounterMetric("proc_out_records_total")
	p.procTimeMS = helper.NewCounterMetric("proc_time_ms")

	p.MetricRecord.RegisterCounterMetric(p.procInRecordsTotal)
	p.MetricRecord.RegisterCounterMetric(p.procOutRecordsTotal)
	p.MetricRecord.RegisterCounterMetric(p.procTimeMS)

	p.Config.Context.SetMetricRecord(p.MetricRecord)

	interval, err := p.Aggregator.Init(p.Config.Context, p)
	if err != nil {
		logger.Error(p.Config.Context.GetRuntimeContext(), "AGGREGATOR_INIT_ERROR", "Aggregator failed to initialize", p.Aggregator.Description(), "error", err)
		return err
	}
	if interval == 0 {
		interval = p.Config.GlobalConfig.AggregatIntervalMs
	}
	p.Interval = time.Millisecond * time.Duration(interval)
	return nil
}

func (p *AggregatorWrapperV2) Record(events *models.PipelineGroupEvents, context pipeline.PipelineContext) error {
	p.procInRecordsTotal.Add(int64(len(events.Events)))
	err := p.Aggregator.Record(events, context)
	if err == nil {
		p.procOutRecordsTotal.Add(int64(len(events.Events)))
	}
	return err
}

func (p *AggregatorWrapperV2) GetResult(context pipeline.PipelineContext) error {
	return p.Aggregator.GetResult(context)
}

func (p *AggregatorWrapperV2) Add(loggroup *protocol.LogGroup) error {
	return nil
}

func (p *AggregatorWrapperV2) AddWithWait(loggroup *protocol.LogGroup, duration time.Duration) error {
	return nil
}
