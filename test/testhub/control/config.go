package control

import (
	"fmt"
	"log"
	"os"
)

const iLogtailLocalConfigDir = "/etc/ilogtail/config/local"
const SLSFlusherConfig = `
flushers:
  - Type: flusher_sls
    Region: ap-southeast-7
    Endpoint: ap-southeast-7.log.aliyuncs.com
    Project: logtail-e2e-bangkok
    Logstore: e2e-test
`

func AddLocalConfig(config []byte, configName string) error {
	filePath := fmt.Sprintf("%s/%s.yaml", iLogtailLocalConfigDir, configName)
	file, err := os.OpenFile(filePath, os.O_WRONLY|os.O_CREATE|os.O_TRUNC, 0644)
	if err != nil {
		log.Fatal(err)
	}
	defer file.Close()
	return os.WriteFile(filePath, config, 0644)
}

func RemoveLocalConfig(configName string) error {
	filePath := fmt.Sprintf("%s/%s.yaml", iLogtailLocalConfigDir, configName)
	return os.Remove(filePath)
}
