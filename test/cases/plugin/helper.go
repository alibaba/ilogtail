package plugin

import (
	"context"
	"fmt"
	"os"
	"runtime"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/engine/controller"
)

func RunPluginE2E(configName string, testType string, debug bool, profile bool) error {
	defer func() {
		if err := recover(); err != nil {
			trace := make([]byte, 2048)
			runtime.Stack(trace, true)
			logger.Error(context.Background(), "PLUGIN_RUNTIME_ALARM", "panicked", err, "stack", string(trace))
			logger.Flush()
			_ = os.Rename(util.GetCurrentBinaryPath()+"logtail_plugin.LOG", config.EngineLogFile)
			os.Exit(1)
		}
		_ = os.Rename(util.GetCurrentBinaryPath()+"logtail_plugin.LOG", config.EngineLogFile)
	}()
	loggerOptions := []logger.ConfigOption{
		logger.OptionAsyncLogger,
	}
	if debug {
		loggerOptions = append(loggerOptions, logger.OptionDebugLevel)
	} else {
		loggerOptions = append(loggerOptions, logger.OptionInfoLevel)
	}
	logger.InitTestLogger(loggerOptions...)
	defer logger.Flush()

	configPath, err := getConfigPathByName(configName, testType)
	if err != nil {
		return err
	}
	cfg, err := config.Load(configPath, profile)
	if err != nil {
		return err
	}
	if err := controller.Start(cfg); err != nil {
		logger.Error(context.Background(), "CONTROLLER_ALARM", "err", err)
		logger.Flush()
		os.Exit(1)
	}
	return nil
}

func getConfigPathByName(configName string, testType string) (string, error) {
	wd, err := os.Getwd()
	if err != nil {
		return "", fmt.Errorf("Cannot get current working directory")
	}
	return fmt.Sprintf("%s/scenarios/%s/%s", wd, testType, configName), nil
}
