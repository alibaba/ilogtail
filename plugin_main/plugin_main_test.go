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

//go:build linux || windows
// +build linux windows

package main

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	_ "github.com/alibaba/ilogtail/pkg/logger/test"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pluginmanager"

	"github.com/stretchr/testify/require"
)

type BadFlusher struct {
	TimeoutSecond int // -1 means hang forever.
	Shutdown      chan int
}

func (f *BadFlusher) Init(ctx pipeline.Context) error {
	return nil
}

func (f *BadFlusher) Description() string {
	return ""
}

func (f *BadFlusher) SetUrgent(flag bool) {
}

func (f *BadFlusher) IsReady(projectName string, logstoreName string, logstoreKey int64) bool {
	return true
}

func (f *BadFlusher) Flush(projectName string, logstoreName string, configName string, logGroupList []*protocol.LogGroup) error {
	return nil
}

func (f *BadFlusher) Stop() error {
	if f.TimeoutSecond == -1 {
		logger.Info(context.Background(), "hang stop")
		for {
			time.Sleep(time.Second * 1)
			select {
			case <-f.Shutdown:
				logger.Info(context.Background(), "quit hang stop")
				return nil
			default:
			}
		}
	} else {
		logger.Info(context.Background(), "timeout stop", f.TimeoutSecond)
		time.Sleep(time.Duration(f.TimeoutSecond) * time.Second)
	}
	return nil
}

func init() {
	pipeline.Flushers["flusher_bad"] = func() pipeline.Flusher {
		return &BadFlusher{}
	}
}

const configTemplateJSONStr = `{
	"flushers": [{
		"type": "flusher_bad",
		"detail": {
			"TimeoutSecond": %v
		}
	}]
}`

// TestHangConfigWhenStop tests if the plugin system can disable that will hang when Stop.
func TestHangConfigWhenStop(t *testing.T) {
	badConfigStr := fmt.Sprintf(configTemplateJSONStr, -1)
	configName := "configName"
	shutdown := make(chan int)

	// Initialize plugin and run config.
	require.Equal(t, 0, InitPluginBase())
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))
	config := pluginmanager.ToStartPipelineConfigWithoutInput
	require.NotNil(t, config)
	require.Equal(t, configName, config.ConfigName)
	flusher, _ := pluginmanager.GetConfigFlushers(config.PluginRunner)[0].(*BadFlusher)
	flusher.Shutdown = shutdown
	Start(configName)
	time.Sleep(time.Second * 2)
	require.Nil(t, pluginmanager.ToStartPipelineConfigWithoutInput)
	pluginmanager.LogtailConfigLock.RLock()
	_, exists := pluginmanager.LogtailConfig[configName]
	require.True(t, exists)
	pluginmanager.LogtailConfigLock.RUnlock()

	// Stop config, it will hang.
	Stop(configName, 0)
	time.Sleep(time.Second * 2)
	config, exists = pluginmanager.DisabledLogtailConfig[configName]
	require.Equal(t, configName, config.ConfigName)
	require.True(t, exists)
	// Load again, succeed. Changed since independently reload
	time.Sleep(time.Second)
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))

	// Notify the config to quit so that it can be enabled again.
	close(shutdown)
	time.Sleep(time.Second * 2)
	require.Empty(t, pluginmanager.DisabledLogtailConfig)

	// Load again, succeed.
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))
	config = pluginmanager.ToStartPipelineConfigWithoutInput
	require.NotNil(t, config)
	require.Equal(t, configName, config.ConfigName)
	Start(configName)
	time.Sleep(time.Second)
	pluginmanager.LogtailConfigLock.RLock()
	_, exists = pluginmanager.LogtailConfig[configName]
	pluginmanager.LogtailConfigLock.RUnlock()
	require.True(t, exists)
	flusher, _ = pluginmanager.GetConfigFlushers(config.PluginRunner)[0].(*BadFlusher)
	shutdown = make(chan int)
	flusher.Shutdown = shutdown

	// Stop config, hang again.
	Stop(config.ConfigNameWithSuffix, 0)
	time.Sleep(time.Second * 2)
	config, exists = pluginmanager.DisabledLogtailConfig[config.ConfigNameWithSuffix]
	require.True(t, exists)
	require.Equal(t, configName, config.ConfigName)

	// Change config detail, load a new pipeline.
	validConfigStr := fmt.Sprintf(configTemplateJSONStr, 4)
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, validConfigStr))
	Start(configName)
	time.Sleep(time.Second * 2)
	pluginmanager.LogtailConfigLock.RLock()
	config, exists = pluginmanager.LogtailConfig[configName]
	pluginmanager.LogtailConfigLock.RUnlock()
	require.True(t, exists)
	require.Equal(t, configName, config.ConfigName)

	// Quit.
	time.Sleep(time.Second)
	StopAll(1)
	StopAll(0)
	pluginmanager.LogtailConfigLock.RLock()
	require.Empty(t, pluginmanager.LogtailConfig)
	pluginmanager.LogtailConfigLock.RUnlock()

	// Close hanged goroutine.
	close(shutdown)
}

// TestSlowConfigWhenStop tests if the plugin system can disable that will stop slowly.
func TestSlowConfigWhenStop(t *testing.T) {
	badConfigStr := fmt.Sprintf(configTemplateJSONStr, 35)
	configName := "configName"

	// Initialize plugin and run config.
	require.Equal(t, 0, InitPluginBase())
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))
	config := pluginmanager.ToStartPipelineConfigWithoutInput
	require.NotNil(t, config)
	require.Equal(t, configName, config.ConfigName)
	Start(configName)
	config, exists := pluginmanager.LogtailConfig[configName]
	require.True(t, exists)
	require.Equal(t, configName, config.ConfigName)
	time.Sleep(time.Second * 2)
	pluginmanager.LogtailConfigLock.RLock()
	config, ok := pluginmanager.LogtailConfig[configName]
	pluginmanager.LogtailConfigLock.RUnlock()
	require.True(t, ok)
	require.Equal(t, configName, config.ConfigName)

	// Stop config, it will hang.
	Stop(configName, 0)
	config, exists = pluginmanager.DisabledLogtailConfig[configName]
	require.True(t, exists)
	require.Equal(t, configName, config.ConfigName)
	// Load again, success. Changed since independently reload
	time.Sleep(time.Second)
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))
	require.Empty(t, pluginmanager.LogtailConfig)
	require.Nil(t, pluginmanager.ToStartPipelineConfigWithInput)

	// Wait more time, so that the config can finish stopping.
	time.Sleep(time.Second * 5)
	// Load again, succeed.
	require.Equal(t, 0, LoadPipeline("project", "logstore", configName, 0, badConfigStr))
	config = pluginmanager.ToStartPipelineConfigWithoutInput
	require.NotNil(t, config)
	require.Equal(t, configName, config.ConfigName)
	Start(configName)
	config, exists = pluginmanager.LogtailConfig[configName]
	require.True(t, exists)
	require.Equal(t, configName, config.ConfigName)
	time.Sleep(time.Second)
	pluginmanager.LogtailConfigLock.RLock()
	config, ok = pluginmanager.LogtailConfig[configName]
	pluginmanager.LogtailConfigLock.RUnlock()
	require.True(t, ok)
	require.Equal(t, configName, config.ConfigName)

	// Quit.
	time.Sleep(time.Second)
	StopAll(1)
	StopAll(0)
	time.Sleep(time.Second * 6)
	require.Empty(t, pluginmanager.DisabledLogtailConfig)
}
