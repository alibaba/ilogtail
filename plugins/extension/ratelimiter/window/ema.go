package window

import "sync"

func NewEMASampleWindow(decay float64, warmup int) SampleWindow {
	if decay <= 0 || decay > 1 {
		panic("decay should be in range of (0,1]")
	}
	return &emaSampleWindow{
		warmup: warmup,
		decay:  decay,
	}
}

type emaSampleWindow struct {
	value  float64
	sum    float64
	count  int
	warmup int
	decay  float64

	lock sync.RWMutex
}

func (e *emaSampleWindow) Add(value float64) {
	e.lock.Lock()
	defer e.lock.Unlock()

	if e.count < e.warmup {
		e.count++
		e.sum += value
		e.value = e.sum / float64(e.count)
		return
	}
	e.value = e.value*(1.0-e.decay) + value*e.decay
}

func (e *emaSampleWindow) Get() float64 {
	e.lock.RLock()
	defer e.lock.RUnlock()
	return e.value
}
