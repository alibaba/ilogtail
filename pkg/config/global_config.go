// Copyright 2021 iLogtail Authors
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

package config

import (
	"fmt"
	"runtime"
)

// GlobalConfig represents global configurations of plugin system.
type GlobalConfig struct {
	InputIntervalMs          int
	AggregatIntervalMs       int
	FlushIntervalMs          int
	DefaultLogQueueSize      int
	DefaultLogGroupQueueSize int
	Tags                     map[string]string
	// Directory to store logtail data, such as checkpoint, etc.
	LogtailSysConfDir string
	// Network identification from logtail.
	HostIP       string
	Hostname     string
	AlwaysOnline bool
	DelayStopSec int

	EnableTimestampNanosecond      bool
	UsingOldContentTag             bool
	EnableContainerdUpperDirDetect bool
	EnableSlsMetricsFormat         bool
}

// LogtailGlobalConfig is the singleton instance of GlobalConfig.
var LogtailGlobalConfig = newGlobalConfig()

// StatisticsConfigJson, AlarmConfigJson
var BaseVersion = "0.1.0"                                                  // will be overwritten through ldflags at compile time
var UserAgent = fmt.Sprintf("ilogtail/%v (%v)", BaseVersion, runtime.GOOS) // set in global config

func newGlobalConfig() (cfg GlobalConfig) {
	cfg = GlobalConfig{
		InputIntervalMs:          1000,
		AggregatIntervalMs:       3000,
		FlushIntervalMs:          3000,
		DefaultLogQueueSize:      1000,
		DefaultLogGroupQueueSize: 4,
		LogtailSysConfDir:        ".",
		DelayStopSec:             300,
	}
	return
}
