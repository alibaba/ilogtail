package window

import (
	"fmt"
	"math"
	"sync"
	"sync/atomic"
	"time"
)

const (
	AggregateOpMin AggregateOp = iota
	AggregateOpMax
)

const (
	BucketOpAvg BucketOp = iota
	BucketOpSum
	BucketOpCount
)

func NewBucketWindow(window time.Duration, buckets int, agg AggregateOp, bucketOp BucketOp, precision time.Duration) SampleWindow {
	bucketDuration := window / time.Duration(buckets)
	if precision == 0 {
		precision = bucketDuration
	}
	return &bucketSampleWindow{
		buckets:        make([]bucket, buckets),
		size:           buckets,
		aggOp:          agg,
		bucketOp:       bucketOp,
		scale:          float64(precision) / float64(bucketDuration),
		bucketDuration: bucketDuration,
		lastUpdateTime: time.Now(),
	}
}

type AggregateOp int

type BucketOp int

type bucket struct {
	sum   float64
	count int64
}

func (b *bucket) reset() {
	b.sum = 0
	b.count = 0
}

func (b *bucket) add(value float64) {
	b.sum += value
	b.count++
}

func (b *bucket) get(op BucketOp) float64 {
	if b.count == 0 {
		return 0
	}
	switch op {
	case BucketOpAvg:
		return b.sum / float64(b.count)
	case BucketOpCount:
		return float64(b.count)
	case BucketOpSum:
		return b.sum
	default:
		panic(fmt.Sprintf("not supported op: %v", op))
	}
}

type bucketSampleWindow struct {
	bucketDuration time.Duration
	buckets        []bucket
	size           int
	aggOp          AggregateOp
	bucketOp       BucketOp
	scale          float64
	lastOffset     int
	lastUpdateTime time.Time

	lastAggResult uint64 // use uint64 store float64 bits
	// lastAggTime   time.Time
	lock sync.RWMutex
}

func (b *bucketSampleWindow) Add(value float64) {
	b.lock.Lock()
	defer b.lock.Unlock()

	now := time.Now()
	elapsed := int(now.Sub(b.lastUpdateTime) / b.bucketDuration)
	if elapsed < 0 {
		// time backwards
		elapsed = b.size
	}
	updateTime := time.Duration(elapsed) * b.bucketDuration
	if elapsed > 0 {
		if elapsed > b.size {
			elapsed = b.size
		}
		// reset expired time buckets
		for i := 1; i <= elapsed; i++ {
			b.buckets[(b.lastOffset+i)%b.size].reset()
		}
		b.lastOffset = (b.lastOffset + elapsed) % b.size
	}
	b.buckets[b.lastOffset].add(value)
	b.lastUpdateTime = b.lastUpdateTime.Add(updateTime)
}

func (b *bucketSampleWindow) Get() float64 {
	b.lock.RLock()
	defer b.lock.RUnlock()

	// lastResult := math.Float64frombits(atomic.LoadUint64(&b.lastAggResult))
	// if lastResult > 0 {
	// 	now := time.Now()
	// 	// todo: b.lastAggTime has race
	// 	elapsed := int(now.Sub(b.lastAggTime) / b.bucketDuration)
	// 	if elapsed == 0 {
	// 		return lastResult
	// 	}
	// }

	var lastResult float64

	for i := 0; i < b.size; i++ {
		if b.buckets[i].count == 0 {
			continue
		}
		bv := b.buckets[i].get(b.bucketOp)
		switch b.aggOp {
		case AggregateOpMax:
			if bv > lastResult {
				lastResult = bv
			}
		case AggregateOpMin:
			if bv < lastResult || lastResult == 0 {
				lastResult = bv
			}
		default:
			panic(fmt.Sprintf("unsupported aggregate Op: %v", b.aggOp))
		}
	}

	if lastResult == 0 {
		return lastResult
	}

	if b.scale > 0 {
		lastResult *= b.scale
	}

	atomic.StoreUint64(&b.lastAggResult, math.Float64bits(lastResult))
	// b.lastAggTime = time.Now()
	return lastResult
}
