package limiter

import (
	"math"
	"sync/atomic"
	"time"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/window"
)

func NewBBRRateLimiter(ops ...Option) RateLimiter {
	conf := &config{maxInflight: math.MaxInt}
	for _, op := range ops {
		op(conf)
	}

	return &bbrRateLimiter{
		maxDelivered:   window.NewBucketWindow(time.Second*10, 100, window.AggregateOpMax, window.BucketOpSum, time.Second),
		minRTT:         window.NewBucketWindow(time.Second*10, 100, window.AggregateOpMin, window.BucketOpAvg, 0),
		maxInflight:    int64(conf.maxInflight),
		urgentDuration: time.Second,
		trigger:        conf.trigger,
	}
}

type bbrRateLimiter struct {
	inflight       int64
	maxDelivered   window.SampleWindow // delivered per second
	minRTT         window.SampleWindow // rtt in milliseconds
	trigger        trigger.Trigger
	maxInflight    int64
	urgentDuration time.Duration
	lastDropTime   int64 // timestamp in nanos
}

func (r *bbrRateLimiter) Allow() (bool, DoneFunc) {
	if r.shouldDrop() {
		return false, nil
	}
	atomic.AddInt64(&r.inflight, 1)
	start := time.Now().UnixNano()
	return true, func(err error) time.Duration {
		atomic.AddInt64(&r.inflight, -1)
		rttNanos := time.Now().UnixNano() - start
		if rttNanos > 0 { // filter time backwards
			r.minRTT.Add(math.Ceil(float64(rttNanos) / float64(time.Millisecond)))
		}
		r.maxDelivered.Add(1)
		return time.Duration(rttNanos)
	}
}

func (r *bbrRateLimiter) shouldDrop() bool {
	now := time.Now().UnixNano()
	inUrgentState := time.Duration(now-atomic.LoadInt64(&r.lastDropTime)) <= r.urgentDuration
	// if last drop triggered in one second
	if !inUrgentState && r.trigger != nil && !r.trigger.Open() {
		return false
	}

	inflight := atomic.LoadInt64(&r.inflight)
	maxInflight := r.calcMaxInflight()
	drop := inflight > maxInflight
	if drop && !inUrgentState {
		atomic.StoreInt64(&r.lastDropTime, now)
	}
	return drop
}

func (r *bbrRateLimiter) calcMaxInflight() int64 {
	delivered := r.maxDelivered.Get()
	minRTT := r.minRTT.Get()
	if delivered == 0 && minRTT == 0 {
		return int64(math.Min(math.MaxInt, float64(r.maxInflight)))
	}
	max := math.Min(math.Max(math.Ceil(delivered*minRTT/1000.0), 1), float64(r.maxInflight))
	return int64(max)
}
