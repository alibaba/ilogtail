package plugin

import "testing"

const (
	TEST_BEHAVIOR    = "behavior"
	TEST_CORE        = "core"
	TEST_PERFORMANCE = "performance"
)

func TestFlusherLoki(t *testing.T) {
	RunPluginE2E("flusher_loki", TEST_BEHAVIOR, false, false)
}
