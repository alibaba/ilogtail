package control

import (
	"fmt"
	"strings"
	"text/template"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/setup"
)

const iLogtailLocalConfigDir = "/etc/ilogtail/config/local"
const SLSFlusherConfigTemplate = `
flushers:
  - Type: flusher_sls
    Aliuid: "{{.Aliuid}}"
    TelemetryType: "logs"
    Region: {{.Region}}
    Endpoint: {{.Endpoint}}
    Project: {{.Project}}
    Logstore: {{.Logstore}}`

var SLSFlusherConfig string

func InitSLSFlusherConfig() {
	tpl := template.Must(template.New("slsFlusherConfig").Parse(SLSFlusherConfigTemplate))
	var builder strings.Builder
	tpl.Execute(&builder, map[string]interface{}{
		"Aliuid":   config.TestConfig.Aliuid,
		"Region":   config.TestConfig.Region,
		"Endpoint": config.TestConfig.Endpoint,
		"Project":  config.TestConfig.Project,
		"Logstore": config.TestConfig.Logstore,
	})
	SLSFlusherConfig = builder.String()
}

func AddLocalConfig(c string, configName string) {
	command := fmt.Sprintf(`cd %s && cat << 'EOF' > %s.yaml
  %s`, iLogtailLocalConfigDir, configName, c+SLSFlusherConfig)
	if err := setup.Env.Exec(command); err != nil {
		panic(err)
	}
}

func RemoveAllLocalConfig() {
	command := fmt.Sprintf("cd %s && rm -rf *.yaml", iLogtailLocalConfigDir)
	if err := setup.Env.Exec(command); err != nil {
		panic(err)
	}
}
