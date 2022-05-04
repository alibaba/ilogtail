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
	DropIfNotMatchCondition bool        `comment:"Optional. When the case condition is not met, whether the log is discarded (true) or retained (false)"`
	Switch                  []Condition `comment:"The switch-case conditions"`

	filterMetric    ilogtail.CounterMetric
	processedMetric ilogtail.CounterMetric
	context         ilogtail.Context
}

type Condition struct {
	Case   ConditionCase   `comment:"The condition that log data satisfies"`
	Action ConditionAction `comment:"The action that executes when the case condition is met"`
}

type FieldApply func(logContent string) bool

type ConditionCase struct {
	LogicalOperator  string            `comment:"Optional. The Logical operators between multiple conditional fields, alternate values are and/or"`
	RelationOperator string            `comment:"Optional. The Relational operators for conditional fields, alternate values are equals/regexp/contains/startwith"`
	FieldConditions  map[string]string `comment:"The key-value pair of field names and expressions"`

	fieldConditionFields map[string]FieldApply
}

type ConditionAction struct {
	IgnoreIfExist bool              `comment:"Optional. Whether to ignore when the same key exists"`
	AddFields     map[string]string `comment:"The appending fields"`
	DropKeys      []string          `comment:"The dropping fields"`

	dropkeyDictionary map[string]bool
}

// Init called for init some system resources, like socket, mutex...
func (p *ProcessorFieldsWithCondition) Init(context ilogtail.Context) error {
	p.context = context
	for i := range p.Switch {
		// set default values
		relationOpertor := p.Switch[i].Case.RelationOperator
		switch relationOpertor {
		case RelationOperatorEquals:
		case RelationOperatorRegexp:
		case RelationOperatorContains:
		case RelationOperatorStartwith:
		default:
			if relationOpertor != RelationOperatorEquals {
				logger.Warning(p.context.GetRuntimeContext(), "CONDITION_INIT_ALARM", "init relationOpertor error, relationOpertor", relationOpertor)
			}
			relationOpertor = RelationOperatorEquals
			p.Switch[i].Case.RelationOperator = relationOpertor
		}
		logicalOperator := p.Switch[i].Case.LogicalOperator
		switch logicalOperator {
		case LogicalOperatorAnd:
		case LogicalOperatorOr:
		default:
			p.Switch[i].Case.LogicalOperator = LogicalOperatorAnd
		}

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
	return "Processor to match multiple conditions, and if one of the conditions is met, the corresponding action is performed."
}

//match single case
func (p *ProcessorFieldsWithCondition) isCaseMatch(log *protocol.Log, conditionCase ConditionCase) bool {
	if conditionCase.fieldConditionFields != nil {
		matchedCount := 0
		for _, cont := range log.Contents {
			if fieldApply, ok := conditionCase.fieldConditionFields[cont.Key]; ok {
				if fieldApply(cont.Value) {
					matchedCount++
				}
			}
		}
		if conditionCase.LogicalOperator == LogicalOperatorAnd && matchedCount == len(conditionCase.fieldConditionFields) {
			return true
		}
		if conditionCase.LogicalOperator == LogicalOperatorOr && matchedCount > 0 {
			return true
		}
	}
	return false
}

//prcess action
func (p *ProcessorFieldsWithCondition) processAction(log *protocol.Log, conditionAction ConditionAction) {
	//add fields
	if conditionAction.AddFields != nil {
		dict := make(map[string]bool)
		for idx := range log.Contents {
			dict[log.Contents[idx].Key] = true
		}
		for k, v := range conditionAction.AddFields {
			if _, exists := dict[k]; conditionAction.IgnoreIfExist && exists {
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
	if conditionAction.dropkeyDictionary != nil {
		for idx := len(log.Contents) - 1; idx >= 0; idx-- {
			if _, exists := conditionAction.dropkeyDictionary[log.Contents[idx].Key]; exists {
				log.Contents = append(log.Contents[:idx], log.Contents[idx+1:]...)
			}
		}
	}
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
