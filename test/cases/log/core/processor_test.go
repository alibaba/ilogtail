package core

import (
	"fmt"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	testhub "github.com/alibaba/ilogtail/test/testhub/common"
	"github.com/alibaba/ilogtail/test/testhub/control"
	"github.com/alibaba/ilogtail/test/testhub/trigger"
)

func TestRegexSingle(t *testing.T) {
	// Setup
	c := fmt.Sprintf(`enable: true
inputs:
- Type: input_file
  FilePaths:
    - %s/simple.log
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
%s`, config.TestConfig.GeneratedLogPath, control.SLSFlusherConfig)
	if _, err := testhub.SSHExec(control.AddLocalConfig(c, "regex_single")); err != nil {
		t.Error(err)
	}
	// Trigger
	// startTime := time.Now().Unix()
	if output, err := testhub.SSHExec(trigger.TriggerRegexSingle(2000, int(time.Second*1), "simple.log")); err != nil {
		fmt.Println(string(output))
		t.Error(err)
	}
	// Verify
	// if err := verify.VerifyLogCount(100, int32(startTime), int32(time.Now().Unix())); err != nil {
	// 	t.Error(err)
	// }
	// if err := verify.VerifyRegexSingle(int32(startTime), int32(time.Now().Unix())); err != nil {
	//     t.Error(err)
	// }
	// // Cleanup
	// control.RemoveLocalConfig("regex_single")
	// cleanup.CleanupGeneratedLog()
}
