package trigger

import (
	"context"
	"html/template"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
)

const triggerRegexTemplate = "cd {{.WorkDir}} && TOTAL_LOG={{.TotalLog}} INTERVAL={{.Interval}} FILENAME={{.Filename}} {{.Command}}"

func TriggerRegexSingle(ctx context.Context, totalLog, interval int) (context.Context, error) {
	command := getRunTriggerCommand("TestGenerateRegexLogSingle")
	var triggerRegexCommand strings.Builder
	template := template.Must(template.New("triggerRegexSingle").Parse(triggerRegexTemplate))
	if err := template.Execute(&triggerRegexCommand, map[string]interface{}{
		"WorkDir":  config.TestConfig.WorkDir,
		"TotalLog": totalLog,
		"Interval": interval,
		"Filename": "regex_single.log",
		"Command":  command,
	}); err != nil {
		return ctx, err
	}
	startTime := time.Now().Unix()
	if err := setup.Env.ExecOnSource(triggerRegexCommand.String()); err != nil {
		return ctx, err
	}
	return context.WithValue(ctx, "startTime", int32(startTime)), nil
}
