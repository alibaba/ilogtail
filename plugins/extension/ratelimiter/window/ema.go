package window

import "sync"

// NewEMASampleWindow creates a sample window based on the ema algorithm.
// age means the max period which one sample take effect in the ema result,
// for example, if you have a time-series with samples once per second,
// and you want to get the moving average over the previous minute, you should
// use an age of 60.
// for best results, the moving average should not be initialized to the
// samples it sees immediately. The book "Production and Operations
// Analysis" by Steven Nahmias suggests initializing the moving average to
// the mean of the first 10 samples. Until the VariableEwma has seen this
// many samples, it is not "ready" to be queried for the value of the
// moving average.
func NewEMASampleWindow(age float64, warmup int) SampleWindow {
	// The formula to compute the alpha required for EMA is: alpha = 2/(N+1).
	// Proof is in the book "Production and Operations Analysis" by Steven Nahmias
	decay := 2 / (age + 1)
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

func (e *emaSampleWindow) Set(fn func(float64) float64) {
	e.lock.Lock()
	defer e.lock.Unlock()
	v := fn(e.value)
	e.value = v
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
