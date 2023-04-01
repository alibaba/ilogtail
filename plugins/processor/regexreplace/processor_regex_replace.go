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
	"regexp"

	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type ProcessorRegexReplace struct {
	Fields []Field

	fieldMap      map[string]Field
	context       pipeline.Context
	logPairMetric pipeline.CounterMetric
}

type Field struct {
	SourceKey   string
	Regex       string
	Replacement string
	DestKey     string

	re *regexp.Regexp
}

var errNoRegexReplaceFields = errors.New("no regex replace fields error")
var errNoSourceKey = errors.New("no source key error")

// Init called for init some system resources, like socket, mutex...
func (p *ProcessorRegexReplace) Init(context pipeline.Context) error {
	p.context = context
	if len(p.Fields) == 0 {
		return errNoRegexReplaceFields
	}
	p.fieldMap = make(map[string]Field)
	for _, field := range p.Fields {
		var err error
		field.re, err = regexp.Compile(field.Regex)
		if err != nil {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init regex error", err, "regex", field.Regex)
			return err
		}
		if len(field.SourceKey) == 0 {
			return errNoSourceKey
		}
		p.fieldMap[field.SourceKey] = field
	}

	p.logPairMetric = helper.NewAverageMetric("regex_replace_pairs_per_log")
	p.context.RegisterCounterMetric(p.logPairMetric)
	return nil
}

func (*ProcessorRegexReplace) Description() string {
	return "regex replace processor for logtail"
}

func (p *ProcessorRegexReplace) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	for _, log := range logArray {
		beginLen := len(log.Contents)
		for _, cont := range log.Contents {
			if regexField, ok := p.fieldMap[cont.Key]; ok && regexField.re.MatchString(cont.Value) {
				newContVal := regexField.re.ReplaceAllString(cont.Value, regexField.Replacement)
				if len(regexField.DestKey) > 0 {
					newContent := &protocol.Log_Content{
						Key:   regexField.DestKey,
						Value: newContVal,
					}
					log.Contents = append(log.Contents, newContent)
				} else {
					cont.Value = newContVal
				}
			}
		}
		p.logPairMetric.Add(int64(len(log.Contents) - beginLen + 1))
	}
	return logArray
}

func init() {
	pipeline.Processors["processor_regex_replace"] = func() pipeline.Processor {
		return &ProcessorRegexReplace{}
	}
}
