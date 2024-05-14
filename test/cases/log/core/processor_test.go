package core

import (
	"fmt"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/test/config"
	"github.com/alibaba/ilogtail/test/testhub/cleanup"
	testhub "github.com/alibaba/ilogtail/test/testhub/common"
	"github.com/alibaba/ilogtail/test/testhub/control"
	"github.com/alibaba/ilogtail/test/testhub/trigger"
	"github.com/alibaba/ilogtail/test/testhub/verify"
)

func TestRegexSingle(t *testing.T) {
	// Setup
	c := fmt.Sprintf(`
enable: true
inputs:
- Type: input_file
  FilePaths:
    - /%s/simple.log
processors:
  - Type: processor_parse_regex_native
    SourceKey: content
    Regex: (\S+)\s(\w+):(\d+)\s\[([^]]+)]\s\[(\w+)]\s(.*)
    Keys:
      - mark
      - file
      - logNo
      - time
      - level
      - msg
%s`, config.TestConfig.GeneratedLogPath, control.SLSFlusherConfig)
	control.AddLocalConfig([]byte(c), "regex_single")
	// Trigger
	startTime := time.Now().Unix()
	testhub.SSHExec(trigger.TriggerRegexSingle(100, 100, "simple.log"))
	// Verify
	if err := verify.VerifyLogCount(100, int32(startTime), int32(time.Now().Unix())); err != nil {
		t.Error(err)
	}
	if err := verify.VerifyRegexSingle(int32(startTime), int32(time.Now().Unix())); err != nil {
		t.Error(err)
	}
	// Cleanup
	control.RemoveLocalConfig("regex_single")
	cleanup.CleanupGeneratedLog()
}
