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

//go:build !windows
// +build !windows

package linux_auditd

import (
	"fmt"
	"testing"
	"time"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

type mockLog struct {
	tags   map[string]string
	fields map[string]string
}

type mockCollector struct {
	logs []*mockLog
}

func (c *mockCollector) AddData(
	tags map[string]string, fields map[string]string, t ...time.Time) {
	c.AddDataWithContext(tags, fields, nil, t...)
}

func (c *mockCollector) AddDataArray(
	tags map[string]string, columns []string, values []string, t ...time.Time) {
	c.AddDataArrayWithContext(tags, columns, values, nil, t...)
}

func (c *mockCollector) AddRawLog(log *protocol.Log) {
	c.AddRawLogWithContext(log, nil)
}

func (c *mockCollector) AddDataWithContext(
	tags map[string]string, fields map[string]string, ctx map[string]interface{}, t ...time.Time) {
	c.logs = append(c.logs, &mockLog{tags, fields})
}

func (c *mockCollector) AddDataArrayWithContext(
	tags map[string]string, columns []string, values []string, ctx map[string]interface{}, t ...time.Time) {
}

func (c *mockCollector) AddRawLogWithContext(log *protocol.Log, ctx map[string]interface{}) {
	out := make(map[string]string)
	for i := 0; i < len(log.Contents); i++ {
		content := log.Contents[i]
		out[content.Key] = content.Value
	}
	fmt.Print(util.InterfaceToJSONStringIgnoreErr(out) + "\n")
}
func TestLinuxAuditInit(t *testing.T) {
	service_auditd := &ServiceLinuxAuditd{}
	ctx := mock.NewEmptyContext("p", "l", "c")
	collector := &mockCollector{}
	service_auditd.Init(ctx)
	err := service_auditd.Start(collector)
	if err == nil {
		time.Sleep(3 * time.Second)
		service_auditd.Stop()
	}
}

// func TestLinuxAuditRules(t *testing.T) {
// 	rulesBlob := `
// 	# Comments and empty lines are ignored.
// 	-w /etc/passwd -p wa -k auth
// 	-a always,exit -S execve -k exec`

// 	rules, err := loadRules(rulesBlob)
// 	if err == nil {
// 		assert.Equal(t, 2, len(rules))
// 	}
// }
