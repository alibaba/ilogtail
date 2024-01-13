// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package ratelimit

import (
	"context"
	"math"
	"sync"
	"sync/atomic"
	"time"

	"github.com/alibaba/ilogtail/pkg/logger"
)

type bucket struct {
	tokens        float64
	lastReplenish time.Time
}

type tokenBucket struct {
	mu sync.Mutex

	limit   rate
	buckets sync.Map

	// GC thresholds and metrics
	gc struct {
		thresholds tokenBucketGCConfig
		metrics    struct {
			numCalls atomic.Int32
		}
	}
}

type tokenBucketGCConfig struct {
	// NumCalls is the number of calls made to IsAllowed. When more than
	// the specified number of calls are made, GC is performed.
	NumCalls int32 `config:"num_calls"`
}

type tokenBucketConfig struct {

	// GC governs when completely filled token buckets must be deleted
	// to free up memory. GC is performed when _any_ of the GC conditions
	// below are met. After each GC, counters corresponding to _each_ of
	// the GC conditions below are reset.
	GC tokenBucketGCConfig `config:"gc"`
}

func newTokenBucket(rate rate) algorithm {
	cfg := tokenBucketConfig{
		GC: tokenBucketGCConfig{
			NumCalls: 10000,
		},
	}

	return &tokenBucket{
		limit:   rate,
		buckets: sync.Map{},
		gc: struct {
			thresholds tokenBucketGCConfig
			metrics    struct {
				numCalls atomic.Int32
			}
		}{
			thresholds: tokenBucketGCConfig{
				NumCalls: cfg.GC.NumCalls,
			},
		},
		mu: sync.Mutex{},
	}
}

func (t *tokenBucket) IsAllowed(key uint64) bool {
	t.runGC()

	b := t.getBucket(key)
	allowed := b.withdraw()

	t.gc.metrics.numCalls.Add(1)
	return allowed
}

func (t *tokenBucket) getBucket(key uint64) *bucket {
	v, exists := t.buckets.LoadOrStore(key, &bucket{
		tokens:        t.limit.value,
		lastReplenish: time.Now(),
	})
	b := v.(*bucket)

	if exists {
		b.replenish(t.limit)
		return b
	}

	return b
}

func (b *bucket) withdraw() bool {
	if b.tokens < 1 {
		return false
	}
	b.tokens--
	return true
}

func (b *bucket) replenish(rate rate) {
	secsSinceLastReplenish := time.Since(b.lastReplenish).Seconds()
	tokensToReplenish := secsSinceLastReplenish * rate.valuePerSecond()

	b.tokens = math.Min(b.tokens+tokensToReplenish, rate.value)
	b.lastReplenish = time.Now()
}

func (t *tokenBucket) runGC() {
	// Don't run GC if thresholds haven't been crossed.
	if t.gc.metrics.numCalls.Load() < t.gc.thresholds.NumCalls {
		return
	}

	if !t.mu.TryLock() {
		return
	}

	go func() {
		defer t.mu.Unlock()
		gcStartTime := time.Now()

		// Add tokens to all buckets according to the rate limit
		// and flag full buckets for deletion.
		toDelete := make([]uint64, 0)
		numBucketsBefore := 0
		t.buckets.Range(func(k, v interface{}) bool {
			key := k.(uint64)
			b := v.(*bucket)

			b.replenish(t.limit)

			if b.tokens >= t.limit.value {
				toDelete = append(toDelete, key)
			}

			numBucketsBefore++
			return true
		})

		// Cleanup full buckets to free up memory
		for _, key := range toDelete {
			t.buckets.Delete(key)
		}

		// Reset GC metrics
		t.gc.metrics.numCalls = atomic.Int32{}

		gcDuration := time.Since(gcStartTime)
		numBucketsDeleted := len(toDelete)
		numBucketsAfter := numBucketsBefore - numBucketsDeleted
		logger.Debugf(context.Background(), "gc duration: %v, buckets: (before: %v, deleted: %v, after: %v)",
			gcDuration, numBucketsBefore, numBucketsDeleted, numBucketsAfter)
	}()
}
