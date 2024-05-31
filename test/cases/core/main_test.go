// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package core

import (
	"os"
	"strconv"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
)

func TestMain(m *testing.M) {
	loggerOptions := []logger.ConfigOption{
		logger.OptionAsyncLogger,
	}
	loggerOptions = append(loggerOptions, logger.OptionInfoLevel)
	logger.InitTestLogger(loggerOptions...)

	config.TestConfig = config.Config{}
	// Log
	config.TestConfig.GeneratedLogDir = os.Getenv("GENERATED_LOG_DIR")
	if len(config.TestConfig.GeneratedLogDir) == 0 {
		config.TestConfig.GeneratedLogDir = "/tmp/ilogtail"
	}
	config.TestConfig.WorkDir = os.Getenv("WORK_DIR")

	// SSH
	config.TestConfig.SSHUsername = os.Getenv("SSH_USERNAME")
	config.TestConfig.SSHIP = os.Getenv("SSH_IP")
	config.TestConfig.SSHPassword = os.Getenv("SSH_PASSWORD")

	// K8s
	config.TestConfig.KubeConfigPath = os.Getenv("KUBE_CONFIG_PATH")

	// SLS
	config.TestConfig.Project = os.Getenv("PROJECT")
	config.TestConfig.Logstore = os.Getenv("LOGSTORE")
	config.TestConfig.AccessKeyID = os.Getenv("ACCESS_KEY_ID")
	config.TestConfig.AccessKeySecret = os.Getenv("ACCESS_KEY_SECRET")
	config.TestConfig.Endpoint = os.Getenv("ENDPOINT")
	config.TestConfig.Aliuid = os.Getenv("ALIUID")
	config.TestConfig.QueryEndpoint = os.Getenv("QUERY_ENDPOINT")
	config.TestConfig.Region = os.Getenv("REGION")
	timeout, err := strconv.ParseInt(os.Getenv("RETRY_TIMEOUT"), 10, 64)
	if err != nil {
		timeout = 30
	}
	config.TestConfig.RetryTimeout = time.Duration(timeout) * time.Second
	code := m.Run()
	logger.Flush()
	os.Exit(code)
}
