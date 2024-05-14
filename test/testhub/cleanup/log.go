package cleanup

import (
	"os"

	"github.com/alibaba/ilogtail/test/config"
)

func CleanupGeneratedLog() error {
	err := os.RemoveAll(config.TestConfig.GeneratedLogPath)
	if err != nil {
		return err
	}
	return nil
}
