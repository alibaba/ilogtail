package helper

import (
	"fmt"
	"sync"
	"time"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/util"
)

var LogEventPool = sync.Pool{
	New: func() interface{} {
		return new(protocol.LogEvent)
	},
}

func CreateLogEvent(t time.Time, enableTimestampNano bool, fields map[string]string) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Timestamp = (uint64(t.Unix()) << 32) | uint64(t.Nanosecond())
	if len(logEvent.Contents) < len(fields) {
		slice := make([]*protocol.LogEvent_Content, len(logEvent.Contents), len(fields))
		copy(slice, logEvent.Contents)
		logEvent.Contents = slice
	} else {
		logEvent.Contents = logEvent.Contents[:len(fields)]
	}
	i := 0
	rawSize := 0
	for key, val := range fields {
		if i >= len(logEvent.Contents) {
			logEvent.Contents = append(logEvent.Contents, &protocol.LogEvent_Content{})
		}
		logEvent.Contents[i].Key = util.ZeroCopyStringToBytes(key)
		logEvent.Contents[i].Value = util.ZeroCopyStringToBytes(val)
		i++
		rawSize += len(val)
	}
	logEvent.FileOffset = uint64(rawSize)
	logEvent.RawSize = uint64(rawSize)
	return logEvent, nil
}

func CreateLogEventByArray(t time.Time, enableTimestampNano bool, columns []string, values []string) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Timestamp = (uint64(t.Unix()) << 32) | uint64(t.Nanosecond())
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(columns))
	if len(columns) != len(values) {
		return nil, fmt.Errorf("columns and values not equal")
	}
	rawSize := 0
	for index := range columns {
		if index >= len(logEvent.Contents) {
			logEvent.Contents = append(logEvent.Contents, &protocol.LogEvent_Content{})
		}
		logEvent.Contents[index].Key = util.ZeroCopyStringToBytes(columns[index])
		logEvent.Contents[index].Value = util.ZeroCopyStringToBytes(values[index])
		rawSize += len(values[index])
	}
	logEvent.FileOffset = uint64(rawSize)
	logEvent.RawSize = uint64(rawSize)
	return logEvent, nil
}

func CreateLogEventByRawLogV1(log *protocol.Log) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Timestamp = (uint64(log.GetTime()) << 32) | uint64(log.GetTimeNs())
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(log.Contents))
	rawSize := 0
	for i, logC := range log.Contents {
		if i >= len(logEvent.Contents) {
			logEvent.Contents = append(logEvent.Contents, &protocol.LogEvent_Content{})
		}
		logEvent.Contents[i].Key = util.ZeroCopyStringToBytes(logC.Key)
		logEvent.Contents[i].Value = util.ZeroCopyStringToBytes(logC.Value)
		rawSize += len(logC.Value)
	}
	logEvent.FileOffset = uint64(rawSize)
	logEvent.RawSize = uint64(rawSize)
	return logEvent, nil
}

