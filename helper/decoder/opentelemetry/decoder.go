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

package opentelemetry

import (
	"encoding/json"
	"errors"
	"net/http"
	"strconv"
	"time"

	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

const (
	pbContentType   = "application/x-protobuf"
	jsonContentType = "application/json"
)

// Decoder impl
type Decoder struct {
	Format string
}

type jsonKVData map[string]interface{}
type jsonArrayData []interface{}

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	switch d.Format {
	case common.ProtocolOTLPLogV1:
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
	case common.ProtocolOTLPMetricV1:
		otlpMetricReq := pmetricotlp.NewRequest()
		switch req.Header.Get("Content-Type") {
		case pbContentType:
			if err = otlpMetricReq.UnmarshalProto(data); err != nil {
				return logs, err
			}
			logs, err = d.ConvertOtlpMetricV1(otlpMetricReq)
		case jsonContentType:
			if err = otlpMetricReq.UnmarshalJSON(data); err != nil {
				return logs, err
			}
			logs, err = d.ConvertOtlpMetricV1(otlpMetricReq)
		default:
			err = errors.New("Invalid ContentType: " + req.Header.Get("Content-Type"))
		}
	default:
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
						Value: strconv.FormatInt(logRecord.Timestamp().AsTime().UnixNano(), 10),
					},
					{
						Key:   "severity_number",
						Value: strconv.FormatInt(int64(logRecord.SeverityNumber()), 10),
					},
					{
						Key:   "severity_text",
						Value: logRecord.SeverityText(),
					},
					{
						Key:   "content",
						Value: logRecord.Body().AsString(),
					},
				}

				if logRecord.Attributes().Len() != 0 {
					if d, err := json.Marshal(logRecord.Attributes().AsRaw()); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "attributes",
							Value: string(d),
						})
					}
				}

				if resourceLog.Resource().Attributes().Len() != 0 {
					if d, err := json.Marshal(resourceLog.Resource().Attributes().AsRaw()); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "resources",
							Value: string(d),
						})
					}
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

func (d *Decoder) ConvertOtlpMetricV1(otlpMetricReq pmetricotlp.Request) (logs []*protocol.Log, err error) {
	resMetrics := otlpMetricReq.Metrics().ResourceMetrics()
	for i := 0; i < resMetrics.Len(); i++ {
		resourceMetric := resMetrics.At(i)
		sMetrics := resourceMetric.ScopeMetrics()
		for j := 0; j < sMetrics.Len(); j++ {
			scopeMetric := sMetrics.At(j)
			metrics := scopeMetric.Metrics()
			for k := 0; k < metrics.Len(); k++ {
				metric := metrics.At(k)

				protoContents := []*protocol.Log_Content{
					{
						Key:   "name",
						Value: metric.Name(),
					},
					{
						Key:   "description",
						Value: metric.Description(),
					},
					{
						Key:   "unit",
						Value: metric.Unit(),
					},
					{
						Key:   "data_type",
						Value: metric.DataType().String(),
					},
				}

				switch metric.DataType() {
				case pmetric.MetricDataTypeGauge:
					gauge := metric.Gauge()
					dataPoints := jsonArrayData{}
					for l := 0; l < gauge.DataPoints().Len(); l++ {
						dataPoints = append(dataPoints, gauge.DataPoints().At(i).Attributes().AsRaw())
					}
					data := jsonKVData{
						"data_points": dataPoints,
					}
					if d, err := json.Marshal(data); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "Gauge",
							Value: string(d),
						})
					}
				case pmetric.MetricDataTypeSum:
					sum := metric.Sum()

					data := jsonKVData{
						"is_monotonic":            sum.IsMonotonic(),
						"aggregation_temporality": sum.AggregationTemporality().String(),
					}
					if sum.DataPoints().Len() > 0 {
						dataPoints := jsonArrayData{}
						for l := 0; l < sum.DataPoints().Len(); l++ {
							dataPoints = append(dataPoints, sum.DataPoints().At(i).Attributes().AsRaw())
						}
						data["data_points"] = dataPoints
					}

					if d, err := json.Marshal(data); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "Sum",
							Value: string(d),
						})
					}
				case pmetric.MetricDataTypeHistogram:
					histogram := metric.Histogram()
					dataPoints := jsonArrayData{}
					for l := 0; l < histogram.DataPoints().Len(); l++ {
						dataPoints = append(dataPoints, histogram.DataPoints().At(i).Attributes().AsRaw())
					}
					data := jsonKVData{
						"data_points": dataPoints,
					}
					if d, err := json.Marshal(data); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "Histogram",
							Value: string(d),
						})
					}
				case pmetric.MetricDataTypeExponentialHistogram:
					exponentialHistogram := metric.ExponentialHistogram()
					dataPoints := jsonArrayData{}
					for l := 0; l < exponentialHistogram.DataPoints().Len(); l++ {
						dataPoints = append(dataPoints, exponentialHistogram.DataPoints().At(i).Attributes().AsRaw())
					}
					data := jsonKVData{
						"data_points": dataPoints,
					}
					if d, err := json.Marshal(data); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "ExponentialHistogram",
							Value: string(d),
						})
					}
				case pmetric.MetricDataTypeSummary:
					summary := metric.Summary()
					dataPoints := jsonArrayData{}
					for l := 0; l < summary.DataPoints().Len(); l++ {
						dataPoints = append(dataPoints, summary.DataPoints().At(i).Attributes().AsRaw())
					}
					data := jsonKVData{
						"data_points": dataPoints,
					}
					if d, err := json.Marshal(data); err == nil {
						protoContents = append(protoContents, &protocol.Log_Content{
							Key:   "Summary",
							Value: string(d),
						})
					}
				}

				protoLog := &protocol.Log{
					Time:     uint32(time.Now().Unix()),
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
