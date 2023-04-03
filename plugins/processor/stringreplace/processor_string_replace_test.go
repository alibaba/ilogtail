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
				Method:        MethodRegex,
				Match:         "{\\",
				ReplaceString: "",
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
		})

		Convey("No SourceKey error", func() {
			processor := &ProcessorRegexReplace{
				Method: MethodRegex,
				Match:  "{}",
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
			So(processor.ReplaceString, ShouldEqual, "")
		})

		Convey("No Method error", func() {
			processor := &ProcessorRegexReplace{}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
			So(processor.ReplaceString, ShouldEqual, "")
		})

		Convey("No Match error", func() {
			processor := &ProcessorRegexReplace{
				Method: MethodConst,
			}
			err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
			So(err, ShouldNotBeNil)
			So(processor.ReplaceString, ShouldEqual, "")
		})
	})

}

func TestProcessorRegexReplaceWork(t *testing.T) {
	Convey("Test Match = regex.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey:     "content",
			Method:        MethodRegex,
			Match:         "\\\\u\\w+\\[\\d{1,3};*\\d{1,3}m|N/A",
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
			Method:        MethodRegex,
			Match:         "(\\d.*\\.)\\d+",
			ReplaceString: "$1*/24",
		}
		err = processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex2 Group", func() {
			ipRecord := `10.10.239.16`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "ip", Value: ipRecord})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `ip`)
			So(logs[0].Contents[0].Value, ShouldEqual, `10.10.239.*/24`)
		})
	})
}

func TestProcessorConstReplaceWork(t *testing.T) {
	Convey("Test Match = const.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey:     "content",
			Method:        MethodConst,
			Match:         "how old are you?",
			ReplaceString: "",
		}
		err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex1", func() {
			record := `hello,how old are you? nice to meet you`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `hello, nice to meet you`)
		})
	})
}

func TestProcessorUnquoteReplaceWork(t *testing.T) {
	Convey("Test Match = unquote.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey: "content",
			Method:    MethodUnquote,
		}
		err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test unquote1", func() {
			record := "{\\x22UNAME\\x22:\\x22\\x22,\\x22GID\\x22:\\x22\\x22,\\x22PAID\\x22:\\x22\\x22,\\x22UUID\\x22:\\x22\\x22,\\x22STARTTIME\\x22:\\x22\\x22,\\x22ENDTIME\\x22:\\x22\\x22,\\x22UID\\x22:\\x222154212790\\x22,\\x22page_num\\x22:1,\\x22page_size\\x22:10}"
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `{"UNAME":"","GID":"","PAID":"","UUID":"","STARTTIME":"","ENDTIME":"","UID":"2154212790","page_num":1,"page_size":10}`)
		})

		Convey("Test unquote2", func() {
			record := "\\u554a"
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `啊`)
		})
	})
}

func TestProcessorRegexChinessCharacters(t *testing.T) {
	Convey("Test Chinese Match.", t, func() {
		processor := &ProcessorRegexReplace{
			SourceKey:     "content",
			Method:        MethodRegex,
			Match:         "\uff08[^）]*\uff09",
			ReplaceString: "",
		}
		err := processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)

		Convey("Test regex", func() {
			record := `替换前（需要被替换的字符）12345`
			log := &protocol.Log{Time: 0}
			log.Contents = append(log.Contents, &protocol.Log_Content{Key: "content", Value: record})
			logs := []*protocol.Log{}
			logs = append(logs, log)
			logs = processor.ProcessLogs(logs)
			So(logs[0].Contents[0].Key, ShouldEqual, `content`)
			So(logs[0].Contents[0].Value, ShouldEqual, `替换前12345`)
		})

		processor = &ProcessorRegexReplace{
			SourceKey:     "content",
			Method:        MethodConst,
			Match:         "（需要被替换的字符）",
			ReplaceString: "",
		}
		err = processor.Init(mock.NewEmptyContext("p", "l", "c"))
		So(err, ShouldBeNil)
		Convey("Test const", func() {
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
