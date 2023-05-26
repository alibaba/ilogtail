package trigger

import (
	"time"

	"github.com/shirou/gopsutil/cpu"

	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/window"
)

// NewCPUUsageTrigger create trigger based on cpu monitor
// threshold range from (1~100)
func NewCPUUsageTrigger(threshold int) Trigger {
	t := &cpuUsageTrigger{
		threshold: float64(threshold * 10),
		interval:  time.Millisecond * 500,
		scale:     10,
		ctx:       make(chan struct{}, 1),
		usage:     window.NewEMASampleWindow(10*time.Second.Seconds()*2, 10),
	}
	go t.start()
	return t
}

type cpuUsageTrigger struct {
	threshold float64
	interval  time.Duration
	scale     int
	ctx       chan struct{}

	usage window.SampleWindow
}

func (c *cpuUsageTrigger) Open() bool {
	return c.usage.Get() >= c.threshold
}

func (c *cpuUsageTrigger) Stop() {
	c.ctx <- struct{}{}
}

func (c *cpuUsageTrigger) start() {
	ticker := time.NewTicker(c.interval)
	defer ticker.Stop()
	for {
		select {
		case <-c.ctx:
			return
		case <-ticker.C:
		}

		percents, err := cpu.Percent(c.interval, false)
		if err != nil {
			continue
		}

		percent := percents[0] * float64(c.scale)
		c.usage.Add(percent)
	}
}
