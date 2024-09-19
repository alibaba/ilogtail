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
	if len(logEvent.Contents) < len(fields) {
		slice := make([]*protocol.LogEvent_Content, len(logEvent.Contents), len(fields))
		copy(slice, logEvent.Contents)
		logEvent.Contents = slice
	} else {
		logEvent.Contents = logEvent.Contents[:len(fields)]
	}
	i := 0
	for key, val := range fields {
		if i >= len(logEvent.Contents) {
			logEvent.Contents = append(logEvent.Contents, &protocol.LogEvent_Content{})
		}
		logEvent.Contents[i].Key = key
		logEvent.Contents[i].Value = util.String2Bytes(val)
		i++
	}
	logEvent.Time = uint32(t.Unix())
	if enableTimestampNano {
		logEvent.TimeNs = uint32(t.Nanosecond())
	}
	return logEvent, nil
}

func CreateLogEventByArray(t time.Time, enableTimestampNano bool, columns []string, values []string) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(columns))
	if len(columns) != len(values) {
		return nil, fmt.Errorf("columns and values not equal")
	}
	for index := range columns {
		cont := &protocol.LogEvent_Content{
			Key:   columns[index],
			Value: util.String2Bytes(values[index]),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}

	logEvent.Time = uint32(t.Unix())
	if enableTimestampNano {
		logEvent.TimeNs = uint32(t.Nanosecond())
	}
	return logEvent, nil
}

func CreateLogEventByRawLogV1(log *protocol.Log) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, len(log.Contents))
	for _, logC := range log.Contents {
		cont := &protocol.LogEvent_Content{
			Key:   logC.Key,
			Value: util.String2Bytes(logC.Value),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Time = log.Time
	logEvent.TimeNs = *log.TimeNs
	return logEvent, nil
}

func CreateLogEventByRawLogV2(log *models.Log) (*protocol.LogEvent, error) {
	logEvent := LogEventPool.Get().(*protocol.LogEvent)
	logEvent.Contents = make([]*protocol.LogEvent_Content, 0, log.Contents.Len())
	for k, v := range log.Contents.Iterator() {
		cont := &protocol.LogEvent_Content{
			Key:   k,
			Value: util.String2Bytes(v.(string)),
		}
		logEvent.Contents = append(logEvent.Contents, cont)
	}
	logEvent.Time = uint32(log.GetTimestamp())
	return logEvent, nil
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

func CreatePipelineEventGroupV1(logEvents []*protocol.LogEvent, configTag map[string]string, logTags map[string]string, ctx map[string]interface{}) (*protocol.PipelineEventGroup, error) {
	var pipelineEventGroup protocol.PipelineEventGroup
	pipelineEventGroup.Type = protocol.PipelineEventGroup_LOG
	pipelineEventGroup.Logs = logEvents
	pipelineEventGroup.Tags = make(map[string][]byte, len(configTag)+len(logTags))
	for k, v := range configTag {
		pipelineEventGroup.Tags[k] = util.String2Bytes(v)
	}
	for k, v := range logTags {
		pipelineEventGroup.Tags[k] = util.String2Bytes(v)
	}
	if ctx != nil {
		if source, ok := ctx["source"].(string); ok {
			pipelineEventGroup.Metadata = make(map[string][]byte)
			pipelineEventGroup.Metadata["source"] = util.String2Bytes(source)
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
		pipelineEventGroup.Tags[k] = util.String2Bytes(v)
	}
	pipelineEventGroup.Metadata = make(map[string][]byte, groupInfo.Metadata.Len())
	for k, v := range groupInfo.Metadata.Iterator() {
		pipelineEventGroup.Metadata[k] = util.String2Bytes(v)
	}
	return &pipelineEventGroup, nil
}