func CreateLogEventByRawLogV2(log *models.Log) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	ts := log.GetTimestamp() / 1e9
	tns := log.GetTimestamp() % 1e9
	logEvent.Timestamp = (ts << 32) | tns
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, log.Contents.Len())
	for k, v := range log.Contents.Iterator() {
		cont := &protocol.LogEvent_Content{
			Key:   util.ZeroCopyStringToBytes(k),
			Value: util.ZeroCopyStringToBytes(v.(string)),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Level = log.GetLevel()
	logEvent.FileOffset = log.GetOffset()
	logEvent.RawSize = log.GetRawSize()
	return logEvent, nil
}

func CreateMetricEventByRawMetricV2(metric *models.Metric) (*protocol.MetricEvent, error) {
	var metricEvent protocol.MetricEvent
	ts := metric.GetTimestamp() / 1e9
	tns := metric.GetTimestamp() % 1e9
	metricEvent.Timestamp = (ts << 32) | tns
	metricEvent.Name = metric.Name
	if metric.GetValue().IsSingleValue() {
		metricEvent.Value = &protocol.MetricEvent_UntypedSingleValue{UntypedSingleValue: &protocol.UntypedSingleValue{Value: metric.Value.GetSingleValue()}}
	} else {
		return nil, fmt.Errorf("unsupported metric value type")
	}
	metricEvent.Tags = make(map[string][]byte, metric.GetTags().Len())
	for k, v := range metric.GetTags().Iterator() {
		metricEvent.Tags[k] = util.ZeroCopyStringToBytes(v)
	}
	return &metricEvent, nil
}

func CreateSpanEventByRawSpanV2(span *models.Span) (*protocol.SpanEvent, error) {
	var spanEvent protocol.SpanEvent
	ts := span.GetTimestamp() / 1e9
	tns := span.GetTimestamp() % 1e9
	spanEvent.Timestamp = (ts << 32) | tns
	spanEvent.TraceID = span.GetTraceID()
	spanEvent.SpanID = span.GetSpanID()
	spanEvent.TraceState = span.GetTraceState()
	spanEvent.ParentSpanID = span.GetParentSpanID()
	spanEvent.Name = span.GetName()
	spanEvent.Kind = protocol.SpanEvent_SpanKind(span.GetKind())
	spanEvent.StartTimeNs = span.GetStartTime()
	spanEvent.EndTimeNs = span.GetEndTime()
	spanEvent.Tags = make(map[string][]byte, span.GetTags().Len())
	for k, v := range span.GetTags().Iterator() {
		spanEvent.Tags[k] = util.ZeroCopyStringToBytes(v)
	}
	spanEvent.Events = make([]*protocol.SpanEvent_InnerEvent, 0, len(span.GetEvents()))
	for _, srcEvent := range span.GetEvents() {
		dstEvent := protocol.SpanEvent_InnerEvent{
			TimestampNs: uint64(srcEvent.Timestamp),
			Name:        srcEvent.Name,
			Tags:        make(map[string][]byte, srcEvent.Tags.Len()),
		}
		for k, v := range srcEvent.Tags.Iterator() {
			dstEvent.Tags[k] = util.ZeroCopyStringToBytes(v)
		}
		spanEvent.Events = append(spanEvent.Events, &dstEvent)
	}
	spanEvent.Links = make([]*protocol.SpanEvent_SpanLink, 0, len(span.GetLinks()))
	for _, srcLink := range span.GetLinks() {
		dstLink := protocol.SpanEvent_SpanLink{
			TraceID:    srcLink.TraceID,
			SpanID:     srcLink.SpanID,
			TraceState: srcLink.TraceState,
			Tags:       make(map[string][]byte, srcLink.Tags.Len()),
		}
		for k, v := range srcLink.Tags.Iterator() {
			dstLink.Tags[k] = util.ZeroCopyStringToBytes(v)
		}
		spanEvent.Links = append(spanEvent.Links, &dstLink)
	}
	spanEvent.Status = protocol.SpanEvent_StatusCode(span.GetStatus())
	return &spanEvent, nil
}

func CreatePipelineEventGroupV1(logEvents []*protocol.LogEvent, configTag map[string]string, logTags map[string]string, ctx map[string]interface{}) (*protocol.PipelineEventGroup, error) {
	var pipelineEventGroup protocol.PipelineEventGroup
	pipelineEventGroup.PipelineEvents = &protocol.PipelineEventGroup_Logs{Logs: &protocol.PipelineEventGroup_LogEvents{Array: logEvents}}
	pipelineEventGroup.Tags = make(map[string][]byte, len(configTag)+len(logTags))
	for k, v := range configTag {
		pipelineEventGroup.Tags[k] = util.ZeroCopyStringToBytes(v)
	}
	for k, v := range logTags {
		pipelineEventGroup.Tags[k] = util.ZeroCopyStringToBytes(v)
	}
	if ctx != nil {
		if source, ok := ctx["source"].(string); ok {
			pipelineEventGroup.Metadata = make(map[string][]byte)
			pipelineEventGroup.Metadata["source_id"] = util.ZeroCopyStringToBytes(source)
		}
	}
	return &pipelineEventGroup, nil
}

func CreatePipelineEventGroupV2(groupInfo *models.GroupInfo, events []models.PipelineEvent) (*protocol.PipelineEventGroup, error) {
	var pipelineEventGroup protocol.PipelineEventGroup
	if len(events) == 0 {
		return nil, fmt.Errorf("events is empty")
	}

	eventType := events[0].GetType()
	switch eventType {
	case models.EventTypeLogging:
		logEvents := make([]*protocol.LogEvent, 0, len(events))
		for _, event := range events {
			if logSrc, ok := event.(*models.Log); ok {
				logDst, _ := CreateLogEventByRawLogV2(logSrc)
				logEvents = append(logEvents, logDst)
			}
		}
		pipelineEventGroup.PipelineEvents = &protocol.PipelineEventGroup_Logs{Logs: &protocol.PipelineEventGroup_LogEvents{Array: logEvents}}
	case models.EventTypeMetric:
		metricEvents := make([]*protocol.MetricEvent, 0, len(events))
		for _, event := range events {
			if metricSrc, ok := event.(*models.Metric); ok {
				metricDst, _ := CreateMetricEventByRawMetricV2(metricSrc)
				metricEvents = append(metricEvents, metricDst)
			}
		}
		pipelineEventGroup.PipelineEvents = &protocol.PipelineEventGroup_Metrics{Metrics: &protocol.PipelineEventGroup_MetricEvents{Array: metricEvents}}
	case models.EventTypeSpan:
		spanEvents := make([]*protocol.SpanEvent, 0, len(events))
		for _, event := range events {
			if spanSrc, ok := event.(*models.Span); ok {
				spanDst, _ := CreateSpanEventByRawSpanV2(spanSrc)
				spanEvents = append(spanEvents, spanDst)
			}
		}
		pipelineEventGroup.PipelineEvents = &protocol.PipelineEventGroup_Spans{Spans: &protocol.PipelineEventGroup_SpanEvents{Array: spanEvents}}
	}

	pipelineEventGroup.Tags = make(map[string][]byte, groupInfo.Tags.Len())
	for k, v := range groupInfo.Tags.Iterator() {
		pipelineEventGroup.Tags[k] = util.ZeroCopyStringToBytes(v)
	}
	pipelineEventGroup.Metadata = make(map[string][]byte, groupInfo.Metadata.Len())
	for k, v := range groupInfo.Metadata.Iterator() {
		pipelineEventGroup.Metadata[k] = util.ZeroCopyStringToBytes(v)
	}
	return &pipelineEventGroup, nil
}
