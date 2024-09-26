// Copyright 2024 iLogtail Authors
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
	"github.com/alibaba/ilogtail/pkg/protocol"

	"time"
)

type FlusherWrapperV1 struct {
	FlusherWrapper
	LogGroupsChan chan *protocol.LogGroup
	Flusher       pipeline.FlusherV1
}

func (wrapper *FlusherWrapperV1) Init(pluginMeta *pipeline.PluginMeta) error {
	wrapper.InitMetricRecord(pluginMeta)

	return wrapper.Flusher.Init(wrapper.Config.Context)
}

func (wrapper *FlusherWrapperV1) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return wrapper.Flusher.IsReady(projectName, logstoreName, logstoreKey)
}

func (wrapper *FlusherWrapperV1) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	startTime := time.Now()
	for _, logGroup := range logGroupList {
		wrapper.inEventsTotal.Add(int64(len(logGroup.Logs)))
		wrapper.inEventGroupsTotal.Add(1)
		wrapper.inSizeBytes.Add(int64(logGroup.Size()))
	}

	err := wrapper.Flusher.Flush(projectName, logstoreName, configName, logGroupList)

	wrapper.totalDelayTimeMs.Add(time.Since(startTime).Milliseconds())
	return err
}
