package helper

import (
	"fmt"
	"time"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol"
)

func CreateLogEvent(t time.Time, enableTimestampNano bool, fields map[string]string) (*protocol.LogEvent, error) {
	var logEvent protocol.LogEvent
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(fields))
	for key, val := range fields {
		cont := &protocol.LogEvent_Content{
			Key:   key,
			Value: []byte(val),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Time = uint32(t.Unix())
	if enableTimestampNano {
		logEvent.TimeNs = uint32(t.Nanosecond())
	}
	return &logEvent, nil
}

func CreateLogEventByArray(t time.Time, enableTimestampNano bool, columns []string, values []string) (*protocol.LogEvent, error) {
	var logEvent protocol.LogEvent
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(columns))
	if len(columns) != len(values) {
		return nil, fmt.Errorf("columns and values not equal")
	}
	for index := range columns {
		cont := &protocol.LogEvent_Content{
			Key:   columns[index],
			Value: []byte(values[index]),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}

	logEvent.Time = uint32(t.Unix())
	if enableTimestampNano {
		logEvent.TimeNs = uint32(t.Nanosecond())
	}
	return &logEvent, nil
}

func CreateLogEventByRawLogV1(log *protocol.Log) (*protocol.LogEvent, error) {
	var logEvent protocol.LogEvent
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(log.Contents))
	for _, logC := range log.Contents {
		cont := &protocol.LogEvent_Content{
			Key:   logC.Key,
			Value: []byte(logC.Value),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Time = log.Time
	logEvent.TimeNs = *log.TimeNs
	return &logEvent, nil
}

func CreateLogEventByRawLogV2(log *models.Log) (*protocol.LogEvent, error) {
	var logEvent protocol.LogEvent
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, log.Contents.Len())
	for k, v := range log.Contents.Iterator() {
		cont := &protocol.LogEvent_Content{
			Key:   k,
			Value: []byte(v.(string)),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Time = uint32(log.GetTimestamp())
	return &logEvent, nil
}

func CreateMetricEventByRawMetricV2(metric *models.Metric) (*protocol.MetricEvent, error) {
	var metricEvent protocol.MetricEvent
	metricEvent.Name = metric.Name
	if metric.GetValue().IsSingleValue() {
		metricEvent.Type = protocol.MetricEvent_SINGLE
		metricEvent.SingleValue = metric.GetValue().GetSingleValue()
	} else {
		metricEvent.Type = protocol.MetricEvent_MULTI
		metricEvent.MultiValue = make(map[string]float64, metric.GetValue().GetMultiValues().Len())
		for k, v := range metric.GetValue().GetMultiValues().Iterator() {
			metricEvent.MultiValue[k] = v
		}
	}
	metricEvent.Tags = make(map[string]string, metric.GetTags().Len())
	for k, v := range metric.GetTags().Iterator() {
		metricEvent.Tags[k] = v
	}
	metricEvent.Time = uint32(metric.GetTimestamp())
	return &metricEvent, nil
}

func CreatePipelineEventGroupV1(logEvents []*protocol.LogEvent, ctx map[string]interface{}) (*protocol.PipelineEventGroup, error) {
	var pipelineEventGroup protocol.PipelineEventGroup
	pipelineEventGroup.Type = protocol.PipelineEventGroup_LOG
	pipelineEventGroup.Logs = logEvents
	if tags, ok := ctx["tags"].(map[string]string); ok {
		pipelineEventGroup.Tags = make(map[string][]byte, len(tags))
		for k, v := range tags {
			pipelineEventGroup.Tags[k] = []byte(v)
		}
	}
	if source, ok := ctx["source"].(string); ok {
		pipelineEventGroup.Metadata = make(map[string][]byte)
		pipelineEventGroup.Metadata["source"] = []byte(source)
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
		pipelineEventGroup.Type = protocol.PipelineEventGroup_LOG
		pipelineEventGroup.Logs = make([]*protocol.LogEvent, 0, len(events))
	case models.EventTypeMetric:
		pipelineEventGroup.Type = protocol.PipelineEventGroup_METRIC
		pipelineEventGroup.Metrics = make([]*protocol.MetricEvent, 0, len(events))
	case models.EventTypeSpan:
		pipelineEventGroup.Type = protocol.PipelineEventGroup_SPAN
		pipelineEventGroup.Spans = make([]*protocol.SpanEvent, 0, len(events))
	}

	for _, event := range events {
		if event.GetType() != eventType {
			return nil, fmt.Errorf("event type is not same")
		}
		switch eventType {
		case models.EventTypeLogging:
			logSrc, ok := event.(*models.Log)
			if ok {
				logDst, _ := CreateLogEventByRawLogV2(logSrc)
				pipelineEventGroup.Logs = append(pipelineEventGroup.Logs, logDst)
			}
		case models.EventTypeMetric:
			metricSrc, ok := event.(*models.Metric)
			if ok {
				metricDst, _ := CreateMetricEventByRawMetricV2(metricSrc)
				pipelineEventGroup.Metrics = append(pipelineEventGroup.Metrics, metricDst)
			}
		case models.EventTypeSpan:
		}
	}

	pipelineEventGroup.Tags = make(map[string][]byte, groupInfo.Tags.Len())
	for k, v := range groupInfo.Tags.Iterator() {
		pipelineEventGroup.Tags[k] = []byte(v)
	}
	pipelineEventGroup.Metadata = make(map[string][]byte, groupInfo.Metadata.Len())
	for k, v := range groupInfo.Metadata.Iterator() {
		pipelineEventGroup.Metadata[k] = []byte(v)
	}
	return &pipelineEventGroup, nil
}
