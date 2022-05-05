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
	"encoding/json"
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
	jsonStr := `{
		"DropIfNotMatchCondition":true,
		"Switch":[
			{
				"Case":{
					"LogicalOperator":"and",
					"RelationOperator":"contains",
					"FieldConditions":{
						"content":"Out of memory"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"IgnoreIfExist":false,
						"Fields":{
							"eventcode":"c1",
							"cid":"c-1",
							"test1.1":"test1.1",
							"test1.2":"test1.2"
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[
							"test1.1",
							"test1.2"
						]
					}
				]
			},
			{
				"Case":{
					"LogicalOperator":"and",
					"RelationOperator":"contains",
					"FieldConditions":{
						"content":"BIOS-provided"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"IgnoreIfExist":false,
						"Fields":{
							"eventcode":"c2",
							"cid":"c-2",
							"test2.1":"test2.1",
							"test2.2":"test2.2"
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[
							"test2.1",
							"test2.2"
						]
					}
				]
			}
		]
	}
	`
	var processor ProcessorFieldsWithCondition
	json.Unmarshal([]byte(jsonStr), &processor)
	err := processor.Init(ctx)
	return &processor, err
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
	jsonStr := `{
		"DropIfNotMatchCondition":true,
		"Switch":[
			{
				"Case":{
					"LogicalOperator":"and",
					"RelationOperator":"test123",
					"FieldConditions":{
						"content": "/^%$*"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"IgnoreIfExist":false,
						"Fields":{
							"eventcode":"c1",
							"cid":"c-1",
							"test1.1":"test1.1",
							"test1.2":"test1.2"
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[
							"test1.1",
							"test1.2"
						]
					}
				]
			}
		]
	}
	`
	var processor ProcessorFieldsWithCondition
	json.Unmarshal([]byte(jsonStr), &processor)
	err := processor.Init(ctx)
	require.NoError(t, err)
	memoryLog, ok := logger.ReadMemoryLog(1)
	assert.True(t, ok)
	assert.Equal(t, 1, logger.GetMemoryLogCount())
	assert.True(t, strings.Contains(memoryLog, "AlarmType:CONDITION_INIT_ALARM\tinit relationOpertor error"))
}

func TestAllRelationsCase(t *testing.T) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	jsonStr := `{
		"DropIfNotMatchCondition":true,
		"Switch":[
			{
				"Case":{
					"RelationOperator":"regexp",
					"FieldConditions":{
						"content": "^dummy test1"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"Fields":{
							"eventcode":"c1"
						}
					}
				]
			},
			{
				"Case":{
					"RelationOperator":"contains",
					"FieldConditions":{
						"content": "dummy test2"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"Fields":{
							"eventcode":"c2"
						}
					}
				]
			},
			{
				"Case":{
					"RelationOperator":"startwith",
					"FieldConditions":{
						"content": "dummy test3"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"Fields":{
							"eventcode":"c3"
						}
					}
				]
			},
			{
				"Case":{
					"RelationOperator":"equals",
					"FieldConditions":{
						"content": "dummy test4"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"Fields":{
							"eventcode":"c4"
						}
					}
				]
			}
		]
	}
	`
	var processor ProcessorFieldsWithCondition
	json.Unmarshal([]byte(jsonStr), &processor)
	err := processor.Init(ctx)
	require.NoError(t, err)
	log1 := &protocol.Log{Time: 0}
	value1 := "dummy test1 regexp end"
	log1.Contents = append(log1.Contents, &protocol.Log_Content{Key: "content", Value: value1})

	log2 := &protocol.Log{Time: 0}
	value2 := "contains start dummy test2 contains end"
	log2.Contents = append(log2.Contents, &protocol.Log_Content{Key: "content", Value: value2})

	log3 := &protocol.Log{Time: 0}
	value3 := "dummy test3 startwith end"
	log3.Contents = append(log3.Contents, &protocol.Log_Content{Key: "content", Value: value3})

	log4 := &protocol.Log{Time: 0}
	value4 := "dummy test4"
	log4.Contents = append(log4.Contents, &protocol.Log_Content{Key: "content", Value: value4})
	processor.ProcessLogs([]*protocol.Log{log1, log2, log3, log4})

	assert.Equal(t, 2, len(log1.Contents))
	assert.Equal(t, "eventcode", log1.Contents[1].Key)
	assert.Equal(t, "c1", log1.Contents[1].Value)
	assert.Equal(t, 2, len(log2.Contents))
	assert.Equal(t, "eventcode", log2.Contents[1].Key)
	assert.Equal(t, "c2", log2.Contents[1].Value)
	assert.Equal(t, 2, len(log3.Contents))
	assert.Equal(t, "eventcode", log3.Contents[1].Key)
	assert.Equal(t, "c3", log3.Contents[1].Value)
	assert.Equal(t, 2, len(log4.Contents))
	assert.Equal(t, "eventcode", log4.Contents[1].Key)
	assert.Equal(t, "c4", log4.Contents[1].Value)
}

