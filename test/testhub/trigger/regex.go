package trigger

import (
	"fmt"

	"github.com/alibaba/ilogtail/test/config"
)

func TriggerRegexSingle(totalLog, interval int, filename string) string {
	command := fmt.Sprintf(commandTemplate, "TestGenerateRegexLogSingle")
	return fmt.Sprintf("cd %s && TOTAL_LOG=%v INTERVAL=%v FILENAME=%v %s", config.TestConfig.WorkDir, totalLog, interval, filename, command)
}
