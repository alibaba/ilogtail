// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package sql

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

type Args struct {
	in      *models.PipelineGroupEvents
	context pipeline.PipelineContext
}

func newProcessor(script string) (*ProcessorSQL, error) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	processor := &ProcessorSQL{
		Script:     script,
		NoKeyError: true,
	}
	err := processor.Init(ctx)
	return processor, err
}

func TestProcessorSQL_Process(t *testing.T) {
	data := []map[string]interface{}{
		{
			"timestamp":  "1234567890",
			"nanosecond": "123456789",
			"event_type": "js_error",
			"idfa":       "abcdefg",
			"user_agent": "Chrome on iOS. Mozilla/5.0 (iPhone; CPU iPhone OS 16_5_1 like Mac OS X)",
			"action":     "click",
			"element":    "#Button",
		},
		{
			"timestamp":  "1234567890",
			"nanosecond": "123456789",
			"event_type": "perf",
			"idfa":       "abcdefg",
			"user_agent": "Chrome on iOS. Mozilla/5.0 (iPhone; CPU iPhone OS 16_5_1 like Mac OS X)",
			"load":       "3",
			"render":     "2",
		},
	}
	tests := []struct {
		name   string
		script string
		data   []map[string]interface{}
		res    []map[string]interface{}
	}{
		// functions have specific UT in function_test.go 、aes_encrypt_test.go, so there mainly tests framework robustness
		// rename、select、where、regexp、concat_ws、md(5)、case when、lower
		{
			name:   "test example0",
			script: "select concat_ws('.', timestamp, nanosecond) event_time, event_type, md5(idfa) idfa, CASE WHEN user_agent REGEXP '.*iPhone OS.*' THEN 'ios' ELSE 'android' END os, action, lower(element) element from log where event_type='js_error'",
			data:   data,
			res: []map[string]interface{}{
				{
					"event_time": "1234567890.123456789",
					"event_type": "js_error",
					"idfa":       "7ac66c0f148de9519b8bd264312c4d64",
					"os":         "ios",
					"action":     "click",
					"element":    "#button",
				},
				nil,
			},
		},
		// another example in another type of CASE WHEN
		{
			name:   "test example1",
			script: "select concat_ws('.', timestamp, nanosecond) event_time, event_type, md5(idfa) idfa, CASE event_type WHEN 'js_error'  THEN 'js' WHEN 'perf' THEN 'system' ELSE 'java' END file_type, action, lower(element) element from log where event_type='js_error'",
			data:   data,
			res: []map[string]interface{}{
				{
					"event_time": "1234567890.123456789",
					"event_type": "js_error",
					"idfa":       "7ac66c0f148de9519b8bd264312c4d64",
					"file_type":  "js",
					"action":     "click",
					"element":    "#button",
				},
				nil,
			},
		},
		// function as a parameter of CASE WHEN
		{
			name:   "test example2",
			script: "select concat_ws('.', timestamp, nanosecond) event_time, event_type, md5(idfa) idfa, CASE event_type WHEN 'js_error'  THEN upper('js') WHEN 'perf' THEN upper('system') ELSE upper('java') END file_type, action, lower(element) element from log where event_type='js_error'",
			data:   data,
			res: []map[string]interface{}{
				{
					"event_time": "1234567890.123456789",
					"event_type": "js_error",
					"idfa":       "7ac66c0f148de9519b8bd264312c4d64",
					"file_type":  "JS",
					"action":     "click",
					"element":    "#button",
				},
				nil,
			},
		},
		// locate、and
		{
			name:   "test condition0",
			script: "select locate('iOS',user_agent) ua_index from log where action = 'click' and element = '#Button'",
			data:   data,
			res: []map[string]interface{}{
				{
					"ua_index": "11",
				},
				nil,
			},
		},
		// ()、or、!=
		{
			name:   "test condition1",
			script: "select locate('iOS',user_agent) ua_index from log where action != 'click' or (element = '#Button' and timestamp = '1234567890')",
			data:   data,
			res: []map[string]interface{}{
				{
					"ua_index": "11",
				},
				nil,
			},
		},
		// compare
		{
			name:   "test condition2",
			script: "select locate('iOS',user_agent) ua_index from log where (render < 5.7 and timestamp > '234567890')",
			data:   data,
			res: []map[string]interface{}{
				nil,
				{
					"ua_index": "11",
				},
			},
		},
		// regexp、like
		{
			name:   "test regexp",
			script: "select locate('iOS',user_agent) ua_index from log where action NOT REGEXP 'cli.{2}' or (element LIKE '#Bu_to_' and timestamp LIKE '123456%')",
			data:   data,
			res: []map[string]interface{}{
				{
					"ua_index": "11",
				},
				nil,
			},
		},
		// if(a,b,c)、function as a parameter of function
		{
			name:   "test function0",
			script: "select idfa, if(md5(idfa),1,2) if_result from log where action NOT REGEXP 'cli.{2}' or (element LIKE '#Bu_to_' and timestamp LIKE '123456%')",
			data:   data,
			res: []map[string]interface{}{
				{
					"idfa":      "abcdefg",
					"if_result": "1",
				},
				nil,
			},
		},
		// function as a parameter of case、function as an expression of where、regexp_substr
		{
			name:   "test function1",
			script: "select CASE WHEN regexp_substr(user_agent,'.* ',4) REGEXP '.*/5.0.*' THEN '5' ELSE '4' END version from log where length(timestamp) > 5",
			data:   data,
			res: []map[string]interface{}{
				{
					"version": "5",
				},
				{
					"version": "5",
				},
			},
		},
		// condition as a parameter of function
		{
			name:   "test function2",
			script: "select idfa, if(1<2,1,2) if_result from log where action NOT REGEXP 'cli.{2}' or (element LIKE '#Bu_to_' and timestamp LIKE '123456%')",
			data:   data,
			res: []map[string]interface{}{
				{
					"idfa":      "abcdefg",
					"if_result": "1",
				},
				nil,
			},
		},
	}
	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			p, err := newProcessor(tt.script)
			require.NoError(t, err)
			args := Args{
				in: &models.PipelineGroupEvents{},
			}
			args.context = pipeline.NewObservePipelineConext(10)

			for i := 0; i < len(tt.data); i++ {
				log := models.NewLog("", nil, "", "", "", models.NewTags(), 0)
				content := log.GetIndices()
				content.AddAll(tt.data[i])
				args.in.Events = append(args.in.Events, log)
			}

			p.Process(args.in, args.context)
			args.context.Collector().CollectList(args.in)
			for i, event := range args.in.Events {
				logEvent := event.(*models.Log)
				out := logEvent.GetIndices()
				if tt.res[i] == nil {
					assert.Nil(t, out)
					continue
				}
				assert.Equal(t, tt.res[i], out.Iterator())
			}
		})
	}
}
