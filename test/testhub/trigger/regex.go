package trigger

import (
	"html/template"
	"strings"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
)

const triggerRegexTemplate = "cd {{.WorkDir}} && TOTAL_LOG={{.TotalLog}} INTERVAL={{.Interval}} FILENAME={{.Filename}} {{.Command}}"

func TriggerRegexSingle(totalLog, interval int, filename string) {
	command := getRunTriggerCommand("TestGenerateRegexLogSingle")
	var triggerRegexCommand strings.Builder
	template := template.Must(template.New("triggerRegexSingle").Parse(triggerRegexTemplate))
	if err := template.Execute(&triggerRegexCommand, map[string]interface{}{
		"WorkDir":  config.TestConfig.WorkDir,
		"TotalLog": totalLog,
		"Interval": interval,
		"Filename": filename,
		"Command":  command,
	}); err != nil {
		panic(err)
	}
	if err := setup.Env.ExecOnSource(triggerRegexCommand.String()); err != nil {
		panic(err)
	}
}
