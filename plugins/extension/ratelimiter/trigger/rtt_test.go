package trigger

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestRTTTrigger(t *testing.T) {
	trigger := NewRTTTrigger(time.Second)
	trigger.Feed(float64(time.Millisecond))
	assert.False(t, trigger.Open())

	trigger.Feed(float64(time.Minute))
	assert.True(t, trigger.Open())
}
