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

package pipeline

import (
	"context"
)

type CommonContext struct {
	Project    string
	Logstore   string
	ConfigName string
}

type CommonMetrics struct {
	ProcInRecordsTotal  CounterMetric
	ProcOutRecordsTotal CounterMetric
	ProcTimeMS          CounterMetric
}

type MetricsRecord struct {
	Labels map[string]string

	*CommonMetrics
	CounterMetrics []CounterMetric
	StringMetrics  []StringMetric
	LatencyMetrics []LatencyMetric
}

// Context for plugin
type Context interface {
	InitContext(project, logstore, configName string)

	GetConfigName() string
	GetProject() string
	GetLogstore() string
	GetRuntimeContext() context.Context
	GetExtension(name string, cfg any) (Extension, error)

	GetMetricRecords() []map[string]string
	RegisterMetricRecord(labels map[string]string) *MetricsRecord

	SetMetricRecord(metricsRecord *MetricsRecord)
	GetMetricRecord() *MetricsRecord

	RegisterCounterMetric(metricsRecord *MetricsRecord, metric CounterMetric)
	RegisterStringMetric(metricsRecord *MetricsRecord, metric StringMetric)
	RegisterLatencyMetric(metricsRecord *MetricsRecord, metric LatencyMetric)

	SaveCheckPoint(key string, value []byte) error
	GetCheckPoint(key string) (value []byte, exist bool)
	SaveCheckPointObject(key string, obj interface{}) error
	GetCheckPointObject(key string, obj interface{}) (exist bool)
}
