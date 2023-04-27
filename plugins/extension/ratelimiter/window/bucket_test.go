package window

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestMaxBucketWindow(t *testing.T) {
	w := NewBucketWindow(time.Second*10, 20, AggregateOpMax, BucketOpAvg, time.Millisecond*500).(*bucketSampleWindow)
	assert.NotNil(t, w)
	assert.Len(t, w.buckets, 20)
	assert.Equal(t, time.Millisecond*500, w.bucketDuration)
	assert.Equal(t, AggregateOpMax, w.aggOp)
	assert.Equal(t, 20, w.size)

	for i := 0; i < 10; i++ {
		w.Add(1)
	}
	assert.EqualValues(t, float64(10), w.buckets[0].sum)
	assert.EqualValues(t, int64(10), w.buckets[0].count)
	assert.Equal(t, float64(1), w.Get())

	time.Sleep(time.Millisecond * 600)
	w.Add(2)
	assert.EqualValues(t, 2, w.buckets[1].sum)
	assert.EqualValues(t, 1, w.buckets[1].count)

	time.Sleep(time.Second)
	w.Add(3)
	assert.EqualValues(t, 3, w.buckets[3].sum)
	assert.EqualValues(t, 1, w.buckets[3].count)

	assert.Equal(t, float64(3), w.Get())
}

func TestMaxBucketWindowPrecision(t *testing.T) {
	w := NewBucketWindow(time.Second*10, 20, AggregateOpMax, BucketOpAvg, time.Second).(*bucketSampleWindow)
	assert.NotNil(t, w)
	assert.Len(t, w.buckets, 20)
	assert.Equal(t, time.Millisecond*500, w.bucketDuration)
	assert.Equal(t, AggregateOpMax, w.aggOp)
	assert.Equal(t, 20, w.size)
	assert.EqualValues(t, 2, w.scale)

	for i := 0; i < 10; i++ {
		w.Add(1)
	}
	assert.EqualValues(t, float64(10), w.buckets[0].sum)
	assert.EqualValues(t, int64(10), w.buckets[0].count)
	assert.Equal(t, float64(2), w.Get())

	time.Sleep(time.Millisecond * 600)
	w.Add(2)
	assert.EqualValues(t, 2, w.buckets[1].sum)
	assert.EqualValues(t, 1, w.buckets[1].count)

	time.Sleep(time.Second)
	w.Add(3)
	assert.EqualValues(t, 3, w.buckets[3].sum)
	assert.EqualValues(t, 1, w.buckets[3].count)

	assert.Equal(t, float64(6), w.Get())
}

func TestMaxSumBucketWindow(t *testing.T) {
	w := NewBucketWindow(time.Second*10, 20, AggregateOpMax, BucketOpSum, time.Second).(*bucketSampleWindow)
	assert.NotNil(t, w)
	assert.Len(t, w.buckets, 20)
	assert.Equal(t, time.Millisecond*500, w.bucketDuration)
	assert.Equal(t, AggregateOpMax, w.aggOp)
	assert.Equal(t, 20, w.size)
	assert.EqualValues(t, 2, w.scale)
	assert.Equal(t, BucketOpSum, w.bucketOp)

	for i := 0; i < 10; i++ {
		w.Add(1)
	}
	assert.EqualValues(t, float64(10), w.buckets[0].sum)
	assert.EqualValues(t, int64(10), w.buckets[0].count)
	assert.Equal(t, float64(20), w.Get())
}
