package core

import (
	"os"
	"testing"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/control"
)

func TestMain(m *testing.M) {
	loggerOptions := []logger.ConfigOption{
		logger.OptionAsyncLogger,
	}
	loggerOptions = append(loggerOptions, logger.OptionInfoLevel)
	logger.InitTestLogger(loggerOptions...)
	defer logger.Flush()

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
	config.TestConfig.AccessKeyId = os.Getenv("ACCESS_KEY_ID")
	config.TestConfig.AccessKeySecret = os.Getenv("ACCESS_KEY_SECRET")
	config.TestConfig.Endpoint = os.Getenv("ENDPOINT")
	config.TestConfig.Aliuid = os.Getenv("ALIUID")
	config.TestConfig.QueryEndpoint = os.Getenv("QUERY_ENDPOINT")
	config.TestConfig.Region = os.Getenv("REGION")

	control.InitSLSClient()
	control.InitSLSFlusherConfig()

	os.Exit(m.Run())
}
