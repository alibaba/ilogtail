package core

import (
	"os"
	"testing"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/test/config"
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
	config.TestConfig.GeneratedLogPath = os.Getenv("GENERATED_LOG_PATH")
	if len(config.TestConfig.GeneratedLogPath) == 0 {
		config.TestConfig.GeneratedLogPath = "/tmp/ilogtail"
	}

	// SSH
	config.TestConfig.SSHUsername = os.Getenv("SSH_USERNAME")
	config.TestConfig.SSHIP = os.Getenv("SSH_IP")
	config.TestConfig.SSHPrivateKeyPath = os.Getenv("SSH_PRIVATE_KEY_PATH")
	config.TestConfig.WorkDir = os.Getenv("WORK_DIR")
	if len(config.TestConfig.WorkDir) == 0 {
		config.TestConfig.WorkDir = "/root/ilogtail/test"
	}

	// SLS
	config.TestConfig.Project = os.Getenv("PROJECT")
	config.TestConfig.Logstore = os.Getenv("LOGSTORE")
	config.TestConfig.AccessKeyId = os.Getenv("ACCESS_KEY_ID")
	config.TestConfig.AccessKeySecret = os.Getenv("ACCESS_KEY_SECRET")
	config.TestConfig.Endpoint = os.Getenv("ENDPOINT")

	os.Exit(m.Run())
}
