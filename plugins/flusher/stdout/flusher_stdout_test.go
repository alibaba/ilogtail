package stdout

import (
	"encoding/json"
	"testing"

	"github.com/alibaba/ilogtail/pkg/models"
	"github.com/alibaba/ilogtail/plugins/test/mock"
	jsoniter "github.com/json-iterator/go"
	"github.com/stretchr/testify/assert"
)

func TestFlusherStdout_Init(t *testing.T) {
	flusher := &FlusherStdout{}
	err := flusher.Init(mock.NewEmptyContext("project", "logstore", "configName"))
	assert.NoError(t, err)
}

func TestFlusherStdout_Description(t *testing.T) {
	flusher := &FlusherStdout{}
	description := flusher.Description()
	assert.Equal(t, "stdout flusher for loongcollector", description)
}

func TestFlusherStdout_IsReady(t *testing.T) {
	flusher := &FlusherStdout{}
	ready := flusher.IsReady("project", "logstore", 0)
	assert.True(t, ready)
}

func TestFlusherStdout_Stop(t *testing.T) {
	flusher := &FlusherStdout{}
	err := flusher.Stop()
	assert.NoError(t, err)
}

func TestFlusherStdout_writeMetricValues(t *testing.T) {
	flusher := &FlusherStdout{
		context: mock.NewEmptyContext("project", "logstore", "configName"),
	}

	metric1 := &models.Metric{
		Name:       "cpu.load.short",
		MetricType: models.MetricTypeGauge,
		Timestamp:  1672321328000000000,
		Tags:       models.NewTagsWithKeyValues("host", "server01", "region", "cn"),
		Value:      &models.MetricSingleValue{Value: 0.64},
	}
	metric2 := &models.Metric{
		Name:       "cpu.load.short",
		MetricType: models.MetricTypeUntyped,
		Timestamp:  1672321328000000000,
		Tags:       models.NewTagsWithKeyValues("host", "server01", "region", "cn"),
		Value:      models.NewMetricMultiValueWithMap(map[string]float64{"value": 0.65}),
		TypedValue: models.NewMetricTypedValueWithMap(
			map[string]*models.TypedValue{
				"value1": {
					Value: "test",
					Type:  models.ValueTypeString,
				},
				"value2": {
					Value: true,
					Type:  models.ValueTypeBoolean,
				},
				"value3": {
					Value: 100,
					Type:  models.ValueTypeInteger,
				},
				"value4": {
					Value: 100,
					Type:  models.ValueTypeUnsigned,
				},
			},
		),
	}

	writer := jsoniter.NewStream(jsoniter.ConfigDefault, nil, 0)
	writer.WriteObjectStart()
	flusher.writeMetricValues(writer, metric1)
	writer.WriteObjectEnd()
	type singleValueMetric struct {
		MetricType string  `json:"metricType"`
		Value      float64 `json:"value"`
	}

	var e singleValueMetric
	err := json.Unmarshal(writer.Buffer(), &e)
	assert.NoError(t, err)
	assert.Equal(t, "Gauge", e.MetricType)
	assert.Equal(t, 0.64, e.Value)

	writer = jsoniter.NewStream(jsoniter.ConfigDefault, nil, 0)
	writer.WriteObjectStart()
	flusher.writeMetricValues(writer, metric2)
	writer.WriteObjectEnd()

	type typedValueMetric struct {
		MetricType string                 `json:"metricType"`
		Values     map[string]interface{} `json:"values"`
	}
	var e2 typedValueMetric
	err = json.Unmarshal(writer.Buffer(), &e2)
	assert.NoError(t, err)
	for k, v := range e2.Values {
		switch k {
		case "value":
			assert.Equal(t, 0.65, v)
		case "value1":
			assert.Equal(t, "test", v)
		case "value2":
			assert.Equal(t, true, v)
		case "value3":
			assert.Equal(t, float64(100), v)
		case "value4":
			assert.Equal(t, float64(100), v)
		}
	}
}
