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

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type ProcessorWrapperV1 struct {
	ProcessorWrapper
	LogsChan  chan *pipeline.LogWithContext
	Processor pipeline.ProcessorV1
}

func (wrapper *ProcessorWrapperV1) Init(pluginMeta *pipeline.PluginMeta) error {
	wrapper.InitMetricRecord(pluginMeta)

	return wrapper.Processor.Init(wrapper.Config.Context)
}

func (wrapper *ProcessorWrapperV1) Process(logArray []*protocol.Log) []*protocol.Log {
	wrapper.procInRecordsTotal.Add(int64(len(logArray)))
	startTime := time.Now()
	result := wrapper.Processor.ProcessLogs(logArray)
	wrapper.procTimeMS.Add(time.Since(startTime).Milliseconds())
	wrapper.procOutRecordsTotal.Add(int64(len(result)))
	return result
}
