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
	"context"
	"fmt"
	"runtime"
	"runtime/debug"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/config"
	"github.com/alibaba/ilogtail/pkg/flags"
	"github.com/alibaba/ilogtail/pkg/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
)

// Following variables are exported so that tests of main package can reference them.
var LogtailConfig sync.Map
var ToStartLogtailConfig *LogstoreConfig
var LastUnsendBuffer map[string]PluginRunner
var ContainerConfig *LogstoreConfig

// Two built-in logtail configs to report statistics and alarm (from system and other logtail configs).
var StatisticsConfig *LogstoreConfig
var AlarmConfig *LogstoreConfig

var statisticsConfigJSON = `{
    "global": {
        "InputIntervalMs" :  60000,
        "AggregatIntervalMs": 1000,
        "FlushIntervalMs": 1000,
        "DefaultLogQueueSize": 4,
		"DefaultLogGroupQueueSize": 4,
		"Tags" : {
			"base_version" : "` + config.BaseVersion + `",
			"logtail_version" : "` + config.BaseVersion + `"
		}
	},
	"inputs" : [
		{
			"type" : "metric_statistics",
			"detail" : null
		}
	]
}`

var alarmConfigJSON = `{
    "global": {
        "InputIntervalMs" :  30000,
        "AggregatIntervalMs": 1000,
        "FlushIntervalMs": 1000,
        "DefaultLogQueueSize": 4,
		"DefaultLogGroupQueueSize": 4,
		"Tags" : {
			"base_version" : "` + config.BaseVersion + `",
			"logtail_version" : "` + config.BaseVersion + `"
		}
    },
	"inputs" : [
		{
			"type" : "metric_alarm",
			"detail" : null
		}
	]
}`

var containerConfigJSON = `{
    "global": {
        "InputIntervalMs" :  30000,
        "AggregatIntervalMs": 1000,
        "FlushIntervalMs": 1000,
        "DefaultLogQueueSize": 4,
		"DefaultLogGroupQueueSize": 4,
		"Tags" : {
			"base_version" : "` + config.BaseVersion + `",
			"logtail_version" : "` + config.BaseVersion + `"
		}
    },
	"inputs" : [
		{
			"type" : "metric_container",
			"detail" : null
		}
	]
}`

func panicRecover(pluginType string) {
	if err := recover(); err != nil {
		trace := make([]byte, 2048)
		runtime.Stack(trace, true)
		logger.Error(context.Background(), "PLUGIN_RUNTIME_ALARM", "plugin", pluginType, "panicked", err, "stack", string(trace))
	}
}

// Init initializes plugin manager.
func Init() (err error) {
	logger.Info(context.Background(), "init plugin, local env tags", helper.EnvTags)

	if err = CheckPointManager.Init(); err != nil {
		return
	}
	if StatisticsConfig, err = loadBuiltinConfig("statistics", "sls-admin", "logtail_plugin_profile",
		"shennong_log_profile", statisticsConfigJSON); err != nil {
		logger.Error(context.Background(), "LOAD_PLUGIN_ALARM", "load statistics config fail", err)
		return
	}
	if AlarmConfig, err = loadBuiltinConfig("alarm", "sls-admin", "logtail_alarm",
		"logtail_alarm", alarmConfigJSON); err != nil {
		logger.Error(context.Background(), "LOAD_PLUGIN_ALARM", "load alarm config fail", err)
		return
	}
	if ContainerConfig, err = loadBuiltinConfig("container", "sls-admin", "logtail_containers", "logtail_containers", containerConfigJSON); err != nil {
		logger.Error(context.Background(), "LOAD_PLUGIN_ALARM", "load container config fail", err)
		return
	}
	logger.Info(context.Background(), "loadBuiltinConfig container")
	return
}

// timeoutStop wrappers LogstoreConfig.Stop with timeout (5s by default).
// @return true if Stop returns before timeout, otherwise false.
func timeoutStop(config *LogstoreConfig, removingFlag bool) bool {
	done := make(chan int)
	go func() {
		logger.Info(config.Context.GetRuntimeContext(), "Stop config in goroutine", "begin")
		_ = config.Stop(removingFlag)
		close(done)
		logger.Info(config.Context.GetRuntimeContext(), "Stop config in goroutine", "end")
	}()
	select {
	case <-done:
		return true
	case <-time.After(30 * time.Second):
		return false
	}
}

