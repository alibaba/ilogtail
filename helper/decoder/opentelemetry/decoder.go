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

	"go.opentelemetry.io/collector/pdata/pcommon"
	"go.opentelemetry.io/collector/pdata/plog"
	"go.opentelemetry.io/collector/pdata/plog/plogotlp"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/pmetric/pmetricotlp"
	"go.opentelemetry.io/collector/pdata/ptrace"
	"go.opentelemetry.io/collector/pdata/ptrace/ptraceotlp"

	"github.com/alibaba/ilogtail/helper/decoder/common"
	"github.com/alibaba/ilogtail/pkg/models"
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

// Decode impl
func (d *Decoder) Decode(data []byte, req *http.Request) (logs []*protocol.Log, err error) {
	if common.ProtocolOTLPLogV1 == d.Format {
		otlpLogReq := plogotlp.NewExportRequest()
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

func (d *Decoder) ParseRequest(res http.ResponseWriter, req *http.Request, maxBodySize int64) (data []byte, statusCode int, err error) {
	return common.CollectBody(res, req, maxBodySize)
}

func (d *Decoder) DecodeV2(data []byte, req *http.Request) (groups []*models.PipelineGroupEvents, err error) {
	switch d.Format {
	case common.ProtocolOTLPLogV1:
		otlpLogReq := plogotlp.NewExportRequest()
		otlpLogReq, err = decodeOltpRequest(otlpLogReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpLogRequestToGroupEvents(otlpLogReq)
	case common.ProtocolOTLPMetricV1:
		otlpMetricReq := pmetricotlp.NewExportRequest()
		otlpMetricReq, err = decodeOltpRequest(otlpMetricReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpMetricRequestToGroupEvents(otlpMetricReq)
	case common.ProtocolOTLPTraceV1:
		otlpTraceReq := ptraceotlp.NewExportRequest()
		otlpTraceReq, err = decodeOltpRequest(otlpTraceReq, data, req)
		if err != nil {
			return groups, err
		}
		groups, err = ConvertOtlpTraceRequestToGroupEvents(otlpTraceReq)
	default:
		err = errors.New("Invalid RequestURI: " + req.RequestURI)
	}
	return groups, err
}

func decodeOltpRequest[P interface {
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

func ConvertOtlpMetricRequestToGroupEvents(otlpMetricReq pmetricotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpMetricsToGroupEvents(otlpMetricReq.Metrics())
}

func ConvertOtlpLogRequestToGroupEvents(otlpLogReq plogotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpLogsToGroupEvents(otlpLogReq.Logs())
}

func ConvertOtlpMetricsToGroupEvents(metrics pmetric.Metrics) (groupEventsSlice []*models.PipelineGroupEvents, err error) {
	// TODO:
	// convert oltp metrics to pipeline group events
	return nil, nil
}

func ConvertOtlpLogsToGroupEvents(logs plog.Logs) (groupEventsSlice []*models.PipelineGroupEvents, err error) {
	// TODO:
	// convert oltp logs to pipeline group events
	return nil, nil
}

func ConvertOtlpTraceRequestToGroupEvents(otlpTraceReq ptraceotlp.ExportRequest) ([]*models.PipelineGroupEvents, error) {
	return ConvertOtlpTracesToGroupEvents(otlpTraceReq.Traces())
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
			scopeAttributes := scope.Attributes()
			otSpans := scopeSpan.Spans()

			groupEvents := &models.PipelineGroupEvents{
				Group:  models.NewGroup(attrs2Meta(resourceAttrs), attrs2Tags(scopeAttributes)),
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
					span.Tags.Add(KeyMessage, message)
				}

				if count := otSpan.DroppedEventsCount(); count > 0 {
					span.Tags.Add(KeyDroppedEventsCount, strconv.Itoa(int(count)))
				}

				if count := otSpan.DroppedAttributesCount(); count > 0 {
					span.Tags.Add(KeyDroppedAttrsCount, strconv.Itoa(int(count)))
				}

				if count := otSpan.DroppedLinksCount(); count > 0 {
					span.Tags.Add(KeyDroppedLinksCount, strconv.Itoa(int(count)))
				}

				groupEvents.Events = append(groupEvents.Events, span)
			}
			groupEventsSlice = append(groupEventsSlice, groupEvents)
		}

	}
	return groupEventsSlice, err
}

func convertSpanLinks(srcLinks ptrace.SpanLinkSlice) []*models.SpanLink {
	spanLinks := make([]*models.SpanLink, 0, srcLinks.Len())

	for i := 0; i < srcLinks.Len(); i++ {
		srcLink := srcLinks.At(i)

		spanLink := models.SpanLink{
			TraceID:    srcLink.TraceID().String(),
			SpanID:     srcLink.SpanID().String(),
			TraceState: srcLink.TraceState().AsRaw(),
			Tags:       attrs2Tags(srcLink.Attributes()),
		}
		spanLinks = append(spanLinks, &spanLink)
	}
	return spanLinks
}

func convertSpanEvents(srcEvents ptrace.SpanEventSlice) []*models.SpanEvent {
	events := make([]*models.SpanEvent, 0, srcEvents.Len())
	for i := 0; i < srcEvents.Len(); i++ {
		srcEvent := srcEvents.At(i)

		spanEvent := &models.SpanEvent{
			Name:      srcEvent.Name(),
			Timestamp: int64(srcEvent.Timestamp()),
			Tags:      attrs2Tags(srcEvent.Attributes()),
		}
		events = append(events, spanEvent)
	}
	return events
}

func attrs2Tags(attributes pcommon.Map) models.Tags {
	if attributes.Len() == 0 {
		return nil
	}
	tags := models.NewTags()
	attributes.Range(
		func(k string, v pcommon.Value) bool {
			tags.Add(k, v.AsString())
			return true
		},
	)
	return tags
}

func attrs2Meta(attributes pcommon.Map) models.Metadata {
	if attributes.Len() == 0 {
		return nil
	}
	meta := models.NewMetadata()
	attributes.Range(
		func(k string, v pcommon.Value) bool {
			meta.Add(k, v.AsString())
			return true
		},
	)
	return meta
}
