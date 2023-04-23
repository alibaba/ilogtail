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
	"fmt"
	"net/http"
	"strconv"
	"time"

	"go.opentelemetry.io/collector/pdata/pcommon"
	"go.opentelemetry.io/collector/pdata/plog"
	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"
	"go.opentelemetry.io/collector/pdata/ptrace"
	"go.opentelemetry.io/collector/pdata/ptrace/ptraceotlp"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/protocol/decoder/common"
	"github.com/alibaba/ilogtail/pkg/protocol/otlp"
	"github.com/alibaba/ilogtail/pkg/util"
)

const (
	pbContentType   = "application/x-protobuf"
	jsonContentType = "application/json"
)

const (
	logEventName = "log_event"
)

// Decoder impl
type Decoder struct {
	Format string
}

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request, tags map[string]string) (logs []*protocol.Log, err error) {
	switch d.Format {
	case common.ProtocolOTLPLogV1:
		otlpLogReq := plogotlp.NewExportRequest()
		otlpLogReq, err = DecodeOtlpRequest(otlpLogReq, data, req)
		if err != nil {
			return logs, err
		}
		logs, err = d.ConvertOtlpLogV1(otlpLogReq)
	case common.ProtocolOTLPMetricV1:
		otlpMetricReq := pmetricotlp.NewExportRequest()
		otlpMetricReq, err = DecodeOtlpRequest(otlpMetricReq, data, req)
		if err != nil {
			return logs, err
		}
		logs, err = d.ConvertOtlpMetricV1(otlpMetricReq)
	case common.ProtocolOTLPTraceV1:
		otlpTraceReq := ptraceotlp.NewExportRequest()
		otlpTraceReq, err = DecodeOtlpRequest(otlpTraceReq, data, req)
		if err != nil {
			return logs, err
		}
		logs, err = d.ConvertOtlpTraceV1(otlpTraceReq)
	default:
		err = errors.New("Invalid RequestURI: " + req.RequestURI)
	}
	return logs, err
}

// ParseRequest impl
func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

