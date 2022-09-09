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

	sls "github.com/alibaba/ilogtail/pkg/protocol/sls"
)

const (
	protocolSingle = "single"
)

const (
	encodingJSON     = "json"
	encodingProtobuf = "protobuf"
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
	protocolSingle: {
		encodingJSON:     true,
		encodingProtobuf: false,
	},
}

type Converter struct {
	Protocol             string
	Encoding             string
	TagKeyRenameMap      map[string]string
	ProtocolKeyRenameMap map[string]string
}

func NewConverter(protocol, encoding string, tagKeyRenameMap, protocolKeyRenameMap map[string]string) (*Converter, error) {
	enc, ok := supportedEncodingMap[protocol]
	if !ok {
		return nil, fmt.Errorf("unsupported protocol: %s", protocol)
	}
	if supported, existed := enc[encoding]; !existed || !supported {
		return nil, fmt.Errorf("unsupported encoding: %s for protocol %s", encoding, protocol)
	}
	return &Converter{
		Protocol:             protocol,
		Encoding:             encoding,
		TagKeyRenameMap:      tagKeyRenameMap,
		ProtocolKeyRenameMap: protocolKeyRenameMap,
	}, nil
}

func (c *Converter) Do(logGroup *sls.LogGroup) (logs [][]byte, err error) {
	logs, _, err = c.DoWithSelectedFields(logGroup, nil)
	return
}

func (c *Converter) DoWithSelectedFields(logGroup *sls.LogGroup, targetFields []string) (logs [][]byte, values [][]string, err error) {
	switch c.Protocol {
	case protocolSingle:
		return c.ConvertToSingleLogs(logGroup, targetFields)
	default:
		return nil, nil, fmt.Errorf("unsupported protocol: %s", c.Protocol)
	}
}

func convertLogToMap(log *sls.Log, logTags []*sls.LogTag, src, topic string, tagKeyRenameMap map[string]string) (map[string]string, map[string]string) {
	contents, tags := make(map[string]string), make(map[string]string, 13)
	inK8s := false
	for _, logContent := range log.Contents {
		if logContent.Key == "__tag__:__user_defined_id__" {
			continue
		}
		if strings.HasPrefix(logContent.Key, "__tag__") {
			if defaultTag, ok := tagConversionMap[logContent.Key[8:]]; ok {
				if newTag, ok := tagKeyRenameMap[defaultTag]; ok {
					tags[newTag] = logContent.Value
				} else {
					tags[defaultTag] = logContent.Value
				}
				if logContent.Key == "__tag__:_pod_uid_" {
					inK8s = true
				}
			} else {
				if newTag, ok := tagKeyRenameMap[logContent.Key[8:]]; ok {
					tags[newTag] = logContent.Value
				} else {
					tags[logContent.Key[8:]] = logContent.Value
				}
			}
		} else {
			if logContent.Key == "__log_topic__" {
				tags["log.topic"] = logContent.Value
			} else {
				contents[logContent.Key] = logContent.Value
			}
		}
	}
	for _, logTag := range logTags {
		if logTag.Key == "__user_defined_id__" || logTag.Key == "__pack_id__" {
			continue
		}
		if defaultTag, ok := tagConversionMap[logTag.Key]; ok {
			if newTag, ok := tagKeyRenameMap[defaultTag]; ok {
				tags[newTag] = logTag.Value
			} else {
				tags[defaultTag] = logTag.Value
			}
			if logTag.Key == "_pod_uid_" {
				inK8s = true
			}
		} else {
			if newTag, ok := tagKeyRenameMap[logTag.Key]; ok {
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

func findTargetValues(targetFields []string, contents, tags, tagKeyRenameMap map[string]string) ([]string, error) {
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
			} else if value, ok := tagKeyRenameMap[field[4:]]; ok {
				desiredValue[i] = tags[value]
			}
		default:
			return nil, fmt.Errorf("unsupported field: %s", field)
		}
	}
	return desiredValue, nil
}
