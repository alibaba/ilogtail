package subscriber

import (
	"strings"
	"sync"
	"text/template"

	"github.com/alibaba/ilogtail/test/config"
)

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
var SLSFlusherConfigOnce sync.Once

func FlusherConfig() string {
	SLSFlusherConfigOnce.Do(func() {
		tpl := template.Must(template.New("slsFlusherConfig").Parse(SLSFlusherConfigTemplate))
		var builder strings.Builder
		_ = tpl.Execute(&builder, map[string]interface{}{
			"Aliuid":   config.TestConfig.Aliuid,
			"Region":   config.TestConfig.Region,
			"Endpoint": config.TestConfig.Endpoint,
			"Project":  config.TestConfig.Project,
			"Logstore": config.TestConfig.Logstore,
		})
		SLSFlusherConfig = builder.String()
	})
	return SLSFlusherConfig
}
