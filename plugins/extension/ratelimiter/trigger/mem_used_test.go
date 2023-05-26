package trigger

import (
	"testing"
	"time"
)

func TestMEMUsedTrigger(t *testing.T) {
	trigger := NewMemUsedTrigger(10)
	time.Sleep(time.Second)
	trigger.Stop()
}
