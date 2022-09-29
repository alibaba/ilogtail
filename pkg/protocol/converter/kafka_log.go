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
	"encoding/json"
	"fmt"
	"strings"

	"github.com/alibaba/ilogtail/pkg/fmtstr"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

func (c *Converter) ConvertToKafkaSingleLog(logGroup *protocol.LogGroup, log *protocol.Log, topic string) ([]byte, *string, error) {
	var marshaledLogs []byte
	contents, tags := convertLogToMap(log, logGroup.LogTags, logGroup.Source, logGroup.Topic, c.TagKeyRenameMap)
	actualTopic, err := formatTopic(contents, tags, c.TagKeyRenameMap, topic)
	if err != nil {
		return nil, nil, err
	}
	customSingleLog := make(map[string]interface{}, numProtocolKeys)
	if newKey, ok := c.ProtocolKeyRenameMap[protocolKeyTime]; ok {
		customSingleLog[newKey] = log.Time
	} else {
		customSingleLog[protocolKeyTime] = log.Time
	}
	if newKey, ok := c.ProtocolKeyRenameMap[protocolKeyContent]; ok {
		customSingleLog[newKey] = contents
	} else {
		customSingleLog[protocolKeyContent] = contents
	}
	if newKey, ok := c.ProtocolKeyRenameMap[protocolKeyTag]; ok {
		customSingleLog[newKey] = tags
	} else {
		customSingleLog[protocolKeyTag] = tags
	}
	switch c.Encoding {
	case EncodingJSON:
		b, err := json.Marshal(customSingleLog)
		if err != nil {
			return nil, nil, fmt.Errorf("unable to marshal log: %v", customSingleLog)
		}
		marshaledLogs = b
	default:
		return nil, nil, fmt.Errorf("unsupported encoding format: %s", c.Encoding)
	}

	return marshaledLogs, actualTopic, nil
}

// formatTopic return topic dynamically by using a format string
func formatTopic(contents, tags, tagKeyRenameMap map[string]string, topicPattern string) (*string, error) {
	sf, err := fmtstr.Compile(topicPattern, func(field string, ops []fmtstr.VariableOp) (fmtstr.FormatEvaler, error) {
		switch {
		case strings.HasPrefix(field, targetContentPrefix):
			if value, ok := contents[field[len(targetContentPrefix):]]; ok {
				return fmtstr.StringElement{S: value}, nil
			}
		case strings.HasPrefix(field, targetTagPrefix):
			if value, ok := tags[field[len(targetTagPrefix):]]; ok {
				return fmtstr.StringElement{S: value}, nil
			} else if value, ok := tagKeyRenameMap[field[len(targetTagPrefix):]]; ok {
				return fmtstr.StringElement{S: tags[value]}, nil
			}
		default:
			return fmtstr.StringElement{S: field}, nil
		}
		return fmtstr.StringElement{S: field}, nil
	})
	if err != nil {
		return nil, err
	}
	topic, err := sf.Run(nil)
	if err != nil {
		return nil, err
	}
	return &topic, nil
}
