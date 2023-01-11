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
	"testing"

	"github.com/alibaba/ilogtail/pkg/logger"

	_ "github.com/alibaba/ilogtail/plugins/flusher/sls"
	_ "github.com/alibaba/ilogtail/plugins/input/docker/stdout"

	"github.com/stretchr/testify/suite"
)

func TestContainerConfig(t *testing.T) {
	suite.Run(t, new(containerConfigTestSuite))
}

func (s *containerConfigTestSuite) TestRefreshEnvAndLabel() {
	s.NoError(loadMockConfig(), "got err when logad config")
	// s.NoError(Resume(), "got err when resume")
	// s.NoError(HoldOn(false), "got err when hold on")

	refreshEnvAndLabel()
	s.Equal(1, len(LogtailConfig))
	s.Equal(1, len(envSet))
	s.Equal(1, len(labelSet))
}

type containerConfigTestSuite struct {
	suite.Suite
}

func (s *containerConfigTestSuite) BeforeTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test start ========================", suiteName, testName)
	// s.NoError(Init(), "got error when init")
}

func (s *containerConfigTestSuite) AfterTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test end =======================", suiteName, testName)

}

// project, logstore, config, configJsonStr
func loadMockConfig() error {
	project := "test_prj"
	logstore := "test_logstore"
	configName := "test_config"

	configStr := `
	{
		"inputs": [{
			"detail": {
				"Stderr": true,
				"IncludeLabel": {
					"app": "^.*$"
				},
				"IncludeEnv": {
					"test": "^.*$"
				},
				"Stdout": true
			},
			"type": "service_docker_stdout"
		}],
		"global": {
			"AlwaysOnline": true,
			"type": "dockerStdout"
		}
	}`
	return LoadLogstoreConfig(project, logstore, configName, 666, configStr)
}
