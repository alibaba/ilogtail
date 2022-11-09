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

package protocol

import (
	"fmt"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

const (
	metricNameKey      = "__name__"
	metricLabelsKey    = "__labels__"
	metricTimeNanoKey  = "__time_nano__"
	metricValueKey     = "__value__"
	metricValueTypeKey = "__type__"
)

const (
	valueTypeFloat  = "float"
	valueTypeInt    = "int"
	valueTypeBool   = "bool"
	valueTypeString = "string"
)

type metricReader struct {
	name      string
	labels    string
	value     string
	valueType string
	timestamp string
}

type metricLabel struct {
	key   string
	value string
}

func (r *metricReader) readNames() (metricName, fieldName string) {
	idx := strings.LastIndexByte(r.name, ':')
	if idx <= 0 {
		return r.name, "value"
	}
	return r.name[:idx], r.name[idx+1:]
}

func (r *metricReader) readSortedLabels() ([]metricLabel, error) {
	n := r.countLabels()
	if n == 0 {
		return nil, nil
	}

	segments := strings.SplitN(r.labels, "|", n)
	sort.Strings(segments)

	labels := make([]metricLabel, len(segments))
	for i, v := range segments {
		idx := strings.Index(v, "#$#")
		if idx < 0 {
			return nil, fmt.Errorf("failed to peed label key")
		}
		labels[i] = metricLabel{key: v[:idx], value: v[idx+3:]}
	}

	return labels, nil
}

func (r *metricReader) countLabels() int {
	if len(r.labels) == 0 {
		return 0
	}
	n := strings.Count(r.labels, "|")
	return n + 1
}

func (r *metricReader) readValue() (interface{}, error) {
	switch r.valueType {
	case valueTypeBool:
		return strconv.ParseBool(r.value)
	case valueTypeString:
		return r.value, nil
	case valueTypeInt:
		return strconv.ParseInt(r.value, 10, 64)
	default:
		return strconv.ParseFloat(r.value, 64)
	}
}

func (r *metricReader) readTimestamp() (time.Time, error) {

	if len(r.timestamp) == 0 {
		return time.Time{}, nil
	}
	t, err := strconv.ParseInt(r.timestamp, 10, 64)
	if err != nil {
		return time.Time{}, err
	}
	return time.Unix(t/1e9, t%1e9).UTC(), nil
}

func newMetricReader(log *protocol.Log) (*metricReader, error) {
	reader := metricReader{}
	for _, v := range log.Contents {
		switch v.Key {
		case metricNameKey:
			reader.name = v.Value
		case metricLabelsKey:
			reader.labels = v.Value
		case metricTimeNanoKey:
			reader.timestamp = v.Value
		case metricValueKey:
			reader.value = v.Value
		case metricValueTypeKey:
			reader.valueType = v.Value
		}
	}
	if len(reader.name) == 0 || len(reader.value) == 0 {
		return nil, fmt.Errorf("metrics data must contains keys: %s, %s", metricNameKey, metricValueKey)
	}
	return &reader, nil
}
