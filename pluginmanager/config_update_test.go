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

package pluginmanager

import (
	"context"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/plugins/flusher/checker"

	"github.com/stretchr/testify/suite"
)

var updateConfigName = "update_mock_block"
var noblockUpdateConfigName = "update_mock_noblock"

func TestConfigUpdate(t *testing.T) {
	suite.Run(t, new(configUpdateTestSuite))
}

type configUpdateTestSuite struct {
	suite.Suite
}

func (s *configUpdateTestSuite) BeforeTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test start ========================", suiteName, testName)
	logger.Info(context.Background(), "load logstore config", updateConfigName)
	s.NoError(LoadMockConfig(updateConfigName, updateConfigName, updateConfigName, GetTestConfig(updateConfigName)))
	s.NoError(Start(updateConfigName))
	time.Sleep(time.Second * time.Duration(1))
}

func (s *configUpdateTestSuite) AfterTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test end ========================", suiteName, testName)
	s.NoError(StopAll(false, false))
	s.NoError(StopAll(false, true))
	LogtailConfig = sync.Map{}
}

func (s *configUpdateTestSuite) TestConfigUpdate() {
	// block config
	config, ok := GetLogtailConfig(updateConfigName)
	s.True(ok)
	s.NotNil(config, "%s logstrore config should exist", updateConfigName)
	checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	s.True(ok)
	s.Equal(0, checkFlusher.GetLogCount(), "the block flusher checker doesn't have any logs")

	// update same hang config
	s.NoError(Stop(updateConfigName, false))
	s.Equal(0, checkFlusher.GetLogCount(), "the hold on block flusher checker doesn't have any logs")
	err := LoadMockConfig(updateConfigName, updateConfigName, updateConfigName, GetTestConfig(updateConfigName))
	s.True(strings.Contains(err.Error(), "failed to create config because timeout stop has happened on it"))
	s.NoError(LoadMockConfig(noblockUpdateConfigName, noblockUpdateConfigName, noblockUpdateConfigName, GetTestConfig(noblockUpdateConfigName)))
	s.NoError(Start(updateConfigName))
	_, exist := GetLogtailConfig(updateConfigName)
	s.False(exist)
	_, exist = GetLogtailConfig(noblockUpdateConfigName)
	s.True(exist)

	// unblock old config
	checkFlusher.Block = false
	time.Sleep(time.Second * time.Duration(5))
	s.Equal(10000, checkFlusher.GetLogCount())
	// this magic number(10000) must exceed number of logs can be hold in processor channel(LogsChan) + aggregator buffer(defaultLogGroup) + flusher channel(LogGroupsChan)
	config, ok = GetLogtailConfig(noblockUpdateConfigName)
	s.True(ok)
	s.Equal(20000, GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker).GetLogCount())
}

func (s *configUpdateTestSuite) TestConfigUpdateMany() {
	config, ok := GetLogtailConfig(updateConfigName)
	s.True(ok)
	s.NotNil(config, "%s logstrore config should exist", updateConfigName)
	checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	s.True(ok)

	s.Equal(0, checkFlusher.GetLogCount(), "the hold on block flusher checker doesn't have any logs")
	// load block config
	for i := 0; i < 5; i++ {
		s.NoError(Stop(updateConfigName, false))
		err := LoadMockConfig(updateConfigName, updateConfigName, updateConfigName, GetTestConfig(updateConfigName))
		s.True(strings.Contains(err.Error(), "failed to create config because timeout stop has happened on it"))
		s.NoError(Start(updateConfigName))
		_, exist := GetLogtailConfig(updateConfigName)
		s.False(exist)
	}
	s.Equal(0, checkFlusher.GetLogCount(), "the hold on block flusher checker doesn't have any logs")
	checkFlusher.Block = false
	time.Sleep(time.Second * time.Duration(5))
	s.Equal(checkFlusher.GetLogCount(), 10000)

	// load normal config
	for i := 0; i < 5; i++ {
		s.NoError(StopAll(false, true))
		s.NoError(StopAll(false, false))
		s.NoError(LoadMockConfig(noblockUpdateConfigName, noblockUpdateConfigName, noblockUpdateConfigName, GetTestConfig(noblockUpdateConfigName)))
		s.NoError(Start(noblockUpdateConfigName))
		_, exist := GetLogtailConfig(noblockUpdateConfigName)
		s.True(exist)
		time.Sleep(time.Millisecond)
	}
	config, ok = GetLogtailConfig(noblockUpdateConfigName)
	s.True(ok)
	checkFlusher, ok = GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	s.True(ok)
	time.Sleep(time.Second * time.Duration(5))
	s.Equal(checkFlusher.GetLogCount(), 20000)
}

func (s *configUpdateTestSuite) TestConfigUpdateName() {
	time.Sleep(time.Second * time.Duration(1))
	config, ok := GetLogtailConfig(updateConfigName)
	s.True(ok)
	checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	defer func() {
		checkFlusher.Block = false
		time.Sleep(time.Second * 5)
		s.Equal(checkFlusher.GetLogCount(), 10000)
	}()
	s.True(ok)

	s.NoError(Stop(updateConfigName, false))
	s.Equal(0, checkFlusher.GetLogCount(), "the hold on blocking flusher checker doesn't have any logs")
	s.NoError(LoadMockConfig(updateConfigName+"_", updateConfigName+"_", updateConfigName+"_", GetTestConfig(updateConfigName)))
	s.NoError(Start(updateConfigName))

	{
		_, exist := GetLogtailConfig(updateConfigName)
		s.False(exist)
		config, exist := GetLogtailConfig(updateConfigName + "_")
		s.True(exist)
		checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
		s.True(ok)
		s.Equal(checkFlusher.GetLogCount(), 0)
		checkFlusher.Block = false
		time.Sleep(time.Second * 5)
		s.Equal(checkFlusher.GetLogCount(), 20000)
	}
}

func (s *configUpdateTestSuite) TestStopAllExit() {
	config, ok := GetLogtailConfig(updateConfigName)
	s.True(ok)
	checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	s.True(ok)
	checkFlusher.Block = false
	time.Sleep(time.Second * time.Duration(5))
	s.NoError(StopAll(true, true))
	s.NoError(StopAll(true, false))
	s.Equal(20000, checkFlusher.GetLogCount())
	s.NoError(Start(updateConfigName))
}

func (s *configUpdateTestSuite) TestHoldOnExitTimeout() {
	time.Sleep(time.Second * time.Duration(1))
	config, ok := GetLogtailConfig(updateConfigName)
	s.True(ok)
	checkFlusher, ok := GetConfigFlushers(config.PluginRunner)[0].(*checker.FlusherChecker)
	s.True(ok)
	s.Equal(0, checkFlusher.GetLogCount())
	s.NoError(StopAll(true, true))
	s.NoError(StopAll(true, false))
	time.Sleep(time.Second)
	s.Equal(0, checkFlusher.GetLogCount())
	checkFlusher.Block = false
	time.Sleep(time.Second * time.Duration(5))
	s.Equal(10000, checkFlusher.GetLogCount())
	time.Sleep(time.Second * 10)
	s.NoError(Start(updateConfigName))
}
