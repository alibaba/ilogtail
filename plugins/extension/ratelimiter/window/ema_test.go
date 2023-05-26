package window

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestEMAWindow(t *testing.T) {
	w := NewEMASampleWindow(10, 10).(*emaSampleWindow)
	for i := 0; i < 10; i++ {
		w.Add(200)
	}
	assert.EqualValues(t, 200*10, w.sum)
	assert.EqualValues(t, 10, w.count)
	assert.EqualValues(t, 200, w.value)
	assert.EqualValues(t, 200, w.Get())

	w.Add(100)
	decay := 2 / (float64(10) + 1)
	assert.EqualValues(t, w.Get(), 200*(1-decay)+100*decay)
}
