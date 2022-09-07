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
)

var tagConversionMap = map[string]string{
	"__path__":         "log.file.path",
	"__hostname__":     "host.name",
	"_node_ip_":        "k8s.node.ip",
	"_node_name_":      "k8s.node.name",
	"_namespace_":      "k8s.namespace.name",
	"_pod_name_":       "k8s.pod.name",
	"_pod_ip_":         "k8s.pod.ip",
	"_pod_uid_":        "k8s.pod.uid",
	"_container_name_": "container.name",
	"_container_ip_":   "container.ip",
	"_image_name_":     "container.image.name",
}

var supportedEncodingMap = map[string]map[string]bool{
	"single": {
		"json":     true,
		"protobuf": false,
	},
	"batch": {
		"json":     true,
		"protobuf": false,
	},
	"otlp": {
		"json":     true,
		"protobuf": true,
	},
}

type Converter struct {
	Protocol          string
	Encoding          string
	TagRenameMap      map[string]string
	ProtoKeyRenameMap map[string]string
}

func NewConverter(protocol, encoding string, keyRenameMap, tagRenameMap map[string]string) (*Converter, error) {
	enc, ok := supportedEncodingMap[protocol]
	if !ok {
		return nil, fmt.Errorf("unsupported protocol: %s", protocol)
	}
	if supported, existed := enc[encoding]; !existed || !supported {
		return nil, fmt.Errorf("unsupported encoding: %s for protocol %s", encoding, protocol)
	}
	return &Converter{
		Protocol:          protocol,
		Encoding:          encoding,
		TagRenameMap:      tagRenameMap,
		ProtoKeyRenameMap: keyRenameMap,
	}, nil
}

func (c *Converter) Do(logGroup *LogGroup) ([][]byte, error) {
	b, _, err := c.DoWithSelectedFields(logGroup, nil)
	return b, err
}

func (c *Converter) DoWithSelectedFields(logGroup *LogGroup, targetFields []string) ([][]byte, [][]string, error) {
	switch c.Protocol {
	case "single":
		return c.ConvertToSingleLogs(logGroup, targetFields)
	default:
		return nil, nil, fmt.Errorf("unsupported protocol: %s", c.Protocol)
	}
}

func (c *Converter) ConvertToSingleLogs(logGroup *LogGroup, targetFields []string) ([][]byte, [][]string, error) {
	marshaledLogs, desiredValues := make([][]byte, len(logGroup.Logs)), make([][]string, len(logGroup.Logs))
	for i, log := range logGroup.Logs {
		contents, tags := c.convertLogToMap(log, logGroup.LogTags, logGroup.Source, logGroup.Topic)

		desiredValue, err := findTargetValues(targetFields, contents, tags, c.TagRenameMap)
		if err != nil {
			return nil, nil, err
		}
		desiredValues[i] = desiredValue

		singleLog := make(map[string]interface{}, 3)
		if newKey, ok := c.ProtoKeyRenameMap["time"]; ok {
			singleLog[newKey] = log.Time
		} else {
			singleLog["time"] = log.Time
		}
		if newKey, ok := c.ProtoKeyRenameMap["contents"]; ok {
			singleLog[newKey] = contents
		} else {
			singleLog["contents"] = contents
		}
		if newKey, ok := c.ProtoKeyRenameMap["tags"]; ok {
			singleLog[newKey] = tags
		} else {
			singleLog["tags"] = tags
		}

		switch c.Encoding {
		case "json":
			b, err := json.Marshal(singleLog)
			if err != nil {
				return nil, nil, fmt.Errorf("unable to marshal log: %v", singleLog)
			}
			marshaledLogs[i] = b
		default:
			return nil, nil, fmt.Errorf("unsupported encoding format: %s", c.Encoding)
		}
	}
	return marshaledLogs, desiredValues, nil
}

func (c *Converter) convertLogToMap(log *Log, logTags []*LogTag, src, topic string) (map[string]string, map[string]string) {
	contents, tags := make(map[string]string), make(map[string]string, 13)
	inK8s := false
	for _, logContent := range log.Contents {
		if logContent.Key == "__tag__:__user_defined_id__" || logContent.Key == "__log_topic__" {
			continue
		}
		if strings.HasPrefix(logContent.Key, "__tag__") {
			if defaultTag, ok := tagConversionMap[logContent.Key[8:]]; ok {
				if newTag, ok := c.TagRenameMap[defaultTag]; ok {
					tags[newTag] = logContent.Value
				} else {
					tags[defaultTag] = logContent.Value
				}
				if logContent.Key == "__tag__:_pod_uid_" {
					inK8s = true
				}
			} else {
				if newTag, ok := c.TagRenameMap[logContent.Key[8:]]; ok {
					tags[newTag] = logContent.Value
				} else {
					tags[logContent.Key[8:]] = logContent.Value
				}
			}
		} else {
			contents[logContent.Key] = logContent.Value
		}
	}
	for _, logTag := range logTags {
		if logTag.Key == "__user_defined_id__" || logTag.Key == "__pack_id__" {
			continue
		}
		if defaultTag, ok := tagConversionMap[logTag.Key]; ok {
			if newTag, ok := c.TagRenameMap[defaultTag]; ok {
				tags[newTag] = logTag.Value
			} else {
				tags[defaultTag] = logTag.Value
			}
			if logTag.Key == "_pod_uid_" {
				inK8s = true
			}
		} else {
			if newTag, ok := c.TagRenameMap[logTag.Key]; ok {
				tags[newTag] = logTag.Value
			} else {
				tags[logTag.Key] = logTag.Value
			}
		}
	}
	tags["host.ip"] = src
	if topic != "" {
		tags["log.topic"] = topic
	}

	if inK8s {
		if v, ok := tags["container.name"]; ok {
			tags["k8s.container.name"] = v
			delete(tags, "container.name")
		}
		if v, ok := tags["container.ip"]; ok {
			tags["k8s.container.ip"] = v
			delete(tags, "container.ip")
		}
		if v, ok := tags["container.image.name"]; ok {
			tags["k8s.container.image.name"] = v
			delete(tags, "container.image.name")
		}
	}
	return contents, tags
}

func findTargetValues(targetFields []string, contents, tags, tagRenameMap map[string]string) ([]string, error) {
	if len(targetFields) == 0 {
		return nil, nil
	}

	desiredValue := make([]string, len(targetFields))
	for i, field := range targetFields {
		switch {
		case strings.HasPrefix(field, "content."):
			if value, ok := contents[field[8:]]; ok {
				desiredValue[i] = value
			}
		case strings.HasPrefix(field, "tag."):
			if value, ok := tags[field[4:]]; ok {
				desiredValue[i] = value
			} else if value, ok := tagRenameMap[field[4:]]; ok {
				desiredValue[i] = tags[value]
			}
		default:
			return nil, fmt.Errorf("unsupported field: %s", field)
		}
	}
	return desiredValue, nil
}
