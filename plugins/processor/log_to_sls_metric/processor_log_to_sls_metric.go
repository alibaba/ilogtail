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

package log_to_sls_metric

import (
	"encoding/json"
	"errors"
	"github.com/alibaba/ilogtail/pkg/logger"
	"github.com/alibaba/ilogtail/pkg/pipeline"
	"github.com/alibaba/ilogtail/pkg/protocol"
	converter "github.com/alibaba/ilogtail/pkg/protocol/converter"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"unicode"
)

type ProcessorLogToSlsMetric struct {
	MetricTimeKey      string
	MetricLabelKeys    []string
	MetricValues       map[string]string
	CustomMetricLabels map[string]string

	names              []string
	values             []string
	metricLabelKeysMap map[string]bool
	metricNamesMap     map[string]bool
	metricValuesMap    map[string]bool
	context            pipeline.Context
}

const (
	PluginName        = "processor_log_to_sls_metric"
	metricNameKey     = "__name__"
	metricLabelsKey   = "__labels__"
	metricTimeNanoKey = "__time_nano__"
	metricValueKey    = "__value__"
)

// Regex for labels and names
var (
	metricLabelKeyRegex = regexp.MustCompile(`^[a-zA-Z_][a-zA-Z0-9_]*$`)
	metricNameRegex     = regexp.MustCompile(`^[a-zA-Z_:][a-zA-Z0-9_:]*$`)
)

var (
	errInvalidMetricLabelKey      = errors.New("the Key of Label must follow the regular expression: ^[a-zA-Z_][a-zA-Z0-9_]*$")
	errInvalidMetricLabelValue    = errors.New("label value can not contain '|' or '#$#'")
	errInvalidMetricLabelKeyCount = errors.New("the number of labels must be equal to the number of MetricLabelKeys")

	errEmptyMetricLabel = errors.New("metricLabel is be empty")

	errEmptyMetricValues = errors.New("metricValues is be empty")

	errInvalidMetricName       = errors.New("the name of metric must follow the regular expression: ^[a-zA-Z_:][a-zA-Z0-9_:]*$")
	errInvalidMetricValue      = errors.New("the value of metric must be a number")
	errInvalidMetricNameCount  = errors.New("the number of metric names must be equal to the number of MetricValues")
	errInvalidMetricValueCount = errors.New("the number of metric values must be equal to the number of MetricValues")

	errInvalidMetricTime = errors.New("the value of __time_nano__ field must be a valid Unix timestamp in nanoseconds")

	errFieldRepeated = errors.New("the field is repeated")
)

func (p *ProcessorLogToSlsMetric) Init(context pipeline.Context) error {
	p.context = context
	// Check if the label parameter exists
	if len(p.MetricLabelKeys) == 0 && len(p.CustomMetricLabels) == 0 {
		logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errEmptyMetricLabel)
		return errEmptyMetricLabel
	}
	// Check if Metric Values are empty
	if len(p.MetricValues) == 0 {
		logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errEmptyMetricValues)
		return errEmptyMetricValues
	}
	// Check field is repeated
	existField := map[string]bool{}
	existField[metricLabelsKey] = true
	// Cache labelKey to map for quick access
	p.metricLabelKeysMap = map[string]bool{}
	for _, labelKey := range p.MetricLabelKeys {
		// The Key of Label must follow the regular expression: ^[a-zA-Z_][a-zA-Z0-9_]*$
		if !metricLabelKeyRegex.MatchString(labelKey) {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errInvalidMetricLabelKey)
			return errInvalidMetricLabelKey
		}
		if ok, _ := existField[labelKey]; ok {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errFieldRepeated)
			return errFieldRepeated
		}
		existField[labelKey] = true
		p.metricLabelKeysMap[labelKey] = true
	}
	// Check keys and values of CustomMetricLabels are valid
	for key, value := range p.CustomMetricLabels {
		if !metricLabelKeyRegex.MatchString(key) {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errInvalidMetricLabelKey)
			return errInvalidMetricLabelKey
		}
		// The value of Label cannot contain "|" or "#$#".
		if strings.Contains(value, "|") || strings.Contains(value, "#$#") {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errInvalidMetricLabelValue)
			return errInvalidMetricLabelValue
		}
		if ok, _ := existField[key]; ok {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errFieldRepeated)
			return errFieldRepeated
		}
		existField[key] = true
	}
	// Cache name and value to map for quick access
	p.metricNamesMap = map[string]bool{}
	p.metricValuesMap = map[string]bool{}
	for name, value := range p.MetricValues {
		if ok, _ := existField[name]; ok {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errFieldRepeated)
			return errInvalidMetricLabelKey
		}
		existField[name] = true
		if ok, _ := existField[value]; ok {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "init processor_log_to_sls_metric error", errFieldRepeated)
			return errInvalidMetricLabelKey
		}
		existField[value] = true
		p.metricNamesMap[name] = true
		p.metricValuesMap[value] = true
		p.names = append(p.names, name)
		p.values = append(p.values, value)
	}

	return nil
}

func (p *ProcessorLogToSlsMetric) Description() string {
	return "Parse fields from logs into metrics."
}

