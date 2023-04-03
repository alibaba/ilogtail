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

package regexreplace

import (
	"testing"

	. "github.com/smartystreets/goconvey/convey"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

func TestInitError(t *testing.T) {
	Convey("Init error.", t, func() {
		Convey("Regex error", func() {
			processor := &ProcessorRegexReplace{
				SourceKey:     "content",
				Regex:         "{\\",
				ReplaceString: "",
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
		})

		Convey("No SourceKey error", func() {
			processor := &ProcessorRegexReplace{
				Regex: "{}",
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
			So(processor.ReplaceString, ShouldEqual, "")
		})
	})

}

func TestProcessorWork(t *testing.T) {
	Convey("Test Match = regex.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey:     "content",
			Regex:         "\\\\u\\w+\\[\\d{1,3};*\\d{1,3}m|N/A",
			ReplaceString: "",
		}
		err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex1", func() {
			record := `2022-09-16 09:03:31.013 \u001b[32mINFO \u001b[0;39m \u001b[34m[TID: N/A]\u001b[0;39m [\u001b[35mThread-30\u001b[0;39m] \u001b[36mc.s.govern.polygonsync.job.BlockTask\u001b[0;39m : 区块采集------结束------\r`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `2022-09-16 09:03:31.013 INFO  [TID: ] [Thread-30] c.s.govern.polygonsync.job.BlockTask : 区块采集------结束------\r`)
		})

		processor = &ProcessorRegexReplace{
			SourceKey:     "ip",
			Regex:         "(\\d.*\\.)\\d+",
			ReplaceString: "${1}0/24",
		}
		err = processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex2", func() {
			ipRecord := `10.10.239.16`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "ip", Value: ipRecord})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `ip`)
			So(logs[0].Contents[0].Value, ShouldEqual, `10.10.239.0/24`)
		})
	})
}

func TestProcessorChinessCharacters(t *testing.T) {
	Convey("Test Chinese Match = regex.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey:     "content",
			Regex:         "\uff08[^）]*\uff09",
			ReplaceString: "",
		}
		err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex1", func() {
			record := `替换前（需要被替换的字符）12345`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `替换前12345`)
		})
	})
}
