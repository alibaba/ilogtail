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

package utf16ToUtf8

import (
	"fmt"

	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"golang.org/x/text/encoding/unicode"
)

type ProcessorUtf16ToUtf8 struct {
	SourceKey string
	Endian    string

	endian  unicode.Endianness
	context pipeline.Context
}

const (
	PluginName = "processor_utf16_to_utf8"
)

func (p *ProcessorUtf16ToUtf8) Init(context pipeline.Context) (err error) {
	p.context = context
	if p.Endian == "Little" {
		p.endian = unicode.LittleEndian
	} else if p.Endian == "Big" {
		p.endian = unicode.BigEndian
	} else {
		return fmt.Errorf("invalid endian: %s", p.Endian)
	}
	return nil
}

func (p *ProcessorUtf16ToUtf8) Description() string {
	return "Parse utf16 logs into utf8."
}

func (p *ProcessorUtf16ToUtf8) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	for _, log := range logArray {
		p.processLog(log)
	}
	return logArray
}

func (p *ProcessorUtf16ToUtf8) processLog(log *protocol.Log) {
	for idx := len(log.Contents) - 1; idx >= 0; idx-- {

		if log.Contents[idx].Key != p.SourceKey {
			continue
		}
		b := []byte(log.Contents[idx].Value)
		utf8String, err := p.DecodeUTF16(b)
		if err != nil {
			fmt.Println(err)
		}
		if len(utf8String) == 0 {
			log.Contents = append(log.Contents[:idx], log.Contents[idx+1:]...)
		} else {
			log.Contents[idx].Value = utf8String
		}
	}
}

func (p *ProcessorUtf16ToUtf8) DecodeUTF16(data []byte) (string, error) {
	if len(data)%2 != 0 && data[0] == 0 {
		data = data[1:]
	}
	decoder := unicode.UTF16(p.endian, unicode.IgnoreBOM).NewDecoder()
	utf8bytes, err := decoder.Bytes(data)

	return string(utf8bytes), err
}

func init() {
	pipeline.Processors[PluginName] = func() pipeline.Processor {
		return &ProcessorUtf16ToUtf8{
			SourceKey: "content",
			Endian:    "Little",
		}
	}
}
