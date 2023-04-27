package limiter

import (
	"fmt"
	"math"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
)

func TestNewBBRRateLimiter(t *testing.T) {
	l := NewBBRRateLimiter(WithMaxInflight(1000), WithTrigger(trigger.NewRTTTrigger(time.Second))).(*bbrRateLimiter)
	assert.NotNil(t, l)
	assert.NotNil(t, l.maxDelivered)
	assert.NotNil(t, l.minRTT)
	assert.NotNil(t, l.trigger)
	assert.EqualValues(t, 1000, l.maxInflight)

	allow, done := l.Allow()
	assert.True(t, allow)
	assert.NotNil(t, done)
}

func TestBBRRateLimiter_Allow(t *testing.T) {
	l := NewBBRRateLimiter(WithTrigger(mockTrigger{})).(*bbrRateLimiter)

	maxInflight := l.calcMaxInflight()
	allow, done := l.Allow()
	assert.EqualValues(t, math.MaxInt, maxInflight)
	assert.True(t, allow)
	done(nil)

	minRTT := l.minRTT.Get()
	maxDelivered := l.maxDelivered.Get()
	assert.Greater(t, minRTT, float64(0))
	assert.Greater(t, maxDelivered, float64(0))

	maxInflight = l.calcMaxInflight()
	fmt.Println(maxDelivered, minRTT, maxInflight)
	assert.NotEqualValues(t, 0, maxInflight)
	assert.NotEqualValues(t, math.MaxInt, maxInflight)
}

func TestBBRRateLimiter_Allow_MaxInflight(t *testing.T) {
	l := NewBBRRateLimiter(WithMaxInflight(10), WithTrigger(mockTrigger{})).(*bbrRateLimiter)

	maxInflight := l.calcMaxInflight()
	allow, done := l.Allow()
	assert.EqualValues(t, 10, maxInflight)
	assert.True(t, allow)
	done(nil)

	minRTT := l.minRTT.Get()
	maxDelivered := l.maxDelivered.Get()
	assert.Greater(t, minRTT, float64(0))
	assert.Greater(t, maxDelivered, float64(0))

	maxInflight = l.calcMaxInflight()
	fmt.Println(maxDelivered, minRTT, maxInflight)
	assert.NotEqualValues(t, 0, maxInflight)
	assert.NotEqualValues(t, math.MaxInt, maxInflight)
}

func TestBBRRateLimiter_Allow_Simulate(t *testing.T) {
	l := NewBBRRateLimiter(WithTrigger(trigger.NewRTTTrigger(time.Millisecond * 20))).(*bbrRateLimiter)

	var deliveredRate int64

	go func() {
		for range time.Tick(time.Second) {
			rate := atomic.SwapInt64(&deliveredRate, 0)
			fmt.Println("======== request rate:", rate)
		}
	}()

	var dropped int64
	var wg sync.WaitGroup
	for i := 0; i < 1000000; i++ {
		if i%10 == 0 {
			time.Sleep(time.Millisecond * 10)
		}
		v := i
		wg.Add(1)
		go func() {
			defer wg.Done()
			allow, done := l.Allow()
			if !allow {
				atomic.AddInt64(&dropped, 1)
				return
			}
			time.Sleep(time.Millisecond * 10 * time.Duration(v))
			if done != nil {
				done(nil)
			}
			atomic.AddInt64(&deliveredRate, 1)
		}()
	}

	wg.Wait()

	fmt.Println(dropped)
}

type mockTrigger struct {
	trigger.Trigger
}

func (receiver mockTrigger) Open() bool {
	return true
}
