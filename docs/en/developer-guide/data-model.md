# Data model

iLogtail currently supports two data models, 'SLS Log Protocol' and 'Pipeline Event'. The descriptions and comparisons of the two models are as follows:
|  | SLS Log Protocol | Pipeline Event |
|  ----  | ----  |  ---- |
| Description | Dedicated processing structure for SLS logs | Scalable observable data model that supports Metrics, Trace, Logging, Bytes, and Profile. |
| Pipeline configuration version | v1 | v2 |
| Logging data structure | Supported | Supported |
| Metric data structure | Not supported | Supported |
| Trace data structure | Not supported | Supported |
| ByteArray data structure | Not supported | Supported |
| Profile data structure | Not supported | Supported |
| Custom data structure | Not supported | Supported |

## SLS Log Protocol

参考 [SLS 协议](log-protocol/protocol-spec/sls.md)

## Pipeline Event

'PipelineEvent' is an abstract data interface in the data processing pipeline. The definition is as follows:

```go
type PipelineEvent interface {
 GetName() string

 SetName(string)

 GetTags() Tags

 GetType() EventType

 GetTimestamp() uint64

 GetObservedTimestamp() uint64

 SetObservedTimestamp(uint64)

 Clone() PipelineEvent
}
```

We also need to introduce the concept of aggregation 'pipelinegroupeve' to support some operations that require grouping data. Aggregation events also allow independent meta and tags, which are usually extracted from data.

```go
type PipelineGroupEvents struct {
 Group  *GroupInfo
 Events []PipelineEvent
}

type GroupInfo struct {
 Metadata Metadata
 Tags     Tags
}
```

### Metric model

The Metric model is defined as follows:

```go
type MetricEvent struct {
        Name              string
        MetricType        MetricType
        Timestamp         uint64
        ObservedTimestamp uint64
        Unit              string
        Description       string
        Tags              Tags
        Value             MetricValue
        TypedValue        MetricTypedValues
}
```

The mainstream metrics data include single-value (eg. Prometheus) and multi-value (eg. influxdb) designs, and two different designs need to be supported in iLogtail. Based on this, two different implementations of MetricValue interface, MetricSingleValue and MetricMultiValue are designed.

```go
type MetricValue interface {
        IsSingleValue() bool

        IsMultiValues() bool

        GetSingleValue() float64

        GetMultiValues() MetricFloatValues
}
type MetricSingleValue struct {
        Value float64
}

type MetricMultiValue struct {
        Values MetricFloatValues
}
```

Typically, metrics of the counter and gauge types use a single value. Histogram and Summary types may use a single value or multiple values in different TSDB. For single value and multiple values, you can use the following API

```go
if metric.Value.IsSingleValue() {
    value := metric.Value.GetSingleValue()
} 

if metric.Value.IsMultiValues() {
  values := metric.Value.GetMultiValues()
  for field, value := range values.Iterator() {
    // The field type is string, which indicates a multi-value field name.
    // The value is float64.
  }
}
```

TypedValue design. MetricValue value type is float64. in some TSDB, such as influxdb, non-numeric types such as string bool are supported in addition to numeric types [influxdb field-value](https:// docs)....). To do this, we extended an additional TypedValue field to store non-numeric field values.

```go
type TypedValue struct {
        Type  ValueType
        Value interface{}
}

type MetricTypedValues interface {
        KeyValues[*TypedValue]
}
```

### Trace model

Currently, the mainstream open source tracing models have several different specifications, such as opentracing opentelemetry skywalking.
No matter in which trace model, trace is composed of multiple spans (the difference is that in skywalking and many private protocols, there is a concept of segment or transaction between trace and Span).
In Span, Tags that carry data semantics are generally defined, and logs or events that record the occurrence of some key events are also recorded.
zipkin jaeger is an implementation of opentracing, and opentelemetry can be considered as a new specification and superset extended on the basis of opentracing (compatible with w3c and clearly defined OTLP protocol).
Therefore, the Span structure defined by us must be compatible with several open source protocols mentioned above. The specific structure is as follows:

```go
type SpanLink struct {
    TraceID    string
    SpanID     string
    TraceState string
    Tags       Tags
}

type SpanEvent struct {
    Timestamp int64
    Name      string
}

type Span struct {
    TraceID      string
    SpanID       string
    ParentSpanID string
    Name         string
    TraceState   string

    StartTime         uint64
    EndTime           uint64
    ObservedTimestamp uint64

    Kind   SpanKind
    Status StatusCode
    Tags   Tags
    Links  []*SpanLink
    Events []*SpanEvent
}
```

### Log model

The Log model is compatible with unstructured and structured logs and reserves fields for link information records.

-The Offset record the Offset of the log in the file when the log file is collected. Optional.
-Name is also optional for Log
-SpanID and TraceID are used for data Association. Optional.
-Contents is used in log structured scenarios to store KV logs analyzed from the Body's original log text.

```go
type LogContents KeyValues[string]

type Log struct {
    Name              string
    Level             string
    SpanID            string
    TraceID           string
    Tags              Tags
    Timestamp         uint64
    ObservedTimestamp uint64
    Offset            uint64
    Contents          LogContents
}
```

The Level field, which is aligned Open Telemetry Logs, supports the following levels:
`Trace`, `Trace2`, `Trace3`, `Trace4`,
`Debug`, `Debug2`, `Debug3`, `Debug4`,
`Info`, `Info2`, `Info3`, `Info4`,
`Warn`, `Warn2`, `Warn3`, `Warn4`,
`Error`, `Error2`, `Error3`, `Error4`,
`Fatal`, `Fatal2`, `Fatal3`, `Fatal4`,
`Unspecified`.
