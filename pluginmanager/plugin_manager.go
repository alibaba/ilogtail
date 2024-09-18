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
var LogtailConfigLock sync.RWMutex
var LogtailConfig map[string]*LogstoreConfig

// Configs that are inited and will be started.
// One config may have multiple Go pipelines, such as ContainerInfo (with input) and static file (without input).
var ToStartLogtailConfigWithInput *LogstoreConfig
var ToStartLogtailConfigWithoutInput *LogstoreConfig
var ContainerConfig *LogstoreConfig

// Configs that were disabled because of slow or hang config.
var DisabledLogtailConfigLock sync.RWMutex
var DisabledLogtailConfig = make(map[string]*LogstoreConfig)

var LastUnsendBuffer map[string]PluginRunner

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
	DisabledLogtailConfigLock.Lock()
	DisabledLogtailConfig[config.ConfigNameWithSuffix] = config
	DisabledLogtailConfigLock.Unlock()
	done := make(chan int)
	go func() {
		logger.Info(config.Context.GetRuntimeContext(), "Stop config in goroutine", "begin")
		_ = config.Stop(removingFlag)
		close(done)
		logger.Info(config.Context.GetRuntimeContext(), "Stop config in goroutine", "end")
		// The config is valid but stop slowly, allow it to load again.
		DisabledLogtailConfigLock.Lock()
		if _, exists := DisabledLogtailConfig[config.ConfigNameWithSuffix]; !exists {
			DisabledLogtailConfigLock.Unlock()
			return
		}
		delete(DisabledLogtailConfig, config.ConfigNameWithSuffix)
		DisabledLogtailConfigLock.Unlock()
		logger.Info(config.Context.GetRuntimeContext(), "Valid but slow stop config, enable it again", config.ConfigName)
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

	LogtailConfigLock.Lock()
	for configName, logstoreConfig := range LogtailConfig {
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
		}
	}
	LogtailConfig = make(map[string]*LogstoreConfig)
	LogtailConfigLock.Unlock()

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

// Stop stop the given config. ConfigName is with suffix.
func Stop(configName string, exitFlag bool) error {
	defer panicRecover("Run plugin")
	LogtailConfigLock.RLock()
	if config, exists := LogtailConfig[configName]; exists {
		LogtailConfigLock.RUnlock()
		if hasStopped := timeoutStop(config, exitFlag); !hasStopped {
			logger.Error(config.Context.GetRuntimeContext(), "CONFIG_STOP_TIMEOUT_ALARM",
				"timeout when stop config, goroutine might leak")
		}
		if !exitFlag {
			LastUnsendBuffer[configName] = config.PluginRunner
		}
		logger.Info(config.Context.GetRuntimeContext(), "Stop config now", configName)
		LogtailConfigLock.Lock()
		delete(LogtailConfig, configName)
		LogtailConfigLock.Unlock()
		return nil
	}
	LogtailConfigLock.RUnlock()
	return fmt.Errorf("config not found: %s", configName)
}

// Start starts the given config. ConfigName is with suffix.
func Start(configName string) error {
	defer panicRecover("Run plugin")
	if ToStartLogtailConfigWithInput != nil && ToStartLogtailConfigWithInput.ConfigNameWithSuffix == configName {
		ToStartLogtailConfigWithInput.Start()
		LogtailConfigLock.Lock()
		LogtailConfig[ToStartLogtailConfigWithInput.ConfigNameWithSuffix] = ToStartLogtailConfigWithInput
		LogtailConfigLock.Unlock()
		ToStartLogtailConfigWithInput = nil
		return nil
	} else if ToStartLogtailConfigWithoutInput != nil && ToStartLogtailConfigWithoutInput.ConfigNameWithSuffix == configName {
		ToStartLogtailConfigWithoutInput.Start()
		LogtailConfigLock.Lock()
		LogtailConfig[ToStartLogtailConfigWithoutInput.ConfigNameWithSuffix] = ToStartLogtailConfigWithoutInput
		LogtailConfigLock.Unlock()
		ToStartLogtailConfigWithoutInput = nil
		return nil
	}
	// should never happen
	var loadedConfigName string
	if ToStartLogtailConfigWithInput != nil {
		loadedConfigName = ToStartLogtailConfigWithInput.ConfigNameWithSuffix
	}
	if ToStartLogtailConfigWithoutInput != nil {
		loadedConfigName += " " + ToStartLogtailConfigWithoutInput.ConfigNameWithSuffix
	}
	return fmt.Errorf("config unmatch with the loaded pipeline: given %s, expect %s", configName, loadedConfigName)
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
