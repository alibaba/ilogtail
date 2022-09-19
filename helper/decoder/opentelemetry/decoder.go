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
	"net/http"
	"strconv"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"
)

const (
	pbContentType   = "application/x-protobuf"
	jsonContentType = "application/json"
)

const (
	metricNameKey = "__name__"
	labelsKey     = "__labels__"
	timeNanoKey   = "__time_nano__"
	valueKey      = "__value__"
)

// Decoder impl
type Decoder struct {
}

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	otlpMetricReq := pmetricotlp.NewRequest()
	switch req.Header.Get("Content-Type") {
	case pbContentType:
		err = otlpMetricReq.UnmarshalProto(data)
		logs, err = d.ConvertOtlpMetric(otlpMetricReq)
	case jsonContentType:
		err = otlpMetricReq.UnmarshalJSON(data)
		logs, err = d.ConvertOtlpMetric(otlpMetricReq)
	default:

	}

	return logs, err
}

func (d *Decoder) ConvertOtlpMetric(otlpMetricReq pmetricotlp.Request) (logs []*protocol.Log, err error) {
	rms := otlpMetricReq.Metrics().ResourceMetrics()
	for i := 0; i < rms.Len(); i++ {
		rm := rms.At(i)
		ilms := rm.ScopeMetrics()
		for j := 0; j < ilms.Len(); j++ {
			ilm := ilms.At(j)
			metrics := ilm.Metrics()
			for k := 0; k < metrics.Len(); k++ {
				metric := metrics.At(k)
				metricName := metric.Name()
				switch metric.DataType() {
				case pmetric.MetricDataTypeNone:
					break
				case pmetric.MetricDataTypeGauge:
				case pmetric.MetricDataTypeSum:
					metricDatas := metric.Sum().DataPoints()
					for i := 0; i < metricDatas.Len(); i++ {
						m := metricDatas.At(i)

						log := &protocol.Log{
							Time: uint32(m.Timestamp().AsTime().Unix()),
							Contents: []*protocol.Log_Content{
								{
									Key:   metricNameKey,
									Value: metricName,
								},
								{
									Key:   labelsKey,
									Value: "",
								},
								{
									Key:   timeNanoKey,
									Value: strconv.FormatInt(m.Timestamp().AsTime().UnixMicro(), 10),
								},
								{
									Key:   valueKey,
									Value: strconv.FormatFloat(m.DoubleVal(), 'g', -1, 64),
								},
							},
						}
						logs = append(logs, log)
					}

				case pmetric.MetricDataTypeHistogram:
				case pmetric.MetricDataTypeExponentialHistogram:
				case pmetric.MetricDataTypeSummary:
				}

			}
		}
	}
	return logs, nil
}

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}
