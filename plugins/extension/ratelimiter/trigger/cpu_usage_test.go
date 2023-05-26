package trigger

import (
	"testing"
	"time"
)

func TestCPUUUsageTrigger(t *testing.T) {
	trigger := NewCPUUsageTrigger(80)
	time.Sleep(time.Second)
	trigger.Stop()
	time.Sleep(time.Second)
}
