package opentelemetry

import (
	"math"
	"sort"
	"strconv"
	"strings"
	"unicode"

	"github.com/alibaba/ilogtail/pkg/protocol"
	"github.com/alibaba/ilogtail/pkg/protocol/otlp"
	"go.opentelemetry.io/collector/pdata/pcommon"
	"go.opentelemetry.io/collector/pdata/pmetric"
)

const (
	metricNameKey      = "__name__"
	labelsKey          = "__labels__"
	timeNanoKey        = "__time_nano__"
	valueKey           = "__value__"
	infinityBoundValue = "+Inf"
	bucketLabelKey     = "le"
	summaryLabelKey    = "quantile"
)

type KeyValue struct {
	Key   string
	Value string
}

type KeyValues struct {
	keyValues []KeyValue
}

func (kv *KeyValues) Len() int { return len(kv.keyValues) }
func (kv *KeyValues) Swap(i, j int) {
	kv.keyValues[i], kv.keyValues[j] = kv.keyValues[j], kv.keyValues[i]
}
func (kv *KeyValues) Less(i, j int) bool { return kv.keyValues[i].Key < kv.keyValues[j].Key }

func (kv *KeyValues) Sort() {
	sort.Sort(kv)
}

func (kv *KeyValues) Replace(key, value string) {
	key = sanitize(key)
	findIndex := sort.Search(len(kv.keyValues), func(index int) bool {
		return kv.keyValues[index].Key >= key
	})
	if findIndex < len(kv.keyValues) && kv.keyValues[findIndex].Key == key {
		kv.keyValues[findIndex].Value = value
	}
}

func (kv *KeyValues) Append(key, value string) {
	key = sanitize(key)
	kv.keyValues = append(kv.keyValues, KeyValue{
		key,
		value,
	})
}

func (kv *KeyValues) Clone() KeyValues {
	var newKeyValues KeyValues
	newKeyValues.keyValues = make([]KeyValue, len(kv.keyValues))
	copy(newKeyValues.keyValues, kv.keyValues)
	return newKeyValues
}

func (kv *KeyValues) String() string {
	var builder strings.Builder
	kv.labelToStringBuilder(&builder)
	return builder.String()
}

func (kv *KeyValues) labelToStringBuilder(sb *strings.Builder) {
	for index, label := range kv.keyValues {
		sb.WriteString(label.Key)
		sb.WriteString("#$#")
		sb.WriteString(label.Value)
		if index != len(kv.keyValues)-1 {
			sb.WriteByte('|')
		}
	}
}

func min(l, r int) int {
	if l < r {
		return l
	}
	return r
}

func formatMetricName(name string) string {
	var newName []byte
	for i := 0; i < len(name); i++ {
		b := name[i]
		if (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9') || b == '_' || b == ':' {
			continue
		} else {
			if newName == nil {
				newName = []byte(name)
			}
			newName[i] = '_'
		}
	}
	if newName == nil {
		return name
	}
	return string(newName)
}

func newMetricLogFromRaw(name string, labels KeyValues, nsec int64, value float64) *protocol.Log {
	labels.Sort()
	return &protocol.Log{
		Time: uint32(nsec / 1e9),
		Contents: []*protocol.Log_Content{
			{
				Key:   metricNameKey,
				Value: formatMetricName(name),
			},
			{
				Key:   labelsKey,
				Value: labels.String(),
			},
			{
				Key:   timeNanoKey,
				Value: strconv.FormatInt(nsec, 10),
			},
			{
				Key:   valueKey,
				Value: strconv.FormatFloat(value, 'g', -1, 64),
			},
		},
	}
}

func sanitize(s string) string {
	if len(s) == 0 {
		return s
	}

	s = strings.Map(sanitizeRune, s)
	if unicode.IsDigit(rune(s[0])) {
		s = "key_" + s
	}

	if s[0] == '_' {
		s = "key" + s
	}

	return s
}

func sanitizeRune(r rune) rune {
	if unicode.IsLetter(r) || unicode.IsDigit(r) {
		return r
	}

	return '_'
}

