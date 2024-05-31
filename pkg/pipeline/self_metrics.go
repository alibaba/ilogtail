package pipeline

import (
	"github.com/alibaba/ilogtail/pkg/protocol"
)

type SelfMetricType int

const (
	_ SelfMetricType = iota
	CounterType
	AverageType
	LatencyType
	StringType
	GaugeType
	DeltaType // replace counter

	/*
		Following Type are not used and not implemented yet.
	*/
	RateType
	SummaryType
	HistogramType
)

type Label struct {
	Key   string
	Value string
}

type MetricValue[T string | float64] struct {
	Name  string // Value Name, e.g. "cpu_usage"
	Value T
}

// MetricSet is a Collector to bundle metrics of the same name that differ in  their label values.
// A MetricSet has three properties:
// 1. Metric Name.
// 2. Constant Labels: Labels has constant value, e.g., "hostname=localhost", "namespace=default"
// 3. Label Keys is label Keys that may have different values, e.g., "status_code=200", "status_code=404".
type MetricSet interface {
	Name() string
	Type() SelfMetricType
	ConstLabels() []Label
	LabelKeys() []string
}

// MetricCollector is the interface implemented by anything that can be used by iLogtail to collect metrics.
type MetricCollector interface {
	Collect() []Metric
}

// MetricVector is a Collector to bundle metrics of the same name that differ in  their label values.
// WithLabels will return a unique Metric that is bound to a set of label values.
// If the labels has label names that are not in the MetricSet, the Metric will be invalid.
type MetricVector[T Metric] interface {
	WithLabels(labels ...Label) T
}

type Metric interface {
	Serialize(log *protocol.Log)
	Clear()
}

// Counter has threae implementations:
// counter: a cumulative counter metric that represents a single monotonically increasing counter whose value can only increase or be reset to zero on restart.
// delta: the increased value in the last window.
// average: the cumulative average value.
type Counter interface {
	Metric
	Add(int64) error // return error when WithLabels returns an invalid metric.
	Get() MetricValue[float64]
}

type Gauge interface {
	Metric
	Set(float64) error // return error when WithLabels returns an invalid metric instance.
	Get() MetricValue[float64]
}

type Latency interface {
	Metric
	Observe(float64) error // return error when WithLabels returns an invalid metric instance.
	Get() MetricValue[float64]
}

type Summary interface {
	Metric
	Observe(float64) error // return error when WithLabels returns an invalid metric intance.
	Get() []MetricValue[float64]
}

type StringMetric interface {
	Metric
	Set(v string) error // return error when WithLabels returns an invalid metric instance.
	Get() MetricValue[string]
}
