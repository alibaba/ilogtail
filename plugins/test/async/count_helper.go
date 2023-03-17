// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package async

import (
	"errors"
	"sync/atomic"
	"time"
)

type UnittestCounter interface {
	BeginUnittest() bool
	StopUnittest(timeout time.Duration, expectNum int64) error
	AddDelta(num int64)
}

type UnitTestCounterHelper struct {
	signal chan struct{}
	num    int64
}

func (a *UnitTestCounterHelper) BeginUnittest() bool {
	a.signal = make(chan struct{})
	return true
}

func (a *UnitTestCounterHelper) StopUnittest(timeout time.Duration, expectNum int64) error {
	begin := time.Now()
	for {
		if time.Now().Sub(begin).Nanoseconds() > timeout.Nanoseconds() {
			return errors.New("timeout")
		}
		if atomic.LoadInt64(&a.num) == expectNum {
			close(a.signal)
			break
		}
		time.Sleep(time.Millisecond)
	}
	return nil
}

func (a *UnitTestCounterHelper) AddDelta(num int64) {
	atomic.AddInt64(&a.num, num)
}
