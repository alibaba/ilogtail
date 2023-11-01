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

package utf16ToUtf8

import (
	"testing"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

func newProcessor(endian string) (*ProcessorUtf16ToUtf8, error) {
	ctx := mock.NewEmptyContext("p", "l", "c")
	processor := &ProcessorUtf16ToUtf8{
		SourceKey: "content",
		Endian:    endian,
	}
	err := processor.Init(ctx)
	return processor, err
}

func TestProcessorUtf16ToUtf8_Init(t *testing.T) {
	_, err := newProcessor("Little")
	if err != nil {
		t.Errorf("Init failed: %s", err)
	}
}

func TestProcessorUtf16ToUtf8_Description(t *testing.T) {
	p, err := newProcessor("Little")
	if err != nil {
		t.Errorf("Init failed: %s", err)
	}
	expectedDescription := "Parse utf16 logs into utf8."
	description := p.Description()

	if description != expectedDescription {
		t.Errorf("Description does not match. Expected: %s, Got: %s", expectedDescription, description)
	}
}

func TestProcessorUtf16ToUtf8_ProcessLogs(t *testing.T) {
	p, err := newProcessor("Little")
	if err != nil {
		t.Errorf("Init failed: %s", err)
	}
	logArray := []*protocol.Log{
		{
			Contents: []*protocol.Log_Content{
				{
					Key:   "content",
					Value: "你好",
				},
			},
		},
		{
			Contents: []*protocol.Log_Content{
				{
					Key:   "content",
					Value: "こんにちは",
				},
			},
		},
	}
	expectedResult := logArray

	result := p.ProcessLogs(logArray)

	if len(result) != len(expectedResult) {
		t.Errorf("ProcessLogs result length does not match. Expected: %d, Got: %d", len(expectedResult), len(result))
	}

	for i := 0; i < len(result); i++ {
		if result[i].Contents[0].Value != expectedResult[i].Contents[0].Value {
			t.Errorf("ProcessLogs result does not match. Expected: %s, Got: %s", expectedResult[i].Contents[0].Value, result[i].Contents[0].Value)
		}
	}
}

func TestProcessorUtf16ToUtf8_Little_processLog(t *testing.T) {
	p, err := newProcessor("Little")
	if err != nil {
		t.Errorf("Init failed: %s", err)
	}
	log := &protocol.Log{
		Contents: []*protocol.Log_Content{
			{
				Key:   "content",
				Value: "\x60\x4F\x7D\x59", // UTF16 encoded string: "你好"
			},
		},
	}
	expectedResult := "你好"

	p.processLog(log)

	if log.Contents[0].Value != expectedResult {
		t.Errorf("processLog result does not match. Expected: %s, Got: %s", expectedResult, log.Contents[0].Value)
	}
}

func TestProcessorUtf16ToUtf8_Big_processLog(t *testing.T) {
	p, err := newProcessor("Big")
	if err != nil {
		t.Errorf("Init failed: %s", err)
	}
	log := &protocol.Log{
		Contents: []*protocol.Log_Content{
			{
				Key:   "content",
				Value: "\x4F\x60\x59\x7D", // UTF16 encoded string: "你好"
			},
		},
	}
	expectedResult := "你好"

	p.processLog(log)

	if log.Contents[0].Value != expectedResult {
		t.Errorf("processLog result does not match. Expected: %s, Got: %s", expectedResult, log.Contents[0].Value)
	}
}
