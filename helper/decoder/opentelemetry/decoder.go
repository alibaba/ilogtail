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

package opentelemetry

import (
	"errors"
	"fmt"
	"net/http"
	"strconv"
	"strings"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"go.opentelemetry.io/collector/pdata/pcommon"
	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
)

const (
	pbContentType   = "application/x-protobuf"
	jsonContentType = "application/json"
)

// Decoder impl
type Decoder struct {
}

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	reqUri := req.RequestURI
	if "/v1/logs" == reqUri {
		otlpLogReq := plogotlp.NewRequest()
		switch req.Header.Get("Content-Type") {
		case pbContentType:
			if err = otlpLogReq.UnmarshalProto(data); err != nil {
				return logs, err
			}
			logs, err = d.ConvertOtlpLogV1(otlpLogReq)
		case jsonContentType:
			if err = otlpLogReq.UnmarshalJSON(data); err != nil {
				return logs, err
			}
			logs, err = d.ConvertOtlpLogV1(otlpLogReq)
		default:
			err = errors.New("Invalid ContentType: " + req.Header.Get("Content-Type"))
		}
	} else {
		err = errors.New("Invalid RequestURI: " + req.RequestURI)
	}
	return logs, err
}

func (d *Decoder) ConvertOtlpLogV1(otlpLogReq plogotlp.Request) (logs []*protocol.Log, err error) {
	rls := otlpLogReq.Logs().ResourceLogs()
	for i := 0; i < rls.Len(); i++ {
		rl := rls.At(i)
		// buf.logEntry("Resource SchemaURL: %s", rl.SchemaUrl())
		// buf.logAttributes("Resource attributes", rl.Resource().Attributes())
		ills := rl.ScopeLogs()
		for j := 0; j < ills.Len(); j++ {
			ils := ills.At(j)
			//buf.logEntry("ScopeLogs SchemaURL: %s", ils.SchemaUrl())
			//buf.logInstrumentationScope(ils.Scope())

			logRecords := ils.LogRecords()
			for k := 0; k < logRecords.Len(); k++ {
				lr := logRecords.At(k)
				// buf.logEntry("ObservedTimestamp: %s", lr.ObservedTimestamp())
				// buf.logEntry("Timestamp: %s", lr.Timestamp())
				// buf.logEntry("Severity: %s", lr.SeverityText())

				log := &protocol.Log{
					Time: uint32(lr.Timestamp().AsTime().Unix()),
					Contents: []*protocol.Log_Content{
						{
							Key:   "timestamp",
							Value: lr.Timestamp().String(),
						},
						{
							Key:   "body",
							Value: attributeValueToString(lr.Body()),
						},
					},
				}
				logs = append(logs, log)
			}
		}
	}

	return logs, nil
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

func attributeValueToString(v pcommon.Value) string {
	switch v.Type() {
	case pcommon.ValueTypeString:
		return v.StringVal()
	case pcommon.ValueTypeBool:
		return strconv.FormatBool(v.BoolVal())
	case pcommon.ValueTypeDouble:
		return strconv.FormatFloat(v.DoubleVal(), 'f', -1, 64)
	case pcommon.ValueTypeInt:
		return strconv.FormatInt(v.IntVal(), 10)
	case pcommon.ValueTypeSlice:
		return sliceToString(v.SliceVal())
	case pcommon.ValueTypeMap:
		return mapToString(v.MapVal())
	default:
		return fmt.Sprintf("<Unknown OpenTelemetry attribute value type %q>", v.Type())
	}
}

func sliceToString(s pcommon.Slice) string {
	var b strings.Builder
	b.WriteByte('[')
	for i := 0; i < s.Len(); i++ {
		if i < s.Len()-1 {
			fmt.Fprintf(&b, "%s, ", attributeValueToString(s.At(i)))
		} else {
			b.WriteString(attributeValueToString(s.At(i)))
		}
	}

	b.WriteByte(']')
	return b.String()
}

func mapToString(m pcommon.Map) string {
	var b strings.Builder
	b.WriteString("{\n")

	m.Sort().Range(func(k string, v pcommon.Value) bool {
		fmt.Fprintf(&b, "     -> %s: %s(%s)\n", k, v.Type(), v.AsString())
		return true
	})
	b.WriteByte('}')
	return b.String()
}
