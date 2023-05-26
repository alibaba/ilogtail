package trigger

import (
	"context"
	"os"
	"time"

	"github.com/shirou/gopsutil/process"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/plugins/extension/ratelimiter/window"
)

// NewMemUsedTrigger create a Trigger based on process mem monitor
// unit of threshold is MB
func NewMemUsedTrigger(threshold float64) Trigger {
	t := &memUsedTrigger{
		threshold: threshold,
		scale:     1024 * 1024,
		interval:  time.Second,
		ctx:       make(chan struct{}, 1),
		used:      window.NewEMASampleWindow(10*time.Second.Seconds()*2, 10),
	}
	go t.start()
	return t
}

type memUsedTrigger struct {
	threshold float64
	interval  time.Duration
	scale     int
	ctx       chan struct{}

	used window.SampleWindow
}

func (m *memUsedTrigger) Open() bool {
	return m.used.Get() >= m.threshold
}

func (m *memUsedTrigger) Stop() {
	m.ctx <- struct{}{}
}

func (m *memUsedTrigger) start() {
	pid := os.Getpid()
	p, err := process.NewProcess(int32(pid))
	if err != nil {
		logger.Warning(context.Background(), "RATELIMITER_TRIGGER_ALARM", "failed to get process id", err)
		return
	}
	ticker := time.NewTicker(m.interval)
	defer ticker.Stop()
	for {
		select {
		case <-m.ctx:
			return
		case <-ticker.C:
		}

		mem, err := p.MemoryInfo()
		if err != nil {
			continue
		}

		rss := mem.RSS / uint64(m.scale)
		m.used.Add(float64(rss))
	}
}
