package trigger

import "fmt"

func TriggerRegexSingle(totalLog, interval int, filename string) string {
	command := fmt.Sprintf(commandTemplate, "TestGenerateRegexLogSingle")
	return fmt.Sprintf("TOTAL_LOG=%v INTERVAL=%v FILENAME=%v %s", totalLog, interval, filename, command)
}
