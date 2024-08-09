// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//	http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
package pluginmanager

import (
	"runtime/metrics"
	"strconv"
	"strings"
)

func GetMetrics() []map[string]string {
	metrics := make([]map[string]string, 0)
	for _, config := range LogtailConfig {
		metrics = append(metrics, config.Context.ExportMetricRecords()...)
	}
	metrics = append(metrics, GetProcessStat())
	return metrics
}

func GetProcessStat() map[string]string {
	recrods := map[string]string{}
	recrods["metric-level"] = "process"
	// key is the metric key in runtime/metrics, value is agent's metric key
	metricNames := map[string]string{
		// cpu
		// "": "global_cpu_go_used_cores",
		// mem. All memory mapped by the Go runtime into the current process as read-write. Note that this does not include memory mapped by code called via cgo or via the syscall package. Sum of all metrics in /memory/classes.
		"/memory/classes/total:bytes": "global_memory_go_used_mb",
		// go routines cnt. Count of live goroutines.
		"/sched/goroutines:goroutines": "global_go_routines_total",
	}

	// metrics to read from runtime/metrics
	samples := make([]metrics.Sample, 0)
	for name := range metricNames {
		samples = append(samples, metrics.Sample{Name: name})
	}
	metrics.Read(samples)

	// push results to recrods
	for _, sample := range samples {
		recordName := metricNames[sample.Name]
		recordValue := sample.Value
		recordValueString := ""
		switch recordValue.Kind() {
		case metrics.KindUint64:
			if strings.HasSuffix(recordName, "_mb") {
				recordValueString = strconv.FormatUint(recordValue.Uint64()/1024/1024, 10)
			} else {
				recordValueString = strconv.FormatUint(recordValue.Uint64(), 10)
			}
		case metrics.KindFloat64:
			recordValueString = strconv.FormatFloat(recordValue.Float64(), 'g', -1, 64)
		}
		recrods[recordName] = recordValueString
	}
	return recrods
}
