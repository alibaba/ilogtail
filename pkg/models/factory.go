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

package models

func NewTagsWithMap(tags map[string]string) Tags {
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewTagsWithKeyValues(keyValues ...string) Tags {
	if len(keyValues)%2 != 0 {
		keyValues = keyValues[:len(keyValues)-1]
	}
	tags := make(map[string]string)
	for i := 0; i < len(keyValues); i += 2 {
		tags[keyValues[i]] = keyValues[i+1]
	}
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewTags() Tags {
	return &keyValuesImpl[string]{
		keyValues: make(map[string]string),
	}
}

func NewMetadataWithMap(tags map[string]string) Metadata {
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewMetadataWithKeyValues(keyValues ...string) Metadata {
	if len(keyValues)%2 != 0 {
		keyValues = keyValues[:len(keyValues)-1]
	}
	tags := make(map[string]string)
	for i := 0; i < len(keyValues); i += 2 {
		tags[keyValues[i]] = keyValues[i+1]
	}
	return &keyValuesImpl[string]{
		keyValues: tags,
	}
}

func NewMetadata() Metadata {
	return &keyValuesImpl[string]{
		keyValues: make(map[string]string),
	}
}

func NewGroup(meta Metadata, tags Tags) *GroupInfo {
	return &GroupInfo{
		Metadata: meta,
		Tags:     tags,
	}
}

func NewMetric(name string, metricType MetricType, tags Tags, timestamp int64, value MetricValue, typedValues MetricTypedValues) *MetricEvent {
	return &MetricEvent{
		Name:       name,
		MetricType: metricType,
		Timestamp:  uint64(timestamp),
		Tags:       tags,
		Value:      value,
		TypedValue: typedValues,
	}
}

func NewSingleValueMetric[T IntUintFloat](name string, metricType MetricType, tags Tags, timestamp int64, value T) *MetricEvent {
	return &MetricEvent{
		Name:       name,
		MetricType: metricType,
		Timestamp:  uint64(timestamp),
		Tags:       tags,
		Value:      &MetricSingleValue{Value: float64(value)},
		TypedValue: nil,
	}
}

func NewMultiValuesMetric(name string, metricType MetricType, tags Tags, timestamp int64, values MetricFloatValues) *MetricEvent {
	return &MetricEvent{
		Name:       name,
		MetricType: metricType,
		Timestamp:  uint64(timestamp),
		Tags:       tags,
		Value:      &MetricMultiValue{Values: values},
		TypedValue: nil,
	}
}

func NewMetricFloatValues() MetricFloatValues {
	return &keyValuesImpl[float64]{
		keyValues: make(map[string]float64),
	}
}

func NewMetricFloatValuesWithMap(keyValues map[string]float64) MetricFloatValues {
	return &keyValuesImpl[float64]{
		keyValues: keyValues,
	}
}
