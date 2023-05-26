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

func NewGradient2RateLimiter(ops ...Option) RateLimiter {
	conf := &config{maxLimit: math.MaxInt, minLimit: 20, initialLimit: 100}
	for _, op := range ops {
		op(conf)
	}

	l := &gradient2RateLimiter{
		ctx:            make(chan struct{}, 1),
		lastRtt:        window.NewBucketWindow(time.Second, 10, window.AggregateOpAvg, window.BucketOpAvg, time.Second), // avg on last 1 second
		longRtt:        window.NewEMASampleWindow(5*time.Minute.Seconds(), 10),                                          // count for last 5 minutes
		shortRtt:       window.NewEMASampleWindow(5*time.Second.Seconds(), 10),                                          // count for last 5 seconds
		estimatedLimit: int64(conf.initialLimit),
		initLimit:      int64(conf.initialLimit),
		minLimit:       int64(conf.minLimit),
		maxLimit:       int64(conf.maxLimit),
		smoothing:      0.2,
		tolerance:      1.5,
		trigger:        conf.trigger,
		urgentDuration: 10 * time.Second,
		queueSize: func(v float64) float64 {
			switch {
			case v <= 2:
				return 0.5
			case v < 10:
				return 1
			case v < 20:
				return 2
			default:
				return 4
			}
		},
	}
	go l.daemon()
	return l
}

type gradient2RateLimiter struct {
	// current inflight number
	inflight int64

	/**
	     * Estimated concurrency limit based on our algorithm
		 * Need volatile
	*/
	estimatedLimit int64

	/**
	 * Tracks a measurement of the realtime rtt, consider the concurrent requests, realtime meant the average rtt in a
	 * very short time window, by default, it's 1 second
	 */
	lastRtt window.SampleWindow

	/**
	 * Tracks a measurement of the short time, and more volatile, RTT meant to represent the current system latency
	 */
	shortRtt window.SampleWindow

	/**
	 * Tracks a measurement of the long term, less volatile, RTT meant to represent the baseline latency.  When the system
	 * is under load gl number is expect to trend higher.
	 */
	longRtt window.SampleWindow

	/**
	 * Maximum allowed limit providing an upper bound failsafe
	 */
	initLimit, minLimit, maxLimit int64

	queueSize func(v float64) float64

	smoothing float64
	tolerance float64

	/**
	 * trigger related
	 */
	trigger        trigger.Trigger
	urgentDuration time.Duration
	lastDropTime   int64 // timestamp in nanos

	ctx chan struct{}
}

func (g *gradient2RateLimiter) Allow() (bool, DoneFunc) {
	if g.shouldDrop() {
		return false, nil
	}
	atomic.AddInt64(&g.inflight, 1)
	start := time.Now().UnixNano()
	return true, func(err error) time.Duration {
		atomic.AddInt64(&g.inflight, -1)
		rttNanos := time.Duration(time.Now().UnixNano() - start)
		g.lastRtt.Add(float64(rttNanos))
		return rttNanos
	}
}

func (g *gradient2RateLimiter) Stop() {
	g.ctx <- struct{}{}
}

func (g *gradient2RateLimiter) shouldDrop() bool {
	inflight := atomic.LoadInt64(&g.inflight)
	now := time.Now().UnixNano()
	inUrgentState := time.Duration(now-atomic.LoadInt64(&g.lastDropTime)) <= g.urgentDuration
	// if last drop triggered in short time ago, we continue the adaptive limit calculation
	if !inUrgentState && g.trigger != nil && !g.trigger.Open() {
		return inflight > g.maxLimit
	}

	drop := g.inflight > atomic.LoadInt64(&g.estimatedLimit)
	if drop && !inUrgentState {
		atomic.StoreInt64(&g.lastDropTime, now)
	}
	return drop
}

// daemon will calculate the new limit per second
func (g *gradient2RateLimiter) daemon() {
	ticker := time.NewTicker(time.Second)
	defer ticker.Stop()
	for {
		select {
		case <-g.ctx:
			return
		case <-ticker.C:
			g.updateLimit()
		}
	}
}

func (g *gradient2RateLimiter) updateLimit() {
	rtt := g.lastRtt.Get()
	g.longRtt.Add(rtt)
	g.shortRtt.Add(rtt)

	longRtt := g.longRtt.Get()
	shortRtt := g.shortRtt.Get()
	if shortRtt == 0 {
		return
	}

	appLimited := atomic.LoadInt64(&g.inflight) < (atomic.LoadInt64(&g.estimatedLimit)+1)/2.0
	if appLimited {
		return
	}

	// If the long RTT is substantially larger than the short RTT then reduce the long RTT measurement.
	// gl can happen when latency returns to normal after a prolonged prior of excessive load.  Reducing the
	// long RTT without waiting for the exponential smoothing helps bring the system back to steady state.
	if longRtt/shortRtt > 2 {
		setter, ok := g.longRtt.(window.Setter)
		if ok {
			setter.Set(func(current float64) float64 {
				return current * 0.95
			})
		}
		longRtt = g.longRtt.Get()
	}

	// Rtt could be higher than rtt_noload because of smoothing rtt noload updates
	// so set to 1.0 to indicate no queuing.  Otherwise calculate the slope and don't
	// allow it to be reduced by more than half to avoid aggressive load-shedding due to
	// outliers.
	gradient := math.Max(0.5, math.Min(1, g.tolerance*longRtt/shortRtt))
	estimated := float64(atomic.LoadInt64(&g.estimatedLimit))

	newLimit := estimated*gradient + g.queueSize(estimated)
	newLimit = estimated*(1-g.smoothing) + newLimit*g.smoothing
	newLimit = math.Max(float64(g.minLimit), math.Min(float64(g.maxLimit), newLimit))
	newLimit = math.Ceil(newLimit)

	atomic.StoreInt64(&g.estimatedLimit, int64(newLimit))

	if logger.DebugFlag() {
		logger.Debugf(context.Background(), "[ratelimiter] update estimateLimit: %d, inflight: %d, lastRtt: %d, longRtt: %d, shortRtt: %d, minLimit: %d, maxLimit: %d, oldLimit: %d",
			newLimit, atomic.LoadInt64(&g.inflight), rtt, longRtt, shortRtt, g.minLimit, g.maxLimit, estimated)
	}
}
