// Copyright 2022 iLogtail Authors
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

package grok

import (
	"testing"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/pingcap/check"
	"github.com/stretchr/testify/require"
)

var _ = check.Suite(&processorTestSuite{})

func Test(t *testing.T) {
	logger.InitTestLogger()
	check.TestingT(t)
}

type processorTestSuite struct {
	processor ilogtail.Processor
}

func (s *processorTestSuite) SetUpTest(c *check.C) {
	s.processor = ilogtail.Processors["processor_grok"]()
}

func (s *processorTestSuite) TestCustomPatterns(c *check.C) {
	processor, _ := s.processor.(*ProcessorGrok)
	c.Assert(processor.Init(mock.NewEmptyContext("p", "l", "c")), check.IsNil)
	processor.CustomPatterns = map[string]string{"HTTP": "%{IP:client} %{WORD:method} %{URIPATHPARAM:request} %{NUMBER:bytes} %{NUMBER:duration}"}
	processor.Match = []string{"%{HTTP}"}
	processor.CustomPatternDir = []string{"./patterns"}
	c.Assert(processor.Init(mock.NewEmptyContext("p", "l", "c")), check.IsNil)
	c.Assert(ans["%{HTTP}"], check.Equals, processor.compiledPatterns[0].String())
}

func (s *processorTestSuite) TestCustomPatternDirs(c *check.C) {
	processor, _ := s.processor.(*ProcessorGrok)
	processor.CustomPatternDir = []string{"./patterns"}
	require.NoError(c, s.processor.Init(mock.NewEmptyContext("p", "l", "c")))
	c.Assert(ans["SLB_URI"], check.Equals, processor.processedPatterns["ELB_URI"])
}

func (s *processorTestSuite) TestMatch(c *check.C) {
	processor, _ := s.processor.(*ProcessorGrok)
	processor.Match = []string{"%{WORD:word1} %{NUMBER:request_time} %{WORD:word2}"}
	processor.KeepSource = true
	require.NoError(c, s.processor.Init(mock.NewEmptyContext("p", "l", "c")))

	log := `begin 123.456 end`
	logPb := test.CreateLogs("content", log)
	logArray := make([]*protocol.Log, 1)
	logArray[0] = logPb

	outLogs := s.processor.ProcessLogs(logArray)
	c.Assert(len(outLogs), check.Equals, 1)
	c.Assert(len(outLogs[0].Contents), check.Equals, 4)
	c.Assert(outLogs[0].Contents[0].GetValue(), check.Equals, log)
	c.Assert(outLogs[0].Contents[1].GetValue(), check.Equals, "begin")
	c.Assert(outLogs[0].Contents[2].GetValue(), check.Equals, "123.456")
	c.Assert(outLogs[0].Contents[3].GetValue(), check.Equals, "end")
	c.Assert(outLogs[0].Contents[0].GetKey(), check.Equals, "content")
	c.Assert(outLogs[0].Contents[1].GetKey(), check.Equals, "word1")
	c.Assert(outLogs[0].Contents[2].GetKey(), check.Equals, "request_time")
	c.Assert(outLogs[0].Contents[3].GetKey(), check.Equals, "word2")
}

func (s *processorTestSuite) TestMultMatch(c *check.C) {
	processor, _ := s.processor.(*ProcessorGrok)
	processor.CustomPatterns = map[string]string{
		"HTTP": "%{IP:client} %{WORD:method} %{URIPATHPARAM:request} %{NUMBER:bytes} %{NUMBER:duration}",
	}
	processor.Match = []string{
		"%{HTTP}",
		"%{WORD:word1} %{NUMBER:request_time} %{WORD:word2}",
		"%{YEAR:year} %{MONTH:month} %{MONTHDAY:day} %{QUOTEDSTRING:motto}",
	}
	processor.KeepSource = false
	processor.IgnoreParseFailure = false
	require.NoError(c, s.processor.Init(mock.NewEmptyContext("p", "l", "c")))

	log1 := `begin 123.456 end`
	logPb1 := test.CreateLogs("content", log1)
	log2 := `2019 June 24 "I am iron man"`
	logPb2 := test.CreateLogs("content", log2)
	log3 := `WRONG LOG`
	logPb3 := test.CreateLogs("content", log3)
	log4 := `10.0.0.0 GET /index.html 15824 0.043`
	logPb4 := test.CreateLogs("content", log4)
	logArray := make([]*protocol.Log, 4)
	logArray[0] = logPb1
	logArray[1] = logPb2
	logArray[2] = logPb3
	logArray[3] = logPb4

	outLogs := s.processor.ProcessLogs(logArray)
	c.Assert(len(outLogs), check.Equals, 4)

	c.Assert(len(outLogs[0].Contents), check.Equals, 3)
	c.Assert(outLogs[0].Contents[0].GetKey(), check.Equals, "word1")
	c.Assert(outLogs[0].Contents[0].GetValue(), check.Equals, "begin")
	c.Assert(outLogs[0].Contents[1].GetKey(), check.Equals, "request_time")
	c.Assert(outLogs[0].Contents[1].GetValue(), check.Equals, "123.456")
	c.Assert(outLogs[0].Contents[2].GetKey(), check.Equals, "word2")
	c.Assert(outLogs[0].Contents[2].GetValue(), check.Equals, "end")

	c.Assert(len(outLogs[1].Contents), check.Equals, 4)
	c.Assert(outLogs[1].Contents[0].GetKey(), check.Equals, "year")
	c.Assert(outLogs[1].Contents[0].GetValue(), check.Equals, "2019")
	c.Assert(outLogs[1].Contents[1].GetKey(), check.Equals, "month")
	c.Assert(outLogs[1].Contents[1].GetValue(), check.Equals, "June")
	c.Assert(outLogs[1].Contents[2].GetKey(), check.Equals, "day")
	c.Assert(outLogs[1].Contents[2].GetValue(), check.Equals, "24")
	c.Assert(outLogs[1].Contents[3].GetKey(), check.Equals, "motto")
	c.Assert(outLogs[1].Contents[3].GetValue(), check.Equals, "\"I am iron man\"")

	c.Assert(len(outLogs[2].Contents), check.Equals, 0)

	c.Assert(len(outLogs[3].Contents), check.Equals, 5)
	c.Assert(outLogs[3].Contents[0].GetKey(), check.Equals, "client")
	c.Assert(outLogs[3].Contents[0].GetValue(), check.Equals, "10.0.0.0")
	c.Assert(outLogs[3].Contents[1].GetKey(), check.Equals, "method")
	c.Assert(outLogs[3].Contents[1].GetValue(), check.Equals, "GET")
	c.Assert(outLogs[3].Contents[2].GetKey(), check.Equals, "request")
	c.Assert(outLogs[3].Contents[2].GetValue(), check.Equals, "/index.html")
	c.Assert(outLogs[3].Contents[3].GetKey(), check.Equals, "bytes")
	c.Assert(outLogs[3].Contents[3].GetValue(), check.Equals, "15824")
	c.Assert(outLogs[3].Contents[4].GetKey(), check.Equals, "duration")
	c.Assert(outLogs[3].Contents[4].GetValue(), check.Equals, "0.043")
}

