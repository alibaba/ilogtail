// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package control

import (
	"context"
	"fmt"
	"strings"
	"sync"
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
var SLSFlusherConfigOnce sync.Once

func AddLocalConfig(ctx context.Context, configName, c string) (context.Context, error) {
	c = completeConfigWithFlusher(c)
	command := fmt.Sprintf(`cd %s && cat << 'EOF' > %s.yaml
%s`, iLogtailLocalConfigDir, configName, c)
	if err := setup.Env.ExecOnLogtail(command); err != nil {
		return ctx, err
	}
	return ctx, nil
}

func RemoveAllLocalConfig(ctx context.Context) (context.Context, error) {
	command := fmt.Sprintf("cd %s && rm -rf *.yaml", iLogtailLocalConfigDir)
	if err := setup.Env.ExecOnLogtail(command); err != nil {
		return ctx, err
	}
	return ctx, nil
}

func completeConfigWithFlusher(c string) string {
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
	return c + SLSFlusherConfig
}
