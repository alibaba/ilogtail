package core

import (
	"fmt"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/cleanup"
	"github.com/alibaba/ilogtail/test/testhub/control"
	"github.com/alibaba/ilogtail/test/testhub/setup"
	"github.com/alibaba/ilogtail/test/testhub/trigger"
	"github.com/alibaba/ilogtail/test/testhub/verify"
)

func TestRegexSingleOnHost(t *testing.T) {
	// Cleanup
	defer cleanup.CleanupAll()
	// Setup
	setup.InitEnv("host")
	c := fmt.Sprintf(`
enable: true
inputs:
- Type: input_file
  FilePaths:
    - %s/regex_single.log
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: (\S+)\s(\w+):(\d+)\s(\S+)\s-\s\[([^]]+)]\s"(\w+)\s(\S+)\s([^"]+)"\s(\d+)\s(\d+)\s"([^"]+)"\s(.*)
    Keys:
      - mark
      - file
      - logNo
      - ip
      - time
      - method
      - url
      - http
      - status
      - size
      - userAgent
      - msg
%s`, config.TestConfig.GeneratedLogDir, control.SLSFlusherConfig)
	control.AddLocalConfig(c, "regex_single")
	// Trigger
	startTime := time.Now().Unix()
	trigger.TriggerRegexSingle(100, 100, "regex_single.log")
	control.WaitLog2SLS(int32(startTime), 30*time.Second)
	// Verify
	verify.VerifyLogCount(100, int32(startTime))
	verify.VerifyRegexSingle(int32(startTime))
}

func TestRegexSingleOnDaemonSet(t *testing.T) {
	// Cleanup
	defer cleanup.CleanupAll()
	// Setup
	setup.InitEnv("daemonset")
	c := fmt.Sprintf(`
enable: true
inputs:
- Type: input_file
  EnableContainerDiscovery: true
  FilePaths:
    - %s/simple.log
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    K8sNamespaceRegex: default
    Regex: (\S+)\s(\w+):(\d+)\s(\S+)\s-\s\[([^]]+)]\s"(\w+)\s(\S+)\s([^"]+)"\s(\d+)\s(\d+)\s"([^"]+)"\s(.*)
    Keys:
      - mark
      - file
      - logNo
      - ip
      - time
      - method
      - url
      - http
      - status
      - size
      - userAgent
      - msg
%s`, config.TestConfig.GeneratedLogDir, control.SLSFlusherConfig)
	control.AddLocalConfig(c, "regex_single")
	// // Trigger
	// startTime := time.Now().Unix()
	// trigger.TriggerRegexSingle(100, 100, "simple.log")
	// control.WaitLog2SLS(int32(startTime), 30*time.Second)
	// // Verify
	// verify.VerifyLogCount(100, int32(startTime))
	// verify.VerifyRegexSingle(int32(startTime))
}
