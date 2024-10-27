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
package generator

import (
	"io"
	"math/rand"
	"os"
	"testing"
	"time"
)

// TestGenerateStdout will be executed in the environment being collected.
func TestGenerateStdout(t *testing.T) {
	config, err := getGenerateFileLogConfigFromEnv("TestMode")
	if err != nil {
		t.Fatalf("get config failed: %v", err)
		return
	}
	testLogContentTmpl := string2Template([]string{
		`{{.LogNo}} [{{.Time}}] [INFO] "这是一条消息，password:123456"`,
		`{{.LogNo}} [{{.Time}}] [WARN] "这是一条消息，password:123456，这是第二条消息，password:00000"`,
		`{{.LogNo}} [{{.Time}}] [ERROR] "这是一条消息"`,
		`{{.LogNo}} [{{.Time}}] [DEBUG] "这是一条消息，password:123456"`,
	})

	logIndex := 0
	logNo := rand.Intn(10000)
	var wr io.Writer
	if config.Custom["TestMode"] == "stdout" {
		wr = os.Stdout
	} else {
		wr = os.Stderr
	}
	for i := 0; i < config.TotalLog; i++ {
		err = testLogContentTmpl[logIndex].Execute(wr, map[string]interface{}{
			"Time":  time.Now().Format("2006-01-02T15:04:05.000000"),
			"LogNo": logNo + i,
		})
		if err != nil {
			t.Fatalf("write log failed: %v", err)
			return
		}
		time.Sleep(time.Duration(config.Interval * int(time.Millisecond)))
		logIndex++
		if logIndex >= len(testLogContentTmpl) {
			logIndex = 0
		}
	}
}
