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
	"crypto/rand"
	"math/big"
	"time"
)

// RandomSleep sleeps for a random duration between 0 and jitterInSec.
// If jitterInSec is 0, it will skip sleep.
// If shutdown is closed, it will stop sleep immediately.
func RandomSleep(maxJitter time.Duration, stopChan chan interface{}) {
	if maxJitter == 0 {
		return
	}

	sleepTime := GetJitter(maxJitter)
	t := time.NewTimer(sleepTime)
	select {
	case <-t.C:
		return
	case <-stopChan:
		t.Stop()
		return
	}
}

func GetJitter(maxJitter time.Duration) time.Duration {
	jitter, err := rand.Int(rand.Reader, big.NewInt(int64(maxJitter)))
	if err != nil {
		return 0
	}
	return time.Duration(jitter.Int64())
}
