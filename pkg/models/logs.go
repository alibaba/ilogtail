package models

import "github.com/alibaba/ilogtail/pkg/util"

type Log struct {
	Name              string
	Level             string
	SpanID            string
	TraceID           string
	Tags              Tags
	Timestamp         uint64
	ObservedTimestamp uint64
	Body              []byte
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
	return noopStringValues
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

func (m *Log) GetBody() []byte {
	if m != nil {
		return m.Body
	}
	return nil
}

func (m *Log) SetBody(body []byte) {
	if m != nil {
		m.Body = body
	}
}

func (m *Log) GetStringBody() string {
	if m != nil && m.Body != nil {
		return util.ZeroCopyBytesToString(m.Body)
	}
	return ""
}

func (m *Log) SetStringBody(body string) {
	if m != nil {
		m.Body = util.ZeroCopyStringToBytes(body)
	}
}
