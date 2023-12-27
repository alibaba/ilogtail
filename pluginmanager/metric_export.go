// Copyright 2022 iLogtail Authors
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

package pluginmanager

func GetMetrics() string {
	metrics := make([]map[string]string, 0)
	for _, config := range LogtailConfig {
		//config.Context.GetMetricRecords()
		metrics = append(metrics, config.Context.GetMetricRecords()...)
	}
	return "[{\"label.config_name\":\"##1.0##adb5-log-hangzhou$access-log\",\"label.logstore\":\"rc-access-log\",\"label.plugin_name\":\"processor_csv\",\"label.plugin_id\":1,\"label.project\":\"test\",\"label.region\":\"cn-shanghai\",\"proc_in_records_total\":123,\"proc_out_records_total\":123,\"proc_time_ms\":123},{\"label.config_name\":\"##1.0##adb5-log-hangzhou$access-log\",\"label.logstore\":\"rc-access-log\",\"label.plugin_name\":\"processor_regex\",\"label.plugin_id\":2,\"label.project\":\"test\",\"label.region\":\"cn-shanghai\",\"proc_in_records_total\":123,\"proc_out_records_total\":123,\"proc_time_ms\":123}]"
}
