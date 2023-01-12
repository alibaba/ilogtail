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

	"github.com/docker/docker/api/types"
	"github.com/docker/docker/api/types/container"
	"github.com/stretchr/testify/suite"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/flags"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
	_ "github.com/alibaba/ilogtail/plugins/flusher/sls"
	_ "github.com/alibaba/ilogtail/plugins/input/docker/stdout"
)

func TestContainerConfig(t *testing.T) {
	suite.Run(t, new(containerConfigTestSuite))
}

func (s *containerConfigTestSuite) TestRefreshEnvAndLabel() {
	s.NoError(loadMockConfig(), "got err when logad config")
	s.Equal(1, len(LogtailConfig))
	s.Equal(1, len(envSet))
	s.Equal(1, len(labelSet))
}

func (s *containerConfigTestSuite) TestCompareEnvAndLabel() {
	envSet = make(map[string]struct{})
	labelSet = make(map[string]struct{})

	s.NoError(loadMockConfig(), "got err when logad config")

	envSet["testEnv1"] = struct{}{}
	labelSet["testLebel1"] = struct{}{}

	diffEnvSet, diffLabelSet := compareEnvAndLabel()
	s.Equal(1, len(diffEnvSet))
	s.Equal(1, len(diffLabelSet))
	s.Equal(2, len(envSet))
	s.Equal(2, len(labelSet))
}

func (s *containerConfigTestSuite) TestCompareEnvAndLabelAndRecordContainer() {
	envSet = make(map[string]struct{})
	labelSet = make(map[string]struct{})

	s.NoError(loadMockConfig(), "got err when logad config")

	envSet["testEnv1"] = struct{}{}
	labelSet["testLebel1"] = struct{}{}

	envList := []string{0: "test=111"}
	info := mockDockerInfoDetail("testConfig", envList)
	cMap := helper.GetContainerMap()
	cMap["test"] = info

	compareEnvAndLabelAndRecordContainer()
	s.Equal(1, len(util.AddedContainers))
}

func (s *containerConfigTestSuite) TestRecordContainers() {
	info := mockDockerInfoDetail("testConfig", []string{0: "test=111"})
	cMap := helper.GetContainerMap()
	cMap["test"] = info

	containerIDs := make(map[string]struct{})
	containerIDs["test"] = struct{}{}
	recordContainers(containerIDs)
	s.Equal(1, len(util.AddedContainers))
}

type containerConfigTestSuite struct {
	suite.Suite
}

func (s *containerConfigTestSuite) BeforeTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test start ========================", suiteName, testName)
}

func (s *containerConfigTestSuite) AfterTest(suiteName, testName string) {
	logger.Infof(context.Background(), "========== %s %s test end =======================", suiteName, testName)

}

func mockDockerInfoDetail(containerName string, envList []string) *helper.DockerInfoDetail {
	dockerInfo := types.ContainerJSON{
		ContainerJSONBase: &types.ContainerJSONBase{
			Name: containerName,
			ID:   "test",
		},
	}
	dockerInfo.Config = &container.Config{}
	dockerInfo.Config.Env = envList
	return helper.CreateContainerInfoDetail(dockerInfo, *flags.LogConfigPrefix, false)
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
