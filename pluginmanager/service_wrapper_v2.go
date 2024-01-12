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
	"github.com/alibaba/ilogtail/pkg/pipeline"

	"time"
)

type ServiceWrapperV2 struct {
	pipeline.PluginContext
	Input    pipeline.ServiceInputV2
	Config   *LogstoreConfig
	Tags     map[string]string
	Interval time.Duration

	LogsChan chan *pipeline.LogWithContext
}

func (p *ServiceWrapperV2) Init(name string, pluginID string, pluginNodeID string, pluginChildNodeID string) error {
	labels := pipeline.GetCommonLabels(p.Config.Context, name, pluginID, pluginNodeID, pluginChildNodeID)
	p.MetricRecord = p.Config.Context.RegisterMetricRecord(labels)

	p.Config.Context.SetMetricRecord(p.MetricRecord)
	_, err := p.Input.Init(p.Config.Context)
	return err
}

func (p *ServiceWrapperV2) StartService(pipelineContext pipeline.PipelineContext) error {
	return p.Input.StartService(pipelineContext)
}
