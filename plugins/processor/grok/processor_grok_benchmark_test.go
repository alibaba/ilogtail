package grok

import (
	"strconv"
	"testing"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	"github.com/dlclark/regexp2"
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

// goos: linux
// goarch: amd64
// pkg: github.com/alibaba/ilogtail/plugins/processor/grok
// cpu: Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz
// BenchmarkTest/original10-6         	  102200	     11067 ns/op	    5792 B/op	      28 allocs/op
// BenchmarkTest/new10-6				  109221	     10764 ns/op	    5792 B/op	      28 allocs/op
// BenchmarkTest/original100-6			    9826	    101980 ns/op	   53928 B/op	     209 allocs/op
// BenchmarkTest/new100-6             	   10000	    103381 ns/op	   53928 B/op	     209 allocs/op
// BenchmarkTest/original1000-6       	    1032	   1050481 ns/op	  526327 B/op	    2909 allocs/op
// BenchmarkTest/new1000-6            	    1184	   1026559 ns/op	  526275 B/op	    2909 allocs/op
func BenchmarkTest(b *testing.B) {
	for _, param := range params {
		b.Run(param.name+strconv.Itoa(param.num), func(b *testing.B) {
			pattern, log := param.mockFunc(param.num, param.separator, param.separatorReg)
			reg := regexp2.MustCompile(pattern, regexp2.RE2)
			b.ReportAllocs()
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				m, _ := reg.FindStringMatch(log)
				for m != nil {
					gps := m.Groups()
					for i := range gps {
						if _, err := strconv.ParseInt(gps[i].Name, 10, 32); err != nil && gps[i].Capture.String() != "" {
						}
					}
					m, _ = reg.FindNextMatch(m)
				}
			}
			b.StopTimer()
		})
	}
}

// goos: linux
// goarch: amd64
// pkg: github.com/alibaba/ilogtail/plugins/processor/grok
// cpu: Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz
// BenchmarkGrokTest/original10-6         	   57016	     19545 ns/op	    7544 B/op	      36 allocs/op
// BenchmarkGrokTest/new10-6              	   53883	     20073 ns/op	    7546 B/op	      36 allocs/op
// BenchmarkGrokTest/original100-6        	    6636	    188088 ns/op	   67190 B/op	     216 allocs/op
// BenchmarkGrokTest/new100-6             	    6690	    175370 ns/op	   67190 B/op	     216 allocs/op
// BenchmarkGrokTest/original1000-6       	     684	   1763691 ns/op	  654911 B/op	    2016 allocs/op
// BenchmarkGrokTest/new1000-6            	     631	   1758877 ns/op	  654963 B/op	    2016 allocs/op
func BenchmarkGrokTest(b *testing.B) {
	for _, param := range params {
		b.Run(param.name+strconv.Itoa(param.num), func(b *testing.B) {
			pattern, log := param.mockFunc(param.num, param.separator, param.separatorReg)
			processor := ilogtail.Processors["processor_grok"]().(*ProcessorGrok)
			processor.CustomPatterns = map[string]string{"MYDATA": pattern}
			processor.Match = []string{"%{MYDATA:data}"}
			processor.Init(mock.NewEmptyContext("p", "l", "c"))

			var Logs protocol.Log
			Logs.Contents = make([]*protocol.Log_Content, 0)

			b.ReportAllocs()
			b.ResetTimer()
			for i := 0; i < b.N; i++ {
				processor.processGrok(&Logs, &log)
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

func mockDataSingleLine(num int, separator, separatorReg string) (string, string) {
	pattern, log := mockData(num, separator, separatorReg)
	return "(?s)" + pattern, log
}