// StopAll stops all config instance and checkpoint manager so that it is ready
// to quit.
// For user-defined config, timeoutStop is used to avoid hanging.
func StopAll(exitFlag, withInput bool) error {
	defer panicRecover("Run plugin")

	configNames := make([]string, 0)
	LogtailConfig.Range(func(key, value interface{}) bool {
		configNames = append(configNames, key.(string))
		return true
	})
	for _, configName := range configNames {
		if logstoreConfig, ok := GetLogtailConfig(configName); ok {
			matchFlag := false
			if withInput {
				if logstoreConfig.PluginRunner.IsWithInputPlugin() {
					matchFlag = true
				}
			} else {
				if !logstoreConfig.PluginRunner.IsWithInputPlugin() {
					matchFlag = true
				}
			}
			if matchFlag {
				logger.Info(logstoreConfig.Context.GetRuntimeContext(), "Stop config", configName)
				if hasStopped := timeoutStop(logstoreConfig, exitFlag); !hasStopped {
					// TODO: This alarm can not be sent to server in current alarm design.
					logger.Error(logstoreConfig.Context.GetRuntimeContext(), "CONFIG_STOP_TIMEOUT_ALARM",
						"timeout when stop config, goroutine might leak")
				}
				logger.Info(context.Background(), "Stop config all", configName)
				LogtailConfig.Delete(configName)
			}
		}
	}
	if exitFlag {
		if StatisticsConfig != nil {
			if *flags.ForceSelfCollect {
				logger.Info(context.Background(), "force collect the static metrics")
				control := pipeline.NewAsyncControl()
				StatisticsConfig.PluginRunner.RunPlugins(pluginMetricInput, control)
				control.WaitCancel()
			}
			_ = StatisticsConfig.Stop(exitFlag)
			StatisticsConfig = nil
		}
		if AlarmConfig != nil {
			if *flags.ForceSelfCollect {
				logger.Info(context.Background(), "force collect the alarm metrics")
				control := pipeline.NewAsyncControl()
				AlarmConfig.PluginRunner.RunPlugins(pluginMetricInput, control)
				control.WaitCancel()
			}
			_ = AlarmConfig.Stop(exitFlag)
			AlarmConfig = nil
		}
		if ContainerConfig != nil {
			if *flags.ForceSelfCollect {
				logger.Info(context.Background(), "force collect the container metrics")
				control := pipeline.NewAsyncControl()
				ContainerConfig.PluginRunner.RunPlugins(pluginMetricInput, control)
				control.WaitCancel()
			}
			_ = ContainerConfig.Stop(exitFlag)
			ContainerConfig = nil
		}
		CheckPointManager.HoldOn()
	}
	return nil
}

// Stop stop the given config.
func Stop(configName string, exitFlag bool) error {
	defer panicRecover("Run plugin")
	if object, exists := LogtailConfig.Load(configName); exists {
		if config, ok := object.(*LogstoreConfig); ok {
			if hasStopped := timeoutStop(config, exitFlag); !hasStopped {
				logger.Error(config.Context.GetRuntimeContext(), "CONFIG_STOP_TIMEOUT_ALARM",
					"timeout when stop config, goroutine might leak")
			}
			if !exitFlag {
				LastUnsendBuffer[configName] = config.PluginRunner
			}
			logger.Info(config.Context.GetRuntimeContext(), "Stop config now", configName)
			LogtailConfig.Delete(configName)
			return nil
		}
	}
	return fmt.Errorf("config not found: %s", configName)
}

// Start starts the given config.
func Start(configName string) error {
	defer panicRecover("Run plugin")
	if ToStartLogtailConfig == nil {
		return fmt.Errorf("no pipeline loaded for the config: %s", configName)
	}
	realConfigName := config.GetRealConfigName(configName)
	if ToStartLogtailConfig.ConfigName != realConfigName {
		// should never happen
		return fmt.Errorf("config unmatch with the loaded pipeline: given %s, expect %s", configName, ToStartLogtailConfig.ConfigName)
	}
	ToStartLogtailConfig.Start()
	LogtailConfig.Store(configName, ToStartLogtailConfig)
	ToStartLogtailConfig = nil
	return nil
}

func GetLogtailConfigSize() int {
	size := 0
	LogtailConfig.Range(func(key, value interface{}) bool {
		size++
		return true
	})
	return size
}

func GetLogtailConfig(key string) (*LogstoreConfig, bool) {
	if object, exists := LogtailConfig.Load(key); exists {
		if config, ok := object.(*LogstoreConfig); ok {
			return config, true
		}
	}
	return nil, false
}

func init() {
	go func() {
		for {
			// force gc every 3 minutes
			time.Sleep(time.Minute * 3)
			logger.Debug(context.Background(), "force gc done", time.Now())
			runtime.GC()
			logger.Debug(context.Background(), "force gc done", time.Now())
			debug.FreeOSMemory()
			logger.Debug(context.Background(), "free os memory done", time.Now())
			if logger.DebugFlag() {
				gcStat := debug.GCStats{}
				debug.ReadGCStats(&gcStat)
				logger.Debug(context.Background(), "gc stats", gcStat)
				memStat := runtime.MemStats{}
				runtime.ReadMemStats(&memStat)
				logger.Debug(context.Background(), "mem stats", memStat)
			}
		}
	}()
}