func res2Labels(labels *KeyValues, resource pcommon.Resource) {
	attrs := resource.Attributes()
	attrs.Range(func(k string, v pcommon.Value) bool {
		labels.Append(k, v.AsString())
		return true
	})
}

func GaugeToLogs(name string, data pmetric.NumberDataPointSlice, defaultLabels KeyValues) (logs []*protocol.Log) {
	for i := 0; i < data.Len(); i++ {
		dataPoint := data.At(i)
		attributeMap := dataPoint.Attributes()
		labels := defaultLabels.Clone()
		attributeMap.Range(func(k string, v pcommon.Value) bool {
			labels.Append(k, v.AsString())
			return true
		})

		value := dataPoint.DoubleValue()
		if dataPoint.IntValue() != 0 {
			value = float64(dataPoint.IntValue())
		}
		logs = append(logs, newMetricLogFromRaw(name, labels, int64(dataPoint.Timestamp()), value))
	}
	return logs
}

func SumToLogs(name string, aggregationTemporality pmetric.AggregationTemporality, isMonotonic string, data pmetric.NumberDataPointSlice, defaultLabels KeyValues) (logs []*protocol.Log) {
	for i := 0; i < data.Len(); i++ {
		dataPoint := data.At(i)
		attributeMap := dataPoint.Attributes()
		labels := defaultLabels.Clone()
		attributeMap.Range(func(k string, v pcommon.Value) bool {
			labels.Append(k, v.AsString())
			return true
		})
		labels.Append(otlp.TagKeyMetricIsMonotonic, isMonotonic)
		labels.Append(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())

		value := dataPoint.DoubleValue()
		if dataPoint.IntValue() != 0 {
			value = float64(dataPoint.IntValue())
		}
		logs = append(logs, newMetricLogFromRaw(name, labels, int64(dataPoint.Timestamp()), value))
	}
	return logs
}

func SummaryToLogs(name string, data pmetric.SummaryDataPointSlice, defaultLabels KeyValues) (logs []*protocol.Log) {
	for i := 0; i < data.Len(); i++ {
		dataPoint := data.At(i)
		attributeMap := dataPoint.Attributes()
		labels := defaultLabels.Clone()
		attributeMap.Range(func(k string, v pcommon.Value) bool {
			labels.Append(k, v.AsString())
			return true
		})

		logs = append(logs, newMetricLogFromRaw(name+"_sum", labels, int64(dataPoint.Timestamp()), dataPoint.Sum()))
		logs = append(logs, newMetricLogFromRaw(name+"_count", labels, int64(dataPoint.Timestamp()), float64(dataPoint.Count())))

		summaryLabels := labels.Clone()
		summaryLabels.Append(summaryLabelKey, "")
		summaryLabels.Sort()

		values := dataPoint.QuantileValues()
		for j := 0; j < values.Len(); j++ {
			value := values.At(j)
			summaryLabels.Replace(summaryLabelKey, strconv.FormatFloat(value.Quantile(), 'g', -1, 64))
			logs = append(logs, newMetricLogFromRaw(name, summaryLabels, int64(dataPoint.Timestamp()), value.Value()))
		}
	}
	return logs
}

