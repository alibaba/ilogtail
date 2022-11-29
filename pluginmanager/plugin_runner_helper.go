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
	"fmt"
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/util"
)

type timerRunner struct {
	interval      time.Duration
	context       ilogtail.Context
	latencyMetric ilogtail.LatencyMetric
	state         interface{}
}

func (p *timerRunner) Run(task func(state interface{}) error, cc *ilogtail.CancellationControl) {
	logger.Info(p.context.GetRuntimeContext(), "start run plugin", p.state)
	defer panicRecover(fmt.Sprint(p.state))
	for {
		exitFlag := util.RandomSleep(p.interval, 0.1, cc.CancelToken())
		p.latencyMetric.Begin()
		if err := task(p.state); err != nil {
			logger.Error(p.context.GetRuntimeContext(), "PLUGIN_RUN_ALARM", "Plugin timer run error", "error", err, "plugin", fmt.Sprint(p.state))
		}
		p.latencyMetric.End()
		if exitFlag {
			logger.Info(p.context.GetRuntimeContext(), "task stop", "done", "plugin", fmt.Sprint(p.state))
			return
		}
	}
}

func flushOutStore[T FlushData, F ilogtail.Flusher](lc *LogstoreConfig, store *FlushOutStore[T], flushers []F, flushFunc func(*LogstoreConfig, F, *FlushOutStore[T]) error) bool {
	for _, flusher := range flushers {
		for waitCount := 0; !flusher.IsReady(lc.ProjectName, lc.LogstoreName, lc.LogstoreKey); waitCount++ {
			if waitCount > maxFlushOutTime*100 {
				logger.Error(lc.Context.GetRuntimeContext(), "DROP_DATA_ALARM", "flush out data timeout, drop data", store.Len())
				return false
			}
			lc.Statistics.FlushReadyMetric.Add(0)
			time.Sleep(time.Duration(10) * time.Millisecond)
		}
		lc.Statistics.FlushReadyMetric.Add(1)
		lc.Statistics.FlushLatencyMetric.Begin()
		err := flushFunc(lc, flusher, store)
		if err != nil {
			logger.Error(lc.Context.GetRuntimeContext(), "FLUSH_DATA_ALARM", "flush data error", lc.ProjectName, lc.LogstoreName, err)
		}
		lc.Statistics.FlushLatencyMetric.End()
	}
	store.Reset()
	return true
}

func loadAdditionalTags(globalConfig *GlobalConfig) models.Tags {
	tags := models.NewTagsWithKeyValues("__hostname__", util.GetHostName())
	for i := 0; i < len(helper.EnvTags); i += 2 {
		tags.Add(helper.EnvTags[i], helper.EnvTags[i+1])
	}
	for key, value := range globalConfig.Tags {
		tags.Add(key, value)
	}
	return tags
}

func GetFlushStoreLen(runner PluginRunner) int {
	if r, ok := runner.(*pluginv1Runner); ok {
		return r.FlushOutStore.Len()
	}
	if r, ok := runner.(*pluginv2Runner); ok {
		return r.FlushOutStore.Len()
	}
	return 0
}

func GetFlushCancelToken(runner PluginRunner) <-chan struct{} {
	if r, ok := runner.(*pluginv1Runner); ok {
		return r.FlushControl.CancelToken()
	}
	if r, ok := runner.(*pluginv2Runner); ok {
		return r.FlushControl.CancelToken()
	}
	return make(<-chan struct{})
}

func GetConfigFluhsers(runner PluginRunner) []ilogtail.Flusher {
	flushers := make([]ilogtail.Flusher, 0)
	if r, ok := runner.(*pluginv1Runner); ok {
		for _, f := range r.FlusherPlugins {
			flushers = append(flushers, f.Flusher)
		}
	} else if r, ok := runner.(*pluginv2Runner); ok {
		for _, f := range r.FlusherPlugins {
			flushers = append(flushers, f)
		}
	}
	return flushers
}
