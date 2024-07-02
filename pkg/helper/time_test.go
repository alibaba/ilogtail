// Copyright 2024 iLogtail Authors
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

package helper

import (
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestJitter(t *testing.T) {
	jitter := GetJitter(100)
	assert.Greater(t, jitter, time.Duration(0))
	assert.LessOrEqual(t, jitter, time.Duration(100))

	start := time.Now()
	RandomSleep(5*time.Second, nil)
	assert.LessOrEqual(t, time.Since(start), time.Second*5)

	stopCh := make(chan any)
	go func() {
		start = time.Now()
		time.Sleep(time.Second)
		close(stopCh)
	}()
	RandomSleep(time.Second*2, stopCh)
	assert.LessOrEqual(t, time.Since(start), time.Second*2)
}
