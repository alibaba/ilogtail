package pipeline

import "github.com/alibaba/ilogtail/pkg/protocol"

type SelfMetricType int

const (
	_ SelfMetricType = iota
	CounterType
	AverageType
	LatencyType
	StringType
	GaugeType

	/*
		Following Type are not used and not implemented yet.
	*/
	RateType
	SummaryType
	HistogramType
)

type Label struct {
	Name  string
	Value string
}

type MetricValue[T string | float64] struct {
	Name  string
	Value T
}

// MetricSet is a Collector to bundle metrics of the same name that differ in  their label values.
// A MetricSet has three properties:
// 1. Metric Name.
// 2. Constant Labels: for metrics that will always be present in the same value.
// 3. Label Names: for metrics that may have different label values, but will always have these labels present.
type MetricSet interface {
	Name() string
	ConstLabels() []Label
	LabelNames() []string
}

// MetricVector is a Collector to bundle metrics of the same name that differ in  their label values.
// WithLabels will return a unique Metric that is bound to a set of label values.
type MetricVector interface {
	WithLabels([]Label) Metric
}

// MetricCollector is the interface implemented by anything that can be used by iLogtail to collect metrics.
type MetricCollector interface {
	Collect() []Metric
}

type Metric interface {
	Serialize(log *protocol.Log)
}

type Counter interface {
	Metric
	Add(float64) error
	Get() MetricValue[float64]
	Clear()
}

type Average interface {
	Counter
}

type Gauge interface {
	Metric
	Set(float64) error
	Get() MetricValue[float64]
	Clear()
}

type Latency interface {
	Metric
	Observe(float64) error
	Get() MetricValue[float64]
}

type Summary interface {
	Metric
	Observe(float64) error
	Get() []MetricValue[float64]
}

type StrMetric interface {
	Metric
	Set(v string) error
	Get() MetricValue[string]
}
