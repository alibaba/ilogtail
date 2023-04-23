package opentelemetry

import (
	"math"
	"strconv"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/pkg/protocol/otlp"
	"go.opentelemetry.io/collector/pdata/pcommon"
	"go.opentelemetry.io/collector/pdata/pmetric"
	"go.opentelemetry.io/collector/pdata/ptrace"
)

func genScopeTags(scope pcommon.InstrumentationScope) models.Tags {
	scopeTags := attrs2Tags(scope.Attributes())
	scopeTags.Add(otlp.TagKeyScopeName, scope.Name())
	scopeTags.Add(otlp.TagKeyScopeVersion, scope.Version())
	scopeTags.Add(otlp.TagKeyScopeDroppedAttributesCount, strconv.Itoa(int(scope.DroppedAttributesCount())))
	return scopeTags
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
	meta := models.NewMetadata()

	attributes.Range(
		func(k string, v pcommon.Value) bool {
			meta.Add(k, v.AsString())
			return true
		},
	)
	return meta
}

func getValue(intValue int64, doubleValue float64) float64 {
	if intValue != 0 {
		return float64(intValue)
	}

	return doubleValue
}

func newMetricFromGaugeDatapoint(datapoint pmetric.NumberDataPoint, metricName, metricUnit, metricDescription string) *models.Metric {
	timestamp := int64(datapoint.Timestamp())
	startTimestamp := datapoint.StartTimestamp()
	tags := attrs2Tags(datapoint.Attributes())

	value := getValue(datapoint.IntValue(), datapoint.DoubleValue())
	metric := models.NewSingleValueMetric(metricName, models.MetricTypeGauge, tags, timestamp, value)
	metric.Unit = metricUnit
	metric.Description = metricDescription
	metric.SetObservedTimestamp(uint64(startTimestamp))
	return metric
}

func newMetricFromSumDatapoint(datapoint pmetric.NumberDataPoint, aggregationTemporality pmetric.AggregationTemporality, isMonotonic, metricName, metricUnit, metricDescription string) *models.Metric {
	timestamp := int64(datapoint.Timestamp())
	startTimestamp := datapoint.StartTimestamp()
	tags := attrs2Tags(datapoint.Attributes())
	tags.Add(otlp.TagKeyMetricIsMonotonic, isMonotonic)
	tags.Add(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())

	value := getValue(datapoint.IntValue(), datapoint.DoubleValue())
	var metric *models.Metric
	if aggregationTemporality == pmetric.AggregationTemporalityCumulative {
		metric = models.NewSingleValueMetric(metricName, models.MetricTypeCounter, tags, timestamp, value)
	} else {
		metric = models.NewSingleValueMetric(metricName, models.MetricTypeRateCounter, tags, timestamp, value)
	}
	metric.Unit = metricUnit
	metric.Description = metricDescription
	metric.SetObservedTimestamp(uint64(startTimestamp))
	return metric
}

func newMetricFromSummaryDatapoint(datapoint pmetric.SummaryDataPoint, metricName, metricUnit, metricDescription string) *models.Metric {
	timestamp := int64(datapoint.Timestamp())
	startTimestamp := datapoint.StartTimestamp()
	tags := attrs2Tags(datapoint.Attributes())

	multivalue := models.NewMetricMultiValue()
	summaryValues := datapoint.QuantileValues()
	for m := 0; m < summaryValues.Len(); m++ {
		summaryValue := summaryValues.At(m)
		quantile := summaryValue.Quantile()
		value := summaryValue.Value()
		multivalue.Add(strconv.FormatFloat(quantile, 'f', -1, 64), value)
	}

	multivalue.Add(otlp.FieldCount, float64(datapoint.Count()))
	multivalue.Add(otlp.FieldSum, datapoint.Sum())

	metric := models.NewMultiValuesMetric(metricName, models.MetricTypeSummary, tags, timestamp, multivalue.GetMultiValues())
	metric.Unit = metricUnit
	metric.Description = metricDescription
	metric.SetObservedTimestamp(uint64(startTimestamp))
	return metric
}

