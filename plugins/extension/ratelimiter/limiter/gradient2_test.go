package limiter

import (
	"fmt"
	"math"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

func TestNewGradient2RateLimiter(t *testing.T) {
	l := NewGradient2RateLimiter(
		WithTrigger(trigger.NewRTTTrigger(time.Second)),
	).(*gradient2RateLimiter)
	assert.NotNil(t, l)
	assert.NotNil(t, l.lastRtt)
	assert.NotNil(t, l.shortRtt)
	assert.NotNil(t, l.longRtt)
	assert.NotNil(t, l.trigger)
	assert.EqualValues(t, 20, l.minLimit)
	assert.EqualValues(t, 100, l.initLimit)
	assert.EqualValues(t, math.MaxInt, l.maxLimit)

	l = NewGradient2RateLimiter(
		WithMaxLimit(1000),
		WithTrigger(trigger.NewRTTTrigger(time.Second)),
		WithMinLimit(100),
	).(*gradient2RateLimiter)
	assert.NotNil(t, l)
	assert.NotNil(t, l.shortRtt)
	assert.NotNil(t, l.longRtt)
	assert.NotNil(t, l.trigger)
	assert.EqualValues(t, 100, l.minLimit)
	assert.EqualValues(t, 1000, l.maxLimit)

	allow, done := l.Allow()
	assert.True(t, allow)
	assert.NotNil(t, done)
}

func TestGradient2RateLimiter_Allow(t *testing.T) {
	l := NewGradient2RateLimiter(
		WithMinLimit(1),
		WithInitialLimit(20),
		WithTrigger(mockTrigger{}),
	).(*gradient2RateLimiter)

	allow, done := l.Allow()
	time.Sleep(time.Millisecond)
	assert.True(t, allow)
	done(nil)

	l.updateLimit()
	longRTT := l.longRtt.Get()
	shortRTT := l.shortRtt.Get()
	assert.Greater(t, longRTT, float64(0))
	assert.Greater(t, shortRTT, float64(0))

	// app limited
	estimated := l.estimatedLimit
	fmt.Println(estimated)
	for i := 0; i < 9; i++ {
		allow, _ = l.Allow()
		assert.True(t, allow)
	}
	l.updateLimit()
	assert.EqualValues(t, estimated, l.estimatedLimit)

	// limit should grow if in tolerance and not in app-limited
	for i := 0; i < 10; i++ {
		allow, done = l.Allow()
		assert.True(t, allow)
		time.Sleep(time.Millisecond)
		if i > 5 {
			done(nil)
		}
		l.updateLimit()
	}
	assert.Greater(t, l.estimatedLimit, estimated)
}

func BenchmarkGradient2RateLimiter_Allow(b *testing.B) {
	l := NewGradient2RateLimiter(
		WithMinLimit(1),
		WithInitialLimit(20),
		WithTrigger(mockTrigger{}),
	).(*gradient2RateLimiter)

	allow, done := l.Allow()
	assert.True(b, allow)
	var total, rejected int64
	for i := 0; i < b.N; i++ {
		if done != nil {
			done(nil)
		}
		allow, done = l.Allow()
		total++
		if !allow {
			rejected++
		}
	}
	fmt.Println(total, rejected)
}
