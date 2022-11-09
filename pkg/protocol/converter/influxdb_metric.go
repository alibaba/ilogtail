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
	"strings"

	"github.com/influxdata/line-protocol/v2/lineprotocol"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

// ConvertToInfluxdbProtocolStream converts @logGroup to []byte in the influxdb line protocol,
// @c.TagKeyRenameMap, @c.ProtocolKeyRenameMap param will be ignored, as they are not very suitable for metrics.
func (c *Converter) ConvertToInfluxdbProtocolStream(logGroup *protocol.LogGroup, targetFields []string) (stream [][]byte, values []map[string]string, err error) {
	if len(c.streamBuffer) == 0 {
		c.streamBuffer = make([]byte, 0, 1024)
	}
	var encoder lineprotocol.Encoder
	encoder.SetBuffer(c.streamBuffer)
	encoder.Reset()
	encoder.SetLax(false)

	for _, log := range logGroup.Logs {
		reader, err := newMetricReader(log)
		if err != nil {
			return nil, nil, err
		}

		metricName, fieldName := reader.readNames()
		labels, err := reader.readSortedLabels()
		if err != nil {
			return nil, nil, err
		}

		v, err := reader.readValue()
		if err != nil {
			return nil, nil, err
		}
		value, ok := lineprotocol.NewValue(v)
		if !ok {
			return nil, nil, fmt.Errorf("unknown field value")
		}
		timestamp, err := reader.readTimestamp()
		if err != nil {
			return nil, nil, err
		}

		encoder.StartLine(metricName)
		for _, v := range labels {
			encoder.AddTag(v.key, v.value)
		}
		encoder.AddField(fieldName, value)
		encoder.EndLine(timestamp)
		if encoder.Err() != nil {
			return nil, nil, encoder.Err()
		}
	}

	c.streamBuffer = encoder.Bytes()

	// we are batching logs in LogGroup, so only support find tags in the logGroup.LogTags
	var desiredValues map[string]string
	if len(targetFields) > 0 {
		desiredValues = findTargetValuesInLogTags(targetFields, logGroup.LogTags)
	}

	return [][]byte{c.streamBuffer}, []map[string]string{desiredValues}, nil
}

func findTargetValuesInLogTags(targetFields []string, logTags []*protocol.LogTag) map[string]string {
	if len(targetFields) == 0 {
		return nil
	}

	tagMap := make(map[string]string, len(logTags))
	for _, logTag := range logTags {
		var tagName string
		if strings.HasPrefix(logTag.Key, tagPrefix) {
			tagName = targetTagPrefix + logTag.Key[len(tagPrefix):]
		} else {
			tagName = logTag.Key
		}
		tagMap[tagName] = logTag.Value
	}

	desiredValue := make(map[string]string, len(targetFields))
	for _, field := range targetFields {
		v, ok := tagMap[field]
		if !ok {
			continue
		}
		desiredValue[field] = v
	}
	return desiredValue
}
