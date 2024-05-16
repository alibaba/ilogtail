package trigger

import (
	"fmt"
	"os"
)

const commandTemplate = "/usr/local/go/bin/go test -v -run ^%s$ github.com/alibaba/ilogtail/test/testhub/trigger"

func getEnvOrDefault(env, fallback string) string {
	if value, ok := os.LookupEnv(env); ok {
		return value
	}
	return fallback
}

func getRunTriggerCommand(triggerName string) string {
	return fmt.Sprintf(commandTemplate, triggerName)
}
