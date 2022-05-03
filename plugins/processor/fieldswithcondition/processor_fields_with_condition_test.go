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
package fieldswithcondition

import (
	"strings"
	"testing"

	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func init() {
	logger.InitTestLogger(logger.OptionOpenMemoryReceiver)
}

func newProcessor() (*ProcessorFieldsWithCondition, error) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	processor := &ProcessorFieldsWithCondition{
		DropIfNotMatchCondition: true,
		Switch: []Condition{
			{
				Case: ConditionCase{
					LogicalOperator:  "and",
					RelationOperator: "contains",
					FieldConditions: map[string]string{
						"content": "Out of memory",
					},
				},
				Action: ConditionAction{
					IgnoreIfExist: true,
					AddFields: map[string]string{
						"eventcode": "c1",
						"cid":       "c-1",
						"test1.1":   "test1.1",
						"test1.2":   "test1.2",
					},
					DropKeys: []string{
						"test1.1",
						"test1.2",
					},
				},
			},
			{
				Case: ConditionCase{
					LogicalOperator:  "and",
					RelationOperator: "contains",
					FieldConditions: map[string]string{
						"content": "BIOS-provided",
					},
				},
				Action: ConditionAction{
					IgnoreIfExist: true,
					AddFields: map[string]string{
						"eventcode": "c2",
						"cid":       "c-2",
						"test2.1":   "test2.1",
						"test2.2":   "test2.2",
					},
					DropKeys: []string{
						"test2.1",
						"test2.2",
					},
				},
			},
		},
	}
	err := processor.Init(ctx)
	return processor, err
}

func TestSuccessCase(t *testing.T) {
	processor, err := newProcessor()
	require.NoError(t, err)
	log1 := &protocol.Log{Time: 0}
	value1 := "time:2017.09.12 20:55:36" +
		"\\tOut of memory: Kill process "
	log1.Contents = append(log1.Contents, &protocol.Log_Content{Key: "content", Value: value1})

	log2 := &protocol.Log{Time: 0}
	value2 := "time:2017.09.12 20:55:36" +
		"\\tBIOS-provided physical RAM map "
	log2.Contents = append(log2.Contents, &protocol.Log_Content{Key: "content", Value: value2})
	processor.ProcessLogs([]*protocol.Log{log1, log2})

	assert.Equal(t, 3, len(log1.Contents))
	assert.Equal(t, 3, len(log2.Contents))
	assert.Equal(t, "eventcode", log1.Contents[1].Key)
	assert.Equal(t, "c1", log1.Contents[1].Value)
	assert.Equal(t, "cid", log1.Contents[2].Key)
	assert.Equal(t, "c-1", log1.Contents[2].Value)
	assert.Equal(t, "eventcode", log2.Contents[1].Key)
	assert.Equal(t, "c2", log2.Contents[1].Value)
	assert.Equal(t, "cid", log2.Contents[2].Key)
	assert.Equal(t, "c-2", log2.Contents[2].Value)
}

func TestRegexFail(t *testing.T) {
	logger.ClearMemoryLog()
	ctx := mock.NewEmptyContext("p", "l", "c")
	processor := &ProcessorFieldsWithCondition{
		DropIfNotMatchCondition: true,
		Switch: []Condition{
			{
				Case: ConditionCase{
					LogicalOperator:  "and",
					RelationOperator: "test123",
					FieldConditions: map[string]string{
						"content": "/^%$*",
					},
				},
				Action: ConditionAction{
					IgnoreIfExist: true,
					AddFields: map[string]string{
						"eventcode": "c1",
						"cid":       "c-1",
						"test1.1":   "test1.1",
						"test1.2":   "test1.2",
					},
					DropKeys: []string{
						"test1.1",
						"test1.2",
					},
				},
			},
		},
	}
	err := processor.Init(ctx)
	require.NoError(t, err)
	memoryLog, ok := logger.ReadMemoryLog(1)
	assert.True(t, ok)
	assert.Equal(t, 1, logger.GetMemoryLogCount())
	assert.True(t, strings.Contains(memoryLog, "AlarmType:CONDITION_INIT_ALARM\tinit relationOpertor error"))
}
