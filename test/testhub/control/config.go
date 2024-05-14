package control

import (
	"fmt"
)

const iLogtailLocalConfigDir = "/etc/ilogtail/config/local"
const SLSFlusherConfig = `flushers:
  - Type: flusher_sls
    Region: ap-southeast-7
    Endpoint: ap-southeast-7.log.aliyuncs.com
    Project: logtail-e2e-bangkok
    Logstore: e2e-test`

func AddLocalConfig(config string, configName string) string {
	return fmt.Sprintf(`cd %s && cat << 'EOF' > %s.yaml
%s
`, iLogtailLocalConfigDir, configName, config)
}

func RemoveLocalConfig(configName string) string {
	return fmt.Sprintf("cd %s && rm -rf %s.yaml", iLogtailLocalConfigDir, configName)
}
