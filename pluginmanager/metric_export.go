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
	goruntimemetrics "runtime/metrics"
	"strconv"
	"strings"
)

const (
	MetricExportTypeGo  = "direct"
	MetricExportTypeCpp = "cpp_provided"
)

func GetMetrics(metricType string) []map[string]string {
	if metricType == MetricExportTypeGo {
		return GetGoDirectMetrics()
	}
	if metricType == MetricExportTypeCpp {
		return GetGoCppProvidedMetrics()
	}
	return []map[string]string{}
}

// 直接输出的go指标，例如go插件指标
// [
//
//		{
//	    	"label.plugin_name": "processor_test",
//	    	"value.proc_in_records_total": "100"
//		},
//		{
//	    	"label.plugin_name": "flusher_stdout",
//	    	"value.flusher_in_records_total": "100"
//		}
//
// ]
func GetGoDirectMetrics() []map[string]string {
	metrics := make([]map[string]string, 0)
	// go plugin metrics
	metrics = append(metrics, GetGoPluginMetrics()...)
	return metrics
}

// 由C++定义的指标，go把值传过去，例如go的进程级指标
// [
//
//	    {
//	        "agent_go_memory_used_mb": "100",
//			"agent_go_routines_total": "20"
//	    }
//
// ]
func GetGoCppProvidedMetrics() []map[string]string {
	metrics := make([]map[string]string, 0)
	// agent-level metrics
	metrics = append(metrics, GetAgentStat()...)
	return metrics
}

// go 插件指标，直接输出
func GetGoPluginMetrics() []map[string]string {
	metrics := make([]map[string]string, 0)
	for _, config := range LogtailConfig {
		metrics = append(metrics, config.Context.ExportMetricRecords()...)
	}
	return metrics
}

// go 进程级指标，由C++部分注册
func GetAgentStat() []map[string]string {
	metrics := []map[string]string{}
	metric := map[string]string{}
	// key is the metric key in runtime/metrics, value is agent's metric key
	metricNames := map[string]string{
		// cpu
		// "": "agent_go_cpu_percent",
		// mem. All memory mapped by the Go runtime into the current process as read-write. Note that this does not include memory mapped by code called via cgo or via the syscall package. Sum of all metrics in /memory/classes.
		"/memory/classes/total:bytes": "agent_go_memory_used_mb",
		// go routines cnt. Count of live goroutines.
		"/sched/goroutines:goroutines": "agent_go_routines_total",
	}

	// metrics to read from runtime/metrics
	samples := make([]goruntimemetrics.Sample, 0)
	for name := range metricNames {
		samples = append(samples, goruntimemetrics.Sample{Name: name})
	}
	goruntimemetrics.Read(samples)

	// push results to recrods
	for _, sample := range samples {
		recordName := metricNames[sample.Name]
		recordValue := sample.Value
		recordValueString := ""
		switch recordValue.Kind() {
		case goruntimemetrics.KindUint64:
			if strings.HasSuffix(recordName, "_mb") {
				recordValueString = strconv.FormatUint(recordValue.Uint64()/1024/1024, 10)
			} else {
				recordValueString = strconv.FormatUint(recordValue.Uint64(), 10)
			}
		case goruntimemetrics.KindFloat64:
			recordValueString = strconv.FormatFloat(recordValue.Float64(), 'g', -1, 64)
		}
		metric[recordName] = recordValueString
	}

	metrics = append(metrics, metric)
	return metrics
}