var ans = map[string]string{
	"%{WORD:word1} %{NUMBER:request_time} %{WORD:word2}":                `(?P<word1>\b\w+\b) (?P<request_time>(?:((?<![0-9.+-])(?>[+-]?(?:(?:[0-9]+(?:\.[0-9]+)?)|(?:\.[0-9]+)))))) (?P<word2>\b\w+\b)`,
	"%{YEAR:year} %{MONTH:month} %{MONTHDAY:day} %{QUOTEDSTRING:motto}": "(?P<year>(?>\\d\\d){1,2}) (?P<month>\\b(?:Jan(?:uary|uar)?|Feb(?:ruary|ruar)?|M(?:a|ä)?r(?:ch|z)?|Apr(?:il)?|Ma(?:y|i)?|Jun(?:e|i)?|Jul(?:y)?|Aug(?:ust)?|Sep(?:tember)?|O(?:c|k)?t(?:ober)?|Nov(?:ember)?|De(?:c|z)(?:ember)?)\\b) (?P<day>(?:(?:0[1-9])|(?:[12][0-9])|(?:3[01])|[1-9])) (?P<motto>(?>(?<!\\\\)(?>\"(?>\\\\.|[^\\\\\"]+)+\"|\"\"|(?>'(?>\\\\.|[^\\\\']+)+')|''|(?>`(?>\\\\.|[^\\\\`]+)+`)|``)))",
	"%{HTTP}": `((?P<client>(?:(((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?)|((?<![0-9])(?:(?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5]))(?![0-9])))) (?P<method>\b\w+\b) (?P<request>((?:/[A-Za-z0-9$.+!*'(){},~:;=@#%_\-]*)+)(?:(\?[A-Za-z0-9$.+!*'|(){},~@#%&/=:;_?\-\[\]<>]*))?) (?P<bytes>(?:((?<![0-9.+-])(?>[+-]?(?:(?:[0-9]+(?:\.[0-9]+)?)|(?:\.[0-9]+)))))) (?P<duration>(?:((?<![0-9.+-])(?>[+-]?(?:(?:[0-9]+(?:\.[0-9]+)?)|(?:\.[0-9]+)))))))`,
	"SLB_URI": `(?P<proto>[A-Za-z]+(\+[A-Za-z+]+)?)://(?:(([a-zA-Z0-9._-]+))(?::[^@]*)?@)?(?:(?P<urihost>((?:((?:(((([0-9A-Fa-f]{1,4}:){7}([0-9A-Fa-f]{1,4}|:))|(([0-9A-Fa-f]{1,4}:){6}(:[0-9A-Fa-f]{1,4}|((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){5}(((:[0-9A-Fa-f]{1,4}){1,2})|:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3})|:))|(([0-9A-Fa-f]{1,4}:){4}(((:[0-9A-Fa-f]{1,4}){1,3})|((:[0-9A-Fa-f]{1,4})?:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){3}(((:[0-9A-Fa-f]{1,4}){1,4})|((:[0-9A-Fa-f]{1,4}){0,2}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){2}(((:[0-9A-Fa-f]{1,4}){1,5})|((:[0-9A-Fa-f]{1,4}){0,3}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(([0-9A-Fa-f]{1,4}:){1}(((:[0-9A-Fa-f]{1,4}){1,6})|((:[0-9A-Fa-f]{1,4}){0,4}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:))|(:(((:[0-9A-Fa-f]{1,4}){1,7})|((:[0-9A-Fa-f]{1,4}){0,5}:((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)(\.(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)){3}))|:)))(%.+)?)|((?<![0-9])(?:(?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5])[.](?:[0-1]?[0-9]{1,2}|2[0-4][0-9]|25[0-5]))(?![0-9]))))|(\b(?:[0-9A-Za-z][0-9A-Za-z-]{0,62})(?:\.(?:[0-9A-Za-z][0-9A-Za-z-]{0,62}))*(\.?|\b))))(?::(?P<port>\b(?:[1-9][0-9]*)\b))?))?(?:((?P<path>(?:/[A-Za-z0-9$.+!*'(){},~:;=@#%_\-]*)+)(?:(?P<params>\?[A-Za-z0-9$.+!*'|(){},~@#%&/=:;_?\-\[\]<>]*))?))?`,
}
