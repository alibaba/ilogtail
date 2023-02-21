package models

import "github.com/alibaba/ilogtail/pkg/util"

type Indices KeyValues[string]

type Log struct {
	Name              string
	Level             string
	SpanID            string
	TraceID           string
	Tags              Tags
	Timestamp         uint64
	ObservedTimestamp uint64
	Offset            uint64
	Body              string
	Indices           Indices
}

func (m *Log) GetName() string {
	if m != nil {
		return m.Name
	}
	return ""
}

func (m *Log) SetName(name string) {
	if m != nil {
		m.Name = name
	}
}

func (m *Log) GetTags() Tags {
	if m != nil {
		return m.Tags
	}
	return NilStringValues
}

func (m *Log) GetType() EventType {
	return EventTypeLogging
}

func (m *Log) GetTimestamp() uint64 {
	if m != nil {
		return m.Timestamp
	}
	return 0
}

func (m *Log) GetObservedTimestamp() uint64 {
	if m != nil {
		return m.ObservedTimestamp
	}
	return 0
}

func (m *Log) SetObservedTimestamp(observedTimestamp uint64) {
	if m != nil {
		m.ObservedTimestamp = observedTimestamp
	}
}

func (m *Log) GetOffset() uint64 {
	if m != nil {
		return m.Offset
	}
	return 0
}

func (m *Log) SetOffset(offset uint64) {
	if m != nil {
		m.Offset = offset
	}
}

func (m *Log) GetLevel() string {
	if m != nil {
		return m.Level
	}
	return ""
}

func (m *Log) SetLevel(level string) {
	if m != nil {
		m.Level = level
	}
}

func (m *Log) GetSpanID() string {
	if m != nil {
		return m.SpanID
	}
	return ""
}

func (m *Log) SetSpanID(spanID string) {
	if m != nil {
		m.SpanID = spanID
	}
}

func (m *Log) GetTraceID() string {
	if m != nil {
		return m.TraceID
	}
	return ""
}

func (m *Log) SetTraceID(traceID string) {
	if m != nil {
		m.TraceID = traceID
	}
}

func (m *Log) GetBody() string {
	if m != nil {
		return m.Body
	}
	return ""
}

func (m *Log) SetBody(body string) {
	if m != nil {
		m.Body = body
	}
}

func (m *Log) GetBytesBody() []byte {
	return util.ZeroCopyStringToBytes(m.GetBody())
}

func (m *Log) SetBytesBody(body []byte) {
	m.SetBody(util.ZeroCopyBytesToString(body))
}

func (m *Log) SetIndices(indices Indices) {
	if m != nil {
		m.Indices = indices
	}
}

func (m *Log) GetIndices() Indices {
	if m != nil {
		return m.Indices
	}
	return NilStringValues
}

func (m *Log) Clone() PipelineEvent {
	if m != nil {
		return &Log{
			Name:              m.Name,
			Level:             m.Level,
			SpanID:            m.SpanID,
			TraceID:           m.TraceID,
			Tags:              m.Tags,
			Timestamp:         m.Timestamp,
			ObservedTimestamp: m.ObservedTimestamp,
			Body:              m.Body,
			Indices:           m.Indices,
			Offset:            m.Offset,
		}
	}
	return nil
}
