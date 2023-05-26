package trigger

import (
	"time"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/window"
)

func NewRTTTrigger(threshold time.Duration) FeedTrigger {
	scale := time.Millisecond
	return &rttTrigger{
		threshold: threshold / scale,
		scale:     scale,
		rtt:       window.NewEMASampleWindow(10*time.Second.Seconds()*2, 10),
	}
}

type rttTrigger struct {
	threshold time.Duration
	scale     time.Duration

	rtt window.SampleWindow
}

// Feed record value as samples
// value should be time duration in nanos
func (r *rttTrigger) Feed(value float64) {
	r.rtt.Add(value / float64(r.scale))
}

func (r *rttTrigger) Open() bool {
	return r.rtt.Get() > float64(r.threshold)
}

func (r *rttTrigger) Stop() {}
