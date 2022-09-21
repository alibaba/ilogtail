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
	"time"

	commonv1 "go.opentelemetry.io/proto/otlp/common/v1"
	logv1 "go.opentelemetry.io/proto/otlp/logs/v1"
	resourcev1 "go.opentelemetry.io/proto/otlp/resource/v1"

	"github.com/alibaba/ilogtail/pkg/protocol"
)

func (c *Converter) ConvertToOtlpLogs(logGroup *protocol.LogGroup, targetFields []string) (*logv1.ResourceLogs, [][]string, error) {
	desiredValues := make([][]string, len(logGroup.Logs))
	attrs := make([]*commonv1.KeyValue, 0)
	attrs = append(attrs, c.convertToOtlpKeyValue("source", logGroup.Source), c.convertToOtlpKeyValue("topic", logGroup.Topic), c.convertToOtlpKeyValue("machine_uuid", logGroup.MachineUUID))
	for _, t := range logGroup.LogTags {
		attrs = append(attrs, c.convertToOtlpKeyValue(t.Key, t.Value))
	}
	logs := make([]*logv1.LogRecord, len(logGroup.Logs))
	for _, log := range logGroup.Logs {
		logs = append(logs, &logv1.LogRecord{
			TimeUnixNano: uint64(log.Time) * uint64(time.Second),
			//Body:
		})
	}
	instrumentLogs := []*logv1.InstrumentationLibraryLogs{{
		InstrumentationLibrary: &commonv1.InstrumentationLibrary{},
		Logs:                   logs,
	}}
	return &logv1.ResourceLogs{
		Resource: &resourcev1.Resource{
			Attributes: attrs,
		},
		InstrumentationLibraryLogs: instrumentLogs,
	}, desiredValues, nil
}

func (c *Converter) convertToOtlpKeyValue(key, value string) *commonv1.KeyValue {
	return &commonv1.KeyValue{Key: key, Value: &commonv1.AnyValue{Value: &commonv1.AnyValue_StringValue{StringValue: value}}}
}
