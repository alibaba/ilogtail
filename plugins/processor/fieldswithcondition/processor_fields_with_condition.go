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

package fieldswithcondition

import (
	"fmt"
	"regexp"
	"strings"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/helper"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const (
	PluginName = "processor_fields_with_condition"

	RelationOperatorEquals    = "equals"
	RelationOperatorRegexp    = "regexp"
	RelationOperatorContains  = "contains"
	RelationOperatorStartwith = "startwith"

	LogicalOperatorAnd = "and"
	LogicalOperatorOr  = "or"
)

type ProcessorFieldsWithCondition struct {
	DropIfNotMatchCondition bool
	Switch                  []Condition

	filterMetric    ilogtail.CounterMetric
	processedMetric ilogtail.CounterMetric
	context         ilogtail.Context
}

type Condition struct {
	Case   ConditionCase
	Action ConditionAction
}

type FieldApply func(logContent string) bool

type ConditionCase struct {
	LogicalOperator  string
	RelationOperator string
	FieldConditions  map[string]string

	fieldConditionFields map[string]FieldApply
}

type ConditionAction struct {
	IgnoreIfExist bool
	AddFields     map[string]string
	DropKeys      []string

	dropkeyDictionary map[string]bool
}

// Init called for init some system resources, like socket, mutex...
func (p *ProcessorFieldsWithCondition) Init(context ilogtail.Context) error {
	p.context = context
	for i := range p.Switch {
		relationOpertor := p.Switch[i].Case.RelationOperator
		if p.Switch[i].Case.FieldConditions != nil {
			p.Switch[i].Case.fieldConditionFields = make(map[string]FieldApply)
			for key, val := range p.Switch[i].Case.FieldConditions {
				switch relationOpertor {
				case RelationOperatorRegexp:
					reg, err := regexp.Compile(val)
					if err != nil {
						logger.Warning(p.context.GetRuntimeContext(), "CONDITION_INIT_ALARM", "init condition regex error, key", key, "regex", val, "error", err)
						return err
					}
					p.Switch[i].Case.fieldConditionFields[key] = func(logContent string) bool {
						return reg.MatchString(logContent)
					}
				case RelationOperatorContains:
					p.Switch[i].Case.fieldConditionFields[key] = func(logContent string) bool {
						return strings.Contains(logContent, val)
					}
				case RelationOperatorStartwith:
					p.Switch[i].Case.fieldConditionFields[key] = func(logContent string) bool {
						return strings.HasPrefix(logContent, val)
					}
				default:
					p.Switch[i].Case.fieldConditionFields[key] = func(logContent string) bool {
						return logContent == val
					}
				}
			}
		}

		if p.Switch[i].Action.DropKeys != nil {
			p.Switch[i].Action.dropkeyDictionary = make(map[string]bool)
			for _, dropKey := range p.Switch[i].Action.DropKeys {
				p.Switch[i].Action.dropkeyDictionary[dropKey] = true
			}
		}
	}
	p.filterMetric = helper.NewCounterMetric(fmt.Sprintf("%v_filtered", PluginName))
	p.context.RegisterCounterMetric(p.filterMetric)
	p.processedMetric = helper.NewCounterMetric(fmt.Sprintf("%v_processed", PluginName))
	p.context.RegisterCounterMetric(p.processedMetric)
	return nil
}

func (*ProcessorFieldsWithCondition) Description() string {
	return "process fields with condition for logtail, if none of the condition is unmatched, drop this log"
}

//match single case
func (p *ProcessorFieldsWithCondition) isCaseMatch(log *protocol.Log, conditionCase ConditionCase) bool {
	if conditionCase.fieldConditionFields != nil {
		includeCount := 0
		for _, cont := range log.Contents {
			if fieldApply, ok := conditionCase.fieldConditionFields[cont.Key]; ok {
				if fieldApply(cont.Value) {
					includeCount++
				}
			}
		}
		if conditionCase.LogicalOperator == LogicalOperatorAnd && includeCount == len(conditionCase.FieldConditions) {
			return true
		}
		if conditionCase.LogicalOperator == LogicalOperatorOr && includeCount > 0 {
			return true
		}
	}
	return false
}

//prcess action
func (p *ProcessorFieldsWithCondition) processAction(log *protocol.Log, conditionAction ConditionAction) {
	//add fields
	if conditionAction.IgnoreIfExist && len(conditionAction.AddFields) > 1 {
		dict := make(map[string]bool)
		for idx := range log.Contents {
			dict[log.Contents[idx].Key] = true
		}
		for k, v := range conditionAction.AddFields {
			if _, exists := dict[k]; exists {
				continue
			}
			newContent := &protocol.Log_Content{
				Key:   k,
				Value: v,
			}
			log.Contents = append(log.Contents, newContent)
		}
	} else {
		for k, v := range conditionAction.AddFields {
			if conditionAction.IgnoreIfExist && p.isExist(log, k) {
				continue
			}
			newContent := &protocol.Log_Content{
				Key:   k,
				Value: v,
			}
			log.Contents = append(log.Contents, newContent)
		}
	}

	// drop fields
	for idx := len(log.Contents) - 1; idx >= 0; idx-- {
		if _, exists := conditionAction.dropkeyDictionary[log.Contents[idx].Key]; exists {
			log.Contents = append(log.Contents[:idx], log.Contents[idx+1:]...)
		}
	}
}

func (p *ProcessorFieldsWithCondition) isExist(log *protocol.Log, key string) bool {
	for idx := range log.Contents {
		if log.Contents[idx].Key == key {
			return true
		}
	}
	return false
}

//match different cases
func (p *ProcessorFieldsWithCondition) MatchAndProcess(log *protocol.Log) bool {
	if p.Switch != nil {
		for _, condition := range p.Switch {
			if p.isCaseMatch(log, condition.Case) {
				p.processAction(log, condition.Action)
				return true
			}
		}
	}
	return false
}

func (p *ProcessorFieldsWithCondition) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	totalLen := len(logArray)
	nextIdx := 0
	for idx := 0; idx < totalLen; idx++ {
		if p.MatchAndProcess(logArray[idx]) || !p.DropIfNotMatchCondition {
			if idx != nextIdx {
				logArray[nextIdx] = logArray[idx]
			}
			nextIdx++
		} else {
			p.filterMetric.Add(1)
		}
		p.processedMetric.Add(1)
	}
	logArray = logArray[:nextIdx]
	return logArray
}

func init() {
	ilogtail.Processors[PluginName] = func() ilogtail.Processor {
		return &ProcessorFieldsWithCondition{}
	}
}