func HistogramToLogs(name string, data pmetric.HistogramDataPointSlice, aggregationTemporality pmetric.AggregationTemporality, defaultLabels KeyValues) (logs []*protocol.Log) {
	for i := 0; i < data.Len(); i++ {
		dataPoint := data.At(i)
		attributeMap := dataPoint.Attributes()
		labels := defaultLabels.Clone()
		attributeMap.Range(func(k string, v pcommon.Value) bool {
			labels.Append(k, v.AsString())
			return true
		})
		labels.Append(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())
		labels.Append(otlp.TagKeyMetricHistogramType, pmetric.MetricTypeHistogram.String())

		if dataPoint.HasSum() {
			logs = append(logs, newMetricLogFromRaw(name+"_sum", labels, int64(dataPoint.Timestamp()), dataPoint.Sum()))
		}
		if dataPoint.HasMin() {
			logs = append(logs, newMetricLogFromRaw(name+"_min", labels, int64(dataPoint.Timestamp()), dataPoint.Min()))
		}
		if dataPoint.HasMax() {
			logs = append(logs, newMetricLogFromRaw(name+"_max", labels, int64(dataPoint.Timestamp()), dataPoint.Max()))
		}
		logs = append(logs, newMetricLogFromRaw(name+"_count", labels, int64(dataPoint.Timestamp()), float64(dataPoint.Count())))

		bounds := dataPoint.ExplicitBounds()
		boundsStr := make([]string, bounds.Len()+1)
		for j := 0; j < bounds.Len(); j++ {
			boundsStr[j] = strconv.FormatFloat(bounds.At(j), 'g', -1, 64)
		}
		boundsStr[len(boundsStr)-1] = infinityBoundValue

		bucketCount := min(len(boundsStr), dataPoint.BucketCounts().Len())

		bucketLabels := labels.Clone()
		bucketLabels.Append(bucketLabelKey, "")
		bucketLabels.Sort()
		for j := 0; j < bucketCount; j++ {
			bucket := dataPoint.BucketCounts().At(j)
			bucketLabels.Replace(bucketLabelKey, boundsStr[j])

			logs = append(logs, newMetricLogFromRaw(name+"_bucket", bucketLabels, int64(dataPoint.Timestamp()), float64(bucket)))
		}
	}
	return logs
}

func ExponentialHistogramToLogs(name string, data pmetric.ExponentialHistogramDataPointSlice, aggregationTemporality pmetric.AggregationTemporality, defaultLabels KeyValues) (logs []*protocol.Log) {
	for i := 0; i < data.Len(); i++ {
		dataPoint := data.At(i)
		attributeMap := dataPoint.Attributes()
		labels := defaultLabels.Clone()
		attributeMap.Range(func(k string, v pcommon.Value) bool {
			labels.Append(k, v.AsString())
			return true
		})
		labels.Append(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())
		labels.Append(otlp.TagKeyMetricHistogramType, pmetric.MetricTypeExponentialHistogram.String())

		if dataPoint.HasSum() {
			logs = append(logs, newMetricLogFromRaw(name+"_sum", labels, int64(dataPoint.Timestamp()), dataPoint.Sum()))
		}
		if dataPoint.HasMin() {
			logs = append(logs, newMetricLogFromRaw(name+"_min", labels, int64(dataPoint.Timestamp()), dataPoint.Min()))
		}
		if dataPoint.HasMax() {
			logs = append(logs, newMetricLogFromRaw(name+"_max", labels, int64(dataPoint.Timestamp()), dataPoint.Max()))
		}
		logs = append(logs, newMetricLogFromRaw(name+"_count", labels, int64(dataPoint.Timestamp()), float64(dataPoint.Count())))

		scale := dataPoint.Scale()
		base := math.Pow(2, math.Pow(2, float64(-scale)))
		postiveFields := genExponentialHistogramValues(true, base, dataPoint.Positive())
		negativeFields := genExponentialHistogramValues(false, base, dataPoint.Negative())

		bucketLabels := labels.Clone()
		bucketLabels.Append(bucketLabelKey, "")
		bucketLabels.Sort()
		for k, v := range postiveFields {
			bucketLabels.Replace(bucketLabelKey, k)
			logs = append(logs, newMetricLogFromRaw(name+"_bucket", bucketLabels, int64(dataPoint.Timestamp()), v))
		}
		bucketLabels.Replace(bucketLabelKey, otlp.FieldZeroCount)
		logs = append(logs, newMetricLogFromRaw(name+"_bucket", bucketLabels, int64(dataPoint.Timestamp()), float64(dataPoint.ZeroCount())))
		for k, v := range negativeFields {
			bucketLabels.Replace(bucketLabelKey, k)
			logs = append(logs, newMetricLogFromRaw(name+"_bucket", bucketLabels, int64(dataPoint.Timestamp()), v))
		}
	}
	return logs
}
