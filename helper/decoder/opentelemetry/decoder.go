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
	"encoding/json"
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
	reqURI := req.RequestURI
	if "/v1/logs" == reqURI {
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
	resLogs := otlpLogReq.Logs().ResourceLogs()
	for i := 0; i < resLogs.Len(); i++ {
		resourceLog := resLogs.At(i)
		sLogs := resourceLog.ScopeLogs()
		for j := 0; j < sLogs.Len(); j++ {
			scopeLog := sLogs.At(j)
			lRecords := scopeLog.LogRecords()
			for k := 0; k < lRecords.Len(); k++ {
				logRecord := lRecords.At(k)

				protoContents := []*protocol.Log_Content{
					{
						Key:   "time_unix_nano",
						Value: logRecord.Timestamp().String(),
					},
					{
						Key:   "observed_time_unix_nano",
						Value: logRecord.ObservedTimestamp().String(),
					},
					{
						Key:   "severity_number",
						Value: logRecord.SeverityNumber().String(),
					},
					{
						Key:   "severity_text",
						Value: logRecord.SeverityText(),
					},
					{
						Key:   "content",
						Value: attributeValueToString(logRecord.Body()),
					},
				}

				if logRecord.Attributes().Len() != 0 {
					d, _ := json.Marshal(logRecord.Attributes().AsRaw())

					protoContents = append(protoContents, &protocol.Log_Content{
						Key:   "attributes",
						Value: string(d),
					})
				}

				if resourceLog.Resource().Attributes().Len() != 0 {
					d, _ := json.Marshal(resourceLog.Resource().Attributes().AsRaw())

					protoContents = append(protoContents, &protocol.Log_Content{
						Key:   "resources",
						Value: string(d),
					})
				}

				protoLog := &protocol.Log{
					Time:     uint32(logRecord.Timestamp().AsTime().Unix()),
					Contents: protoContents,
				}
				logs = append(logs, protoLog)
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
