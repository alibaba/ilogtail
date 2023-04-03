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
	"errors"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/dlclark/regexp2"
)

type ProcessorRegexReplace struct {
	SourceKey     string
	Regex         string
	ReplaceString string

	re            *regexp2.Regexp
	context       pipeline.Context
	logPairMetric pipeline.CounterMetric
}

var errNoSourceKey = errors.New("no source key error")

// Init called for init some system resources, like socket, mutex...
func (p *ProcessorRegexReplace) Init(context pipeline.Context) error {
	p.context = context
	var err error
	p.re, err = regexp2.Compile(p.Regex, regexp2.RE2)
	if err != nil {
		logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init regex error", err, "regex", p.Regex)
		return err
	}
	if len(p.SourceKey) == 0 {
		return errNoSourceKey
	}

	p.logPairMetric = helper.NewAverageMetric("regex_replace_pairs_per_log")
	p.context.RegisterCounterMetric(p.logPairMetric)
	return nil
}

func (*ProcessorRegexReplace) Description() string {
	return "regex replace processor for logtail"
}

func (p *ProcessorRegexReplace) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	replaceCount := 0
	for _, log := range logArray {
		for _, cont := range log.Contents {
			if p.SourceKey != cont.Key {
				continue
			}
			if ok, _ := p.re.MatchString(cont.Value); ok {
				newContVal, err := p.re.Replace(cont.Value, p.ReplaceString, -1, -1)
				if err == nil {
					cont.Value = newContVal
					replaceCount++
				}
			}
		}
		p.logPairMetric.Add(int64(replaceCount))
	}
	return logArray
}

func init() {
	pipeline.Processors["processor_regex_replace"] = func() pipeline.Processor {
		return &ProcessorRegexReplace{}
	}
}
