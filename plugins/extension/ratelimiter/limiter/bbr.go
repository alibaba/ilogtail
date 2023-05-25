package limiter

import (
	"context"
	"math"
	"sync/atomic"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/trigger"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/window"
)

func NewBBRRateLimiter(ops ...Option) RateLimiter {
	conf := &config{maxLimit: math.MaxInt, minLimit: 20, initialLimit: 100}
	for _, op := range ops {
		op(conf)
	}

	l := &bbrRateLimiter{
		maxDelivered:   window.NewBucketWindow(time.Minute*5, 100, window.AggregateOpMax, window.BucketOpSum, time.Second),
		minRTT:         window.NewBucketWindow(time.Minute*5, 100, window.AggregateOpMin, window.BucketOpAvg, 0),
		estimatedLimit: int64(conf.initialLimit),
		initLimit:      int64(conf.initialLimit),
		maxLimit:       int64(conf.maxLimit),
		minLimit:       int64(conf.minLimit),
		urgentDuration: time.Second,
		trigger:        conf.trigger,
		ctx:            make(chan struct{}, 1),
	}
	go l.daemon()
	return l
}

type bbrRateLimiter struct {
	inflight                      int64
	estimatedLimit                int64
	maxDelivered                  window.SampleWindow // delivered per second
	minRTT                        window.SampleWindow // rtt in milliseconds
	trigger                       trigger.Trigger
	maxLimit, initLimit, minLimit int64
	urgentDuration                time.Duration
	lastDropTime                  int64 // timestamp in nanos

	ctx chan struct{}
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

func (r *bbrRateLimiter) Stop() {
	r.ctx <- struct{}{}
}

func (r *bbrRateLimiter) shouldDrop() bool {
	inflight := atomic.LoadInt64(&r.inflight)
	now := time.Now().UnixNano()
	inUrgentState := time.Duration(now-atomic.LoadInt64(&r.lastDropTime)) <= r.urgentDuration
	// if last drop triggered in short time ago, we continue the adaptive limit calculation
	if !inUrgentState && r.trigger != nil && !r.trigger.Open() {
		return inflight > r.maxLimit
	}

	drop := inflight > atomic.LoadInt64(&r.estimatedLimit)
	if drop && !inUrgentState {
		atomic.StoreInt64(&r.lastDropTime, now)
	}
	return drop
}

// daemon will calculate the new limit per second
func (r *bbrRateLimiter) daemon() {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	for {
		select {
		case <-r.ctx:
			return
		case <-ticker.C:
			r.updateLimit()
		}
	}
}

func (r *bbrRateLimiter) updateLimit() {
	delivered := r.maxDelivered.Get()
	minRTT := r.minRTT.Get()
	if delivered == 0 || minRTT == 0 {
		return
	}
	limit := math.Ceil(delivered * (minRTT / 1000.0))
	limit = math.Max(float64(r.minLimit), math.Min(float64(r.maxLimit), limit))
	atomic.StoreInt64(&r.estimatedLimit, int64(limit))

	if logger.DebugFlag() {
		logger.Debugf(context.Background(), "[ratelimiter] update estimatedLimit: %d, inflight: %d, maxDeliverPerSecond: %d, minRTTInMillis: %d, minLimit: %d, maxLimit: %d",
			limit, atomic.LoadInt64(&r.inflight), delivered, minRTT, r.minLimit, r.maxLimit)
	}
}
