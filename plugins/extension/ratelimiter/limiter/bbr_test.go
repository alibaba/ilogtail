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

	maxInflight := l.calcLimit()
	allow, done := l.Allow()
	time.Sleep(time.Millisecond)
	assert.EqualValues(t, math.MaxInt, maxInflight)
	assert.True(t, allow)
	done(nil)

	minRTT := l.minRTT.Get()
	maxDelivered := l.maxDelivered.Get()
	assert.Greater(t, minRTT, float64(0))
	assert.Greater(t, maxDelivered, float64(0))

	maxInflight = l.calcLimit()
	fmt.Println(maxDelivered, minRTT, maxInflight)
	assert.NotEqualValues(t, 0, maxInflight)
	assert.NotEqualValues(t, math.MaxInt, maxInflight)
}

func TestBBRRateLimiter_Allow_MaxInflight(t *testing.T) {
	l := NewBBRRateLimiter(WithMaxLimit(10), WithTrigger(mockTrigger{})).(*bbrRateLimiter)

	maxInflight := l.calcLimit()
	allow, done := l.Allow()
	assert.EqualValues(t, 10, maxInflight)
	assert.True(t, allow)
	done(nil)

	minRTT := l.minRTT.Get()
	maxDelivered := l.maxDelivered.Get()
	assert.Greater(t, minRTT, float64(0))
	assert.Greater(t, maxDelivered, float64(0))

	maxInflight = l.calcLimit()
	fmt.Println(maxDelivered, minRTT, maxInflight)
	assert.NotEqualValues(t, 0, maxInflight)
	assert.NotEqualValues(t, math.MaxInt, maxInflight)
}

type mockTrigger struct {
	trigger.Trigger
}

func (receiver mockTrigger) Open() bool {
	return true
}