func (p *ProcessorLogToSlsMetric) ProcessLogs(logArray []*protocol.Log) []*protocol.Log {
	var metricLogs []*protocol.Log
TraverseLogArray:
	for _, log := range logArray {
		names := map[string]string{}
		values := map[string]string{}
		//__time_nano__ field
		var timeNano string
		// __labels__ field
		metricLabels := converter.MetricLabels{}
		labelCount := 0
		for i, cont := range log.Contents {
			if log.Contents[i] == nil {
				continue
			}
			// __labels__
			if cont.Key == metricLabelsKey {
				labels := strings.Split(cont.Value, "|")
				for _, label := range labels {
					keyValues := strings.Split(label, "#$#")
					key := keyValues[0]
					if p.metricLabelKeysMap[key] {
						logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errFieldRepeated)
						continue TraverseLogArray
					}
					// The Key of Label must follow the regular expression: ^[a-zA-Z_][a-zA-Z0-9_]*$
					if !metricLabelKeyRegex.MatchString(key) {
						logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricLabelKey)
						continue TraverseLogArray
					}
					value := keyValues[1]
					// The value of Label cannot contain "|" or "#$#".
					if strings.Contains(value, "|") || strings.Contains(value, "#$#") {
						logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricLabelValue)
						continue TraverseLogArray
					}
					metricLabels = append(metricLabels, converter.MetricLabel{Key: key, Value: value})
				}
			}

			// Match to the label field
			if p.metricLabelKeysMap[cont.Key] {
				// The value of Label cannot contain "|" or "#$#".
				if strings.Contains(cont.Value, "|") || strings.Contains(cont.Value, "#$#") {
					logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricLabelValue)
					continue TraverseLogArray
				}
				labelCount++
				metricLabels = append(metricLabels, converter.MetricLabel{Key: cont.Key, Value: cont.Value})
			}

			// Match to the name field
			if p.metricNamesMap[cont.Key] {
				// Metric name needs to follow the regular expression: ^[a-zA-Z_:][a-zA-Z0-9_:]*$
				if !metricNameRegex.MatchString(cont.Value) {
					logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricName)
					continue TraverseLogArray
				}
				names[cont.Key] = cont.Value
			}

			// Match to the value field
			if p.metricValuesMap[cont.Key] {
				// Metric value needs to be a float type string.
				if !canParseToFloat64(cont.Value) {
					logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricValue)
					continue TraverseLogArray
				}
				values[cont.Key] = cont.Value
			}

			if len(p.MetricTimeKey) > 0 && cont.Key == p.MetricTimeKey {
				if !isTimeNano(cont.Value) {
					logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricTime)
					continue TraverseLogArray
				}
				timeNano = convertToTimestamp(cont.Value)
			}
		}
		if len(timeNano) == 0 {
			timeNano = convertToTimestamp(log.Time)
		}

		// The number of labels must be equal to the number of label fields.
		if labelCount < len(p.MetricLabelKeys) {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricLabelKeyCount)
			continue TraverseLogArray
		}

		// The number of names must be equal to the number of name fields.
		if len(names) < len(p.metricNamesMap) {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricNameCount)
			continue TraverseLogArray
		}

		// The number of values must be equal to the number of value fields.
		if len(values) < len(p.metricValuesMap) {
			logger.Error(p.context.GetRuntimeContext(), "PROCESSOR_INIT_ALARM", "process log error", errInvalidMetricValueCount)
			continue TraverseLogArray
		}

		for key, value := range p.CustomMetricLabels {
			metricLabels = append(metricLabels, converter.MetricLabel{Key: key, Value: value})
		}

		// sort label
		sort.Sort(metricLabels)
		metricLabel := metricLabels.GetLabel()
		for i, _ := range p.names {
			metricLog := &protocol.Log{
				Time:     log.Time,
				Contents: nil,
			}
			metricLog.Contents = append(metricLog.Contents, &protocol.Log_Content{
				Key:   metricLabelsKey,
				Value: metricLabel,
			})
			metricLog.Contents = append(metricLog.Contents, &protocol.Log_Content{
				Key:   metricNameKey,
				Value: names[p.names[i]],
			})
			metricLog.Contents = append(metricLog.Contents, &protocol.Log_Content{
				Key:   metricValueKey,
				Value: values[p.values[i]],
			})
			metricLog.Contents = append(metricLog.Contents, &protocol.Log_Content{
				Key:   metricTimeNanoKey,
				Value: timeNano,
			})
			metricLogs = append(metricLogs, metricLog)
		}
	}

	return metricLogs

}

func convertToTimestamp(values interface{}) string {
	var timestamp string
	switch t := values.(type) {
	case json.Number:
		timestamp = t.String()
	case float64:
		timestamp = strconv.FormatFloat(t, 'f', -1, 64)
	case uint32:
		timestamp = strconv.FormatFloat(float64(t), 'f', -1, 64)
	case string:
		timestamp = values.(string)
	}
	if len(timestamp) < 19 {
		timestamp += strings.Repeat("0", 19-len(timestamp))
	}
	return timestamp
}

func canParseToFloat64(s string) bool {
	_, err := strconv.ParseFloat(s, 64)
	return err == nil
}

func isTimeNano(t string) bool {
	if len(t) != 19 {
		return false
	}
	for _, c := range t {
		if !unicode.IsDigit(c) {
			return false
		}
	}
	return true
}

func init() {
	pipeline.Processors[PluginName] = func() pipeline.Processor {
		return &ProcessorLogToSlsMetric{}
	}
}
