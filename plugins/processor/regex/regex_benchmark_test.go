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

package regex

import (
	"regexp"
	"strconv"
	"testing"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
)

type MockParam struct {
	name         string
	num          int
	separator    string
	separatorReg string
	mockFunc     func(int, string, string) (string, string)
}

var params = []MockParam{
	{"original", 10, " ", "\\s", mockData},
	{"new", 10, "\n", "\n", mockDataSingleLine},
	{"original", 100, " ", "\\s", mockData},
	{"new", 100, "\n", "\n", mockDataSingleLine},
	{"original", 1000, " ", "\\s", mockData},
	{"new", 1000, "\n", "\n", mockDataSingleLine},
}

// goos: darwin
// goarch: amd64
// pkg: github.com/alibaba/ilogtail/plugins/processor/regex
// cpu: Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz
// BenchmarkTest/original10-12               109512              9603 ns/op             356 B/op          2 allocs/op
// BenchmarkTest/new10-12                    122436              9775 ns/op             355 B/op          2 allocs/op
// BenchmarkTest/original100-12                  44          28163247 ns/op           12556 B/op         11 allocs/op
// BenchmarkTest/new100-12                       43          28902079 ns/op           12764 B/op         11 allocs/op
// BenchmarkTest/original1000-12                  1        58107751748 ns/op       33614872 B/op       4028 allocs/op
// BenchmarkTest/new1000-12                       1        60034394846 ns/op       33614880 B/op       4029 allocs/op
func BenchmarkTest(b *testing.B) {
	for _, param := range params {
		b.Run(param.name+strconv.Itoa(param.num), func(b *testing.B) {
			pattern, log := param.mockFunc(param.num, param.separator, param.separatorReg)
			reg := regexp.MustCompile(pattern)
			b.ReportAllocs()
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				reg.FindStringSubmatch(log)
			}
			b.StopTimer()
		})
	}
}

func mockData(num int, separator, separatorReg string) (string, string) {
	line := "dfasdfjlksdfhjaklsdfjhsdf4861238423904ljfaslkdfasjkdfhlasdjfkl3"
	reg := "(\\w+)"
	var res, pattern string
	for i := 0; i < num; i++ {
		res += line
		pattern += reg
		if i != num-1 {
			res += separator
			pattern += separatorReg
		}
	}
	return pattern, res
}

// goos: linux
// goarch: amd64
// pkg: github.com/alibaba/ilogtail/plugins/processor/regex
// cpu: Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz
// BenchmarkRegexpTest
// BenchmarkRegexpTest/original10
// BenchmarkRegexpTest/original10-6         	  110100	     11092 ns/op	     253 B/op	       2 allocs/op
// BenchmarkRegexpTest/new10-6              	  106592	     10424 ns/op	     253 B/op	       2 allocs/op
// BenchmarkRegexpTest/original100-6        	      37	  28913435 ns/op	   12500 B/op	      13 allocs/op
// BenchmarkRegexpTest/new100-6             	      40	  27906572 ns/op	   11699 B/op	      12 allocs/op
// BenchmarkRegexpTest/original1000-6       	       1	56747915620 ns/op	33597184 B/op	    4031 allocs/op
// BenchmarkRegexpTest/new1000-6					   1	61395055880 ns/op	33597088 B/op	    4030 allocs/op

func BenchmarkRegexpTest(b *testing.B) {
	for _, param := range params {
		b.Run(param.name+strconv.Itoa(param.num), func(b *testing.B) {
			pattern, log := param.mockFunc(param.num, param.separator, param.separatorReg)
			processor := ilogtail.Processors["processor_regex"]().(*ProcessorRegex)
			processor.Keys = []string{"test"}
			processor.Regex = pattern
			processor.Init(mock.NewEmptyContext("p", "l", "c"))

			var Logs protocol.Log
			Logs.Contents = make([]*protocol.Log_Content, 0)

			b.ReportAllocs()
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				processor.processRegex(&Logs, &log)
			}
			b.StopTimer()
		})
	}
}

func mockDataSingleLine(num int, separator, separatorReg string) (string, string) {
	pattern, log := mockData(num, separator, separatorReg)
	return "(?s)" + pattern, log
}
