// Copyright 2021 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package mock

import (
	"time"

	"github.com/alibaba/ilogtail"
	"github.com/alibaba/ilogtail/pkg/models"
)

type InputMock struct {
	GroupMeta map[string]string
	GroupTags map[string]string

	Tags   map[string]string
	Fields map[string]float64
	Index  int64

	context ilogtail.Context
}

func (r *InputMock) Init(context ilogtail.Context) (int, error) {
	r.context = context
	return 0, nil
}

func (r *InputMock) Description() string {
	return "mock input plugin for logtail"
}

func (r *InputMock) Collect(context ilogtail.PipelineContext) error {
	r.Index++
	group := models.NewGroup(models.NewMetadataWithMap(r.GroupMeta), models.NewTagsWithMap(r.GroupTags))
	counter := models.NewSingleValueMetric("counter_metrics_mock", models.MetricTypeCounter, models.NewTagsWithMap(r.Tags), time.Now().UnixNano(), r.Index)
	gauge := models.NewSingleValueMetric("gauge_metrics_mock", models.MetricTypeGauge, models.NewTagsWithMap(r.Tags), time.Now().UnixNano(), r.Index)
	multiValues := models.NewMultiValuesMetric("multi_values_metrics_mock", models.MetricTypeUntyped, models.NewTagsWithMap(r.Tags), time.Now().UnixNano(), models.NewMetricFloatValuesWithMap(r.Fields))
	context.Collector().Collect(group, counter, gauge, multiValues)
	return nil
}

func init() {
	ilogtail.MetricInputs["metric_mock"] = func() ilogtail.MetricInput {
		return &InputMock{
			Index:     0,
			GroupMeta: make(map[string]string),
			GroupTags: make(map[string]string),
			Tags:      make(map[string]string),
			Fields:    make(map[string]float64),
		}
	}
}