// DecodeV2 impl
func (d *Decoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	switch d.Format {
	case common.ProtocolOTLPLogV1:
		otlpLogReq := plogotlp.NewExportRequest()
		otlpLogReq, err = DecodeOtlpRequest(otlpLogReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpLogRequestToGroupEvents(otlpLogReq)
	case common.ProtocolOTLPMetricV1:
		otlpMetricReq := pmetricotlp.NewExportRequest()
		otlpMetricReq, err = DecodeOtlpRequest(otlpMetricReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpMetricRequestToGroupEvents(otlpMetricReq)
	case common.ProtocolOTLPTraceV1:
		otlpTraceReq := ptraceotlp.NewExportRequest()
		otlpTraceReq, err = DecodeOtlpRequest(otlpTraceReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpTraceRequestToGroupEvents(otlpTraceReq)
	default:
		err = errors.New("Invalid RequestURI: " + req.RequestURI)
	}
	return groups, err
}

// DecodeOtlpRequest decodes the data and fills into the otlp logs/metrics/traces export request.
func DecodeOtlpRequest[P interface {
	UnmarshalProto(data []byte) error
	UnmarshalJSON(data []byte) error
}](des P, data []byte, req *http.Request) (P, error) {
	var err error
	switch req.Header.Get("Content-Type") {
	case pbContentType:
		if err = des.UnmarshalProto(data); err != nil {
			return des, err
		}
	case jsonContentType:
		if err = des.UnmarshalJSON(data); err != nil {
			return des, err
		}
	default:
		err = errors.New("Invalid ContentType: " + req.Header.Get("Content-Type"))
	}
	return des, err
}

func (d *Decoder) ConvertOtlpLogV1(otlpLogReq plogotlp.ExportRequest) (logs []*protocol.Log, err error) {
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

func (d *Decoder) ConvertOtlpMetricV1(otlpMetricReq pmetricotlp.ExportRequest) (logs []*protocol.Log, err error) {
	resMetrics := otlpMetricReq.Metrics().ResourceMetrics()
	resMetricsLen := resMetrics.Len()

	if 0 == resMetricsLen {
		return
	}

	for i := 0; i < resMetricsLen; i++ {
		resMetricsSlice := resMetrics.At(i)
		var labels KeyValues
		res2Labels(&labels, resMetricsSlice.Resource())

		scopeMetrics := resMetricsSlice.ScopeMetrics()
		for j := 0; j < scopeMetrics.Len(); j++ {
			otMetrics := scopeMetrics.At(j).Metrics()

			for k := 0; k < otMetrics.Len(); k++ {
				otMetric := otMetrics.At(k)
				metricName := otMetric.Name()

				switch otMetric.Type() {
				case pmetric.MetricTypeGauge:
					otGauge := otMetric.Gauge()
					otDatapoints := otGauge.DataPoints()
					logs = append(logs, GaugeToLogs(metricName, otDatapoints, labels)...)
				case pmetric.MetricTypeSum:
					otSum := otMetric.Sum()
					isMonotonic := strconv.FormatBool(otSum.IsMonotonic())
					aggregationTemporality := otSum.AggregationTemporality()
					otDatapoints := otSum.DataPoints()
					logs = append(logs, SumToLogs(metricName, aggregationTemporality, isMonotonic, otDatapoints, labels)...)
				case pmetric.MetricTypeSummary:
					otSummary := otMetric.Summary()
					otDatapoints := otSummary.DataPoints()
					logs = append(logs, SummaryToLogs(metricName, otDatapoints, labels)...)
				case pmetric.MetricTypeHistogram:
					otHistogram := otMetric.Histogram()
					aggregationTemporality := otHistogram.AggregationTemporality()
					otDatapoints := otHistogram.DataPoints()
					logs = append(logs, HistogramToLogs(metricName, otDatapoints, aggregationTemporality, labels)...)
				case pmetric.MetricTypeExponentialHistogram:
					otExponentialHistogram := otMetric.ExponentialHistogram()
					aggregationTemporality := otExponentialHistogram.AggregationTemporality()
					otDatapoints := otExponentialHistogram.DataPoints()
					logs = append(logs, ExponentialHistogramToLogs(metricName, otDatapoints, aggregationTemporality, labels)...)
				default:
					// TODO:
					// find a better way to handle metric with type MetricTypeEmpty.
					log := &protocol.Log{
						Time: uint32(time.Now().Unix()),
						Contents: []*protocol.Log_Content{
							{
								Key:   metricNameKey,
								Value: otMetric.Name(),
							},
							{
								Key:   labelsKey,
								Value: otMetric.Type().String(),
							},
							{
								Key:   timeNanoKey,
								Value: strconv.FormatInt(time.Now().UnixNano(), 10),
							},
							{
								Key:   valueKey,
								Value: otMetric.Description(),
							},
						},
					}
					logs = append(logs, log)
				}
			}
		}
	}

	return logs, err
}

func (d *Decoder) ConvertOtlpTraceV1(otlpTraceReq ptraceotlp.ExportRequest) (logs []*protocol.Log, err error) {
	return logs, fmt.Errorf("does_not_support_otlptraces")
}

func ConvertOtlpLogRequestToGroupEvents(otlpLogReq plogotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpLogsToGroupEvents(otlpLogReq.Logs())
}

func ConvertOtlpMetricRequestToGroupEvents(otlpMetricReq pmetricotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpMetricsToGroupEvents(otlpMetricReq.Metrics())
}

func ConvertOtlpTraceRequestToGroupEvents(otlpTraceReq ptraceotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpTracesToGroupEvents(otlpTraceReq.Traces())
}

func ConvertOtlpLogsToGroupEvents(logs plog.Logs) (groupEventsSlice []*models.PipelineGroupEvents, err error) {
	resLogs := logs.ResourceLogs()
	resLogsLen := resLogs.Len()

	if resLogsLen == 0 {
		return
	}

	for i := 0; i < resLogsLen; i++ {
		resourceLog := resLogs.At(i)
		resourceAttrs := resourceLog.Resource().Attributes()
		scopeLogs := resourceLog.ScopeLogs()
		scopeLogsLen := scopeLogs.Len()

		for j := 0; j < scopeLogsLen; j++ {
			scopeLog := scopeLogs.At(j)
			scope := scopeLog.Scope()
			scopeTags := genScopeTags(scope)
			otLogs := scopeLog.LogRecords()
			otLogsLen := otLogs.Len()

			groupEvents := &models.PipelineGroupEvents{
				Group:  models.NewGroup(attrs2Meta(resourceAttrs), scopeTags),
				Events: make([]models.PipelineEvent, 0, otLogs.Len()),
			}

			for k := 0; k < otLogsLen; k++ {
				logRecord := otLogs.At(k)

				var body []byte
				switch logRecord.Body().Type() {
				case pcommon.ValueTypeBytes:
					body = logRecord.Body().Bytes().AsRaw()
				case pcommon.ValueTypeStr:
					body = util.ZeroCopyStringToBytes(logRecord.Body().AsString())
				default:
					body = util.ZeroCopyStringToBytes(fmt.Sprintf("%#v", logRecord.Body().AsRaw()))
				}

				level := logRecord.SeverityText()
				if level == "" {
					level = logRecord.SeverityNumber().String()
				}

				event := models.NewLog(
					logEventName,
					body,
					level,
					logRecord.SpanID().String(),
					logRecord.TraceID().String(),
					attrs2Tags(logRecord.Attributes()),
					uint64(logRecord.Timestamp().AsTime().UnixNano()),
				)
				event.ObservedTimestamp = uint64(logRecord.ObservedTimestamp().AsTime().UnixNano())
				event.Tags.Add(otlp.TagKeyLogFlag, strconv.Itoa(int(logRecord.Flags())))
				groupEvents.Events = append(groupEvents.Events, event)
			}

			groupEventsSlice = append(groupEventsSlice, groupEvents)
		}
	}

	return groupEventsSlice, err
}

func ConvertOtlpMetricsToGroupEvents(metrics pmetric.Metrics) (groupEventsSlice []*models.PipelineGroupEvents, err error) {
	resMetrics := metrics.ResourceMetrics()
	if resMetrics.Len() == 0 {
		return
	}

	for i := 0; i < resMetrics.Len(); i++ {
		resourceMetric := resMetrics.At(i)
		resourceAttrs := resourceMetric.Resource().Attributes()
		scopeMetrics := resourceMetric.ScopeMetrics()

		for j := 0; j < scopeMetrics.Len(); j++ {
			scopeMetric := scopeMetrics.At(j)
			scope := scopeMetric.Scope()
			scopeTags := genScopeTags(scope)
			otMetrics := scopeMetric.Metrics()

			groupEvents := &models.PipelineGroupEvents{
				Group:  models.NewGroup(attrs2Meta(resourceAttrs), scopeTags),
				Events: make([]models.PipelineEvent, 0, otMetrics.Len()),
			}

			for k := 0; k < otMetrics.Len(); k++ {
				otMetric := otMetrics.At(k)
				metricName := otMetric.Name()
				metricUnit := otMetric.Unit()
				metricDescription := otMetric.Description()

				switch otMetric.Type() {
				case pmetric.MetricTypeGauge:
					otGauge := otMetric.Gauge()
					otDatapoints := otGauge.DataPoints()
					for l := 0; l < otDatapoints.Len(); l++ {
						datapoint := otDatapoints.At(l)
						metric := newMetricFromGaugeDatapoint(datapoint, metricName, metricUnit, metricDescription)
						groupEvents.Events = append(groupEvents.Events, metric)
					}
				case pmetric.MetricTypeSum:
					otSum := otMetric.Sum()
					isMonotonic := strconv.FormatBool(otSum.IsMonotonic())
					aggregationTemporality := otSum.AggregationTemporality()
					otDatapoints := otSum.DataPoints()

					for l := 0; l < otDatapoints.Len(); l++ {
						datapoint := otDatapoints.At(l)
						metric := newMetricFromSumDatapoint(datapoint, aggregationTemporality, isMonotonic, metricName, metricUnit, metricDescription)
						groupEvents.Events = append(groupEvents.Events, metric)
					}
				case pmetric.MetricTypeSummary:
					otSummary := otMetric.Summary()
					otDatapoints := otSummary.DataPoints()
					for l := 0; l < otDatapoints.Len(); l++ {
						datapoint := otDatapoints.At(l)
						metric := newMetricFromSummaryDatapoint(datapoint, metricName, metricUnit, metricDescription)
						groupEvents.Events = append(groupEvents.Events, metric)
					}
				case pmetric.MetricTypeHistogram:
					otHistogram := otMetric.Histogram()
					aggregationTemporality := otHistogram.AggregationTemporality()
					otDatapoints := otHistogram.DataPoints()

					for l := 0; l < otDatapoints.Len(); l++ {
						datapoint := otDatapoints.At(l)
						metric := newMetricFromHistogramDatapoint(datapoint, aggregationTemporality, metricName, metricUnit, metricDescription)
						groupEvents.Events = append(groupEvents.Events, metric)
					}
				case pmetric.MetricTypeExponentialHistogram:
					otExponentialHistogram := otMetric.ExponentialHistogram()
					aggregationTemporality := otExponentialHistogram.AggregationTemporality()
					otDatapoints := otExponentialHistogram.DataPoints()

					for l := 0; l < otDatapoints.Len(); l++ {
						datapoint := otDatapoints.At(l)
						metric := newMetricFromExponentialHistogramDatapoint(datapoint, aggregationTemporality, metricName, metricUnit, metricDescription)
						groupEvents.Events = append(groupEvents.Events, metric)
					}
				default:
					// TODO:
					// find a better way to handle metric with type MetricTypeEmpty.
					metric := models.NewSingleValueMetric(metricName, models.MetricTypeUntyped, models.NewTags(), time.Now().Unix(), 0)
					metric.Unit = metricUnit
					metric.Description = otMetric.Description()
					groupEvents.Events = append(groupEvents.Events, metric)
				}

			}
			groupEventsSlice = append(groupEventsSlice, groupEvents)
		}
	}
	return groupEventsSlice, err
}

func ConvertOtlpTracesToGroupEvents(traces ptrace.Traces) (groupEventsSlice []*models.PipelineGroupEvents, err error) {
	resSpans := traces.ResourceSpans()

	for i := 0; i < resSpans.Len(); i++ {
		resourceSpan := resSpans.At(i)
		// A Resource is an immutable representation of the entity producing telemetry as Attributes.
		// These attributes should be stored in meta.
		resourceAttrs := resourceSpan.Resource().Attributes()
		scopeSpans := resourceSpan.ScopeSpans()

		for j := 0; j < scopeSpans.Len(); j++ {
			scopeSpan := scopeSpans.At(j)
			scope := scopeSpan.Scope()
			scopeTags := genScopeTags(scope)
			otSpans := scopeSpan.Spans()

			groupEvents := &models.PipelineGroupEvents{
				Group:  models.NewGroup(attrs2Meta(resourceAttrs), scopeTags),
				Events: make([]models.PipelineEvent, 0, otSpans.Len()),
			}

			for k := 0; k < otSpans.Len(); k++ {
				otSpan := otSpans.At(k)

				spanName := otSpan.Name()
				traceID := otSpan.TraceID().String()
				spanID := otSpan.SpanID().String()
				spanKind := models.SpanKind(otSpan.Kind())
				startTs := otSpan.StartTimestamp()
				endTs := otSpan.EndTimestamp()
				spanAttrs := otSpan.Attributes()
				events := convertSpanEvents(otSpan.Events())
				links := convertSpanLinks(otSpan.Links())

				span := models.NewSpan(spanName, traceID, spanID, spanKind,
					uint64(startTs), uint64(endTs), attrs2Tags(spanAttrs), events, links)

				span.ParentSpanID = otSpan.ParentSpanID().String()
				span.Status = models.StatusCode(otSpan.Status().Code())

				if message := otSpan.Status().Message(); len(message) > 0 {
					span.Tags.Add(otlp.TagKeySpanStatusMessage, message)
				}

				span.Tags.Add(otlp.TagKeySpanDroppedEventsCount, strconv.Itoa(int(otSpan.DroppedEventsCount())))
				span.Tags.Add(otlp.TagKeySpanDroppedAttrsCount, strconv.Itoa(int(otSpan.DroppedAttributesCount())))
				span.Tags.Add(otlp.TagKeySpanDroppedLinksCount, strconv.Itoa(int(otSpan.DroppedLinksCount())))

				groupEvents.Events = append(groupEvents.Events, span)
			}
			groupEventsSlice = append(groupEventsSlice, groupEvents)
		}

	}
	return groupEventsSlice, err
}