func TestNoMatchCase(t *testing.T) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	jsonStr := `{
		"DropIfNotMatchCondition":true,
		"Switch":[
			{
				"Case":{
					"LogicalOperator":"and",
					"RelationOperator":"equals",
					"FieldConditions":{
						"content": "dummy"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"IgnoreIfExist":true,
						"Fields":{
							"eventcode":"c1",
							"cid":"c-1",
							"test1.1":"test1.1",
							"test1.2":"test1.2"
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[
							"test1.1",
							"test1.2"
						]
					}
				]
			}
		]
	}
	`
	var processor ProcessorFieldsWithCondition
	json.Unmarshal([]byte(jsonStr), &processor)
	err := processor.Init(ctx)
	require.NoError(t, err)
	log1 := &protocol.Log{Time: 0}
	value1 := "time:2017.09.12 20:55:36" +
		"\\tOut of memory: Kill process "
	log1.Contents = append(log1.Contents, &protocol.Log_Content{Key: "content", Value: value1})
	logArray := processor.ProcessLogs([]*protocol.Log{log1})
	//丢弃测试
	assert.Equal(t, 0, len(logArray))

	jsonStr = `{
		"DropIfNotMatchCondition":false,
		"Switch":[
			{
				"Case":{
					"LogicalOperator":"and",
					"RelationOperator":"equals",
					"FieldConditions":{
						"content": "dummy"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"IgnoreIfExist":true,
						"Fields":{
							"eventcode":"c1",
							"cid":"c-1",
							"test1.1":"test1.1",
							"test1.2":"test1.2"
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[
							"test1.1",
							"test1.2"
						]
					}
				]
			}
		]
	}
	`
	json.Unmarshal([]byte(jsonStr), &processor)
	err = processor.Init(ctx)
	require.NoError(t, err)
	logArray = processor.ProcessLogs([]*protocol.Log{log1})
	//保留测试
	assert.Equal(t, 1, len(logArray))
}

func TestOptinalDefaultCase(t *testing.T) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	jsonStr := `{
		"Switch":[
			{
				"Case":{
					"FieldConditions":{
						"content": "dummy"
					}
				},
				"Actions":[
					{
						"type":"processor_add_fields",
						"Fields":{
							"eventcode":"c1",
						}
					},
					{
						"type":"processor_drop",
						"DropKeys":[]
					}
				]
			}
		]
	}
	`
	var processor ProcessorFieldsWithCondition
	json.Unmarshal([]byte(jsonStr), &processor)
	err := processor.Init(ctx)
	require.NoError(t, err)
	assert.False(t, processor.DropIfNotMatchCondition)
	for i := range processor.Switch {
		// set default values
		relationOpertor := processor.Switch[i].Case.RelationOperator
		logicalOpertor := processor.Switch[i].Case.LogicalOperator
		fieldConditions := processor.Switch[i].Case.FieldConditions
		assert.Equal(t, RelationOperatorEquals, relationOpertor)
		assert.Equal(t, LogicalOperatorAnd, logicalOpertor)
		assert.Equal(t, 1, len(fieldConditions))

		actionAdd := processor.Switch[i].actions[0]
		actionDrop := processor.Switch[i].actions[1]
		assert.False(t, actionAdd.IgnoreIfExist)
		assert.Equal(t, 1, len(actionAdd.Fields))
		assert.Equal(t, 0, len(actionDrop.dropkeyDictionary))
	}
}
