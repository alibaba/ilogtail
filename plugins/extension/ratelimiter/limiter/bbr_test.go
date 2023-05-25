package limiter

import (
	"fmt"
	"math"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

func TestNewBBRRateLimiter(t *testing.T) {
	l := NewBBRRateLimiter(WithMaxLimit(1000), WithTrigger(trigger.NewRTTTrigger(time.Second))).(*bbrRateLimiter)
	assert.NotNil(t, l)
	assert.NotNil(t, l.maxDelivered)
	assert.NotNil(t, l.minRTT)
	assert.NotNil(t, l.trigger)
	assert.EqualValues(t, 1000, l.maxLimit)

	allow, done := l.Allow()
	assert.True(t, allow)
	assert.NotNil(t, done)
}

func TestBBRRateLimiter_Allow(t *testing.T) {
	l := NewBBRRateLimiter(WithTrigger(mockTrigger{})).(*bbrRateLimiter)

	allow, done := l.Allow()
	time.Sleep(time.Millisecond)
	assert.EqualValues(t, 100, l.estimatedLimit)
	assert.True(t, allow)
	done(nil)

	l.updateLimit()
	minRTT := l.minRTT.Get()
	maxDelivered := l.maxDelivered.Get()
	assert.Greater(t, minRTT, float64(0))
	assert.Greater(t, maxDelivered, float64(0))

	l.updateLimit()
	fmt.Println(maxDelivered, minRTT, l.estimatedLimit)
	assert.NotEqualValues(t, 0, l.estimatedLimit)
	assert.NotEqualValues(t, math.MaxInt, l.estimatedLimit)
}

type mockTrigger struct {
	trigger.Trigger
}

func (receiver mockTrigger) Open() bool {
	return true
}