func newMetricFromHistogramDatapoint(datapoint pmetric.HistogramDataPoint, aggregationTemporality pmetric.AggregationTemporality, metricName, metricUnit, metricDescription string) *models.Metric {
	timestamp := int64(datapoint.Timestamp())
	startTimestamp := datapoint.StartTimestamp()

	tags := attrs2Tags(datapoint.Attributes())
	tags.Add(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())
	tags.Add(otlp.TagKeyMetricHistogramType, pmetric.MetricTypeHistogram.String())

	// TODO:
	// handle datapoint's Exemplars, Flags
	multivalue := models.NewMetricMultiValue()
	multivalue.Add(otlp.FieldCount, float64(datapoint.Count()))

	if datapoint.HasSum() {
		multivalue.Add(otlp.FieldSum, datapoint.Sum())
	}
	if datapoint.HasMin() {
		multivalue.Add(otlp.FieldMin, datapoint.Min())
	}

	if datapoint.HasMax() {
		multivalue.Add(otlp.FieldMax, datapoint.Max())
	}

	bucketCounts, explicitBounds := datapoint.BucketCounts(), datapoint.ExplicitBounds()

	// bucketsCounts can be 0, otherwise, #bucketCounts == #bounds + 1.
	if bucketCounts.Len() == 0 || bucketCounts.Len() != explicitBounds.Len()+1 {
		metric := models.NewMultiValuesMetric(metricName, models.MetricTypeHistogram, tags, timestamp, multivalue.GetMultiValues())
		metric.Unit = metricUnit
		metric.SetObservedTimestamp(uint64(startTimestamp))
		return metric
	}

	// for bucket bounds: [0, 5, 10], there are 4 bucket counts: (-inf, 0], (0, 5], (5, 10], (10, +inf]
	for m := 0; m < bucketCounts.Len(); m++ {
		lowerBound := math.Inf(-1)
		upperBound := math.Inf(+1)
		if m > 0 {
			lowerBound = explicitBounds.At(m - 1)
		}
		if m < explicitBounds.Len() {
			upperBound = explicitBounds.At(m)
		}
		fieldName := otlp.ComposeBucketFieldName(lowerBound, upperBound, true)

		count := bucketCounts.At(m)
		multivalue.Add(fieldName, float64(count))
	}

	metric := models.NewMultiValuesMetric(metricName, models.MetricTypeHistogram, tags, timestamp, multivalue.GetMultiValues())
	metric.Unit = metricUnit
	metric.Description = metricDescription
	metric.SetObservedTimestamp(uint64(startTimestamp))
	return metric
}

// Note that ExponentialHistogram is still in experimental status.
// https://opentelemetry.io/docs/reference/specification/metrics/data-model/#exponentialhistogram
func newMetricFromExponentialHistogramDatapoint(datapoint pmetric.ExponentialHistogramDataPoint, aggregationTemporality pmetric.AggregationTemporality, metricName, metricUnit, metricDescription string) *models.Metric {
	timestamp := int64(datapoint.Timestamp())
	startTimestamp := datapoint.StartTimestamp()

	tags := attrs2Tags(datapoint.Attributes())
	tags.Add(otlp.TagKeyMetricAggregationTemporality, aggregationTemporality.String())
	tags.Add(otlp.TagKeyMetricHistogramType, pmetric.MetricTypeExponentialHistogram.String())

	// TODO:
	// handle datapoint's Exemplars, Flags
	multivalue := models.NewMetricMultiValue()
	multivalue.Add(otlp.FieldCount, float64(datapoint.Count()))

	if datapoint.HasSum() {
		multivalue.Add(otlp.FieldSum, datapoint.Sum())
	}
	if datapoint.HasMin() {
		multivalue.Add(otlp.FieldMin, datapoint.Min())
	}

	if datapoint.HasMax() {
		multivalue.Add(otlp.FieldMax, datapoint.Max())
	}

	scale := datapoint.Scale()
	base := math.Pow(2, math.Pow(2, float64(-scale)))
	multivalue.Values.Add(otlp.FieldScale, float64(scale)) // store scale in multivalues

	// For example, when scale is 3, base is 2 ** (2 ** -3) = 2 ** (1/8).
	// The negative bucket bounds look like:
	// ...., [-2 ** (2/8),  -2 ** (1/8)), [-2 ** (1/8), -2 ** (0/8)),
	// The positive bucket bounds look like:
	// (2 ** (0/8), 2 ** (1/8)], (2 ** (1/8), 2 ** (2/8)], ....
	// Meanwhile, there is also a zero bucket.
	postiveFields := genExponentialHistogramValues(true, base, datapoint.Positive())
	negativeFields := genExponentialHistogramValues(false, base, datapoint.Negative())

	multivalue.Values.AddAll(postiveFields)
	multivalue.Values.AddAll(negativeFields)

	multivalue.Add(otlp.FieldZeroCount, float64(datapoint.ZeroCount()))

	metric := models.NewMultiValuesMetric(metricName, models.MetricTypeHistogram, tags, timestamp, multivalue.GetMultiValues())
	metric.Unit = metricUnit
	metric.Description = metricDescription
	metric.SetObservedTimestamp(uint64(startTimestamp))
	return metric
}

func genExponentialHistogramValues(isPositive bool, base float64, buckets pmetric.ExponentialHistogramDataPointBuckets) map[string]float64 {
	offset := buckets.Offset()
	rawbucketCounts := buckets.BucketCounts()
	res := make(map[string]float64, rawbucketCounts.Len())
	for i := 0; i < rawbucketCounts.Len(); i++ {
		bucketCount := rawbucketCounts.At(i)
		lowerBoundary := math.Pow(base, float64(int(offset)+i))
		upperBoundary := lowerBoundary * base
		fieldKey := otlp.ComposeBucketFieldName(lowerBoundary, upperBoundary, isPositive)
		res[fieldKey] = float64(bucketCount)
	}
	if isPositive {
		res[otlp.FieldPositiveOffset] = float64(offset)
	} else {
		res[otlp.FieldNegativeOffset] = float64(offset)
	}
	return res
}
