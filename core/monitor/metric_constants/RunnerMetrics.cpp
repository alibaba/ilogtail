// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "MetricConstants.h"

using namespace std;

namespace logtail {

// label keys
const string METRIC_LABEL_KEY_RUNNER_NAME = "runner_name";

// label values
const string METRIC_LABEL_KEY_METRIC_CATEGORY_RUNNER = "runner";
const string METRIC_LABEL_VALUE_RUNNER_NAME_FILE_SERVER = "file_server";
const string METRIC_LABEL_VALUE_RUNNER_NAME_FLUSHER = "flusher_runner";
const string METRIC_LABEL_VALUE_RUNNER_NAME_HTTP_SINK = "http_sink";
const string METRIC_LABEL_VALUE_RUNNER_NAME_PROCESSOR = "processor_runner";
const string METRIC_LABEL_VALUE_RUNNER_NAME_PROMETHEUS = "prometheus_runner";
const string METRIC_LABEL_VALUE_RUNNER_NAME_EBPF_SERVER = "ebpf_server";

// metric keys
const string METRIC_RUNNER_IN_EVENTS_TOTAL = "runner_in_events_total";
const string METRIC_RUNNER_IN_EVENT_GROUPS_TOTAL = "runner_in_event_groups_total";
const string METRIC_RUNNER_IN_SIZE_BYTES = "runner_in_size_bytes";
const string METRIC_RUNNER_IN_ITEMS_TOTAL = "runner_in_items_total";
const string METRIC_RUNNER_LAST_RUN_TIME = "runner_last_run_time";
const string METRIC_RUNNER_OUT_ITEMS_TOTAL = "runner_out_items_total";
const string METRIC_RUNNER_TOTAL_DELAY_MS = "runner_total_delay_ms";
const string METRIC_RUNNER_CLIENT_REGISTER_STATE = "runner_client_register_state";
const string METRIC_RUNNER_CLIENT_REGISTER_RETRY_TOTAL = "runner_client_register_retry_total";
const string METRIC_RUNNER_JOBS_TOTAL = "runner_jobs_total";

/**********************************************************
 *   all sinks
 **********************************************************/
const string METRIC_RUNNER_SINK_OUT_SUCCESSFUL_ITEMS_TOTAL = "runner_out_successful_items_total";
const string METRIC_RUNNER_SINK_OUT_FAILED_ITEMS_TOTAL = "runner_out_failed_items_total";
const string METRIC_RUNNER_SINK_SUCCESSFUL_ITEM_TOTAL_RESPONSE_TIME_MS = "runner_successful_item_total_response_time_ms";
const string METRIC_RUNNER_SINK_FAILED_ITEM_TOTAL_RESPONSE_TIME_MS = "runner_failed_item_total_response_time_ms";
const string METRIC_RUNNER_SINK_SENDING_ITEMS_TOTAL = "runner_sending_items_total";
const string METRIC_RUNNER_SINK_SEND_CONCURRENCY = "runner_send_concurrency";

/**********************************************************
 *   flusher runner
 **********************************************************/
const string METRIC_RUNNER_FLUSHER_IN_RAW_SIZE_BYTES = "runner_in_raw_size_bytes";
const string METRIC_RUNNER_FLUSHER_WAITING_ITEMS_TOTAL = "runner_waiting_items_total";

/**********************************************************
 *   file server
 **********************************************************/
const string METRIC_RUNNER_FILE_WATCHED_DIRS_TOTAL = "runner_watched_dirs_total";
const string METRIC_RUNNER_FILE_ACTIVE_READERS_TOTAL = "runner_active_readers_total";
const string METRIC_RUNNER_FILE_ENABLE_FILE_INCLUDED_BY_MULTI_CONFIGS_FLAG
    = "runner_enable_file_included_by_multi_configs";
const string METRIC_RUNNER_FILE_POLLING_MODIFY_CACHE_SIZE = "runner_polling_modify_cache_size";
const string METRIC_RUNNER_FILE_POLLING_DIR_CACHE_SIZE = "runner_polling_dir_cache_size";
const string METRIC_RUNNER_FILE_POLLING_FILE_CACHE_SIZE = "runner_polling_file_cache_size";

/**********************************************************
 *   ebpf server
 **********************************************************/
const string METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE = "recv_event_stage";
const string METRIC_LABEL_KEY_RUNNER_EVENT_TYPE = "event_type";
const string METRIC_LABEL_KEY_RUNNER_PARSER_PROTOCOL = "parser_protocol";
const string METRIC_LABEL_KEY_RUNNER_PARSE_STATUS = "parser_status";
const string METRIC_LABEL_KEY_RUNNER_PLUGIN_TYPE = "plugin_type";

const string METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_POLL_KERNEL = "poll_kernel";
const string METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER = "after_perf_worker";
const string METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_REPORT_TO_LC = "report_to_lc";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_CONN_STATS = "conn_stats";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_DATA_EVENT = "data_event";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_CTRL_EVENT = "ctrl_event";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_LOG = "log";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_METRIC = "metric";
const string METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_TRACE = "trace";
const string METRIC_LABEL_VALUE_RUNNER_PARSER_PROTOCOL_HTTP = "http";
const string METRIC_LABEL_VALUE_RUNNER_PARSE_STATUS_SUCCESS = "success";
const string METRIC_LABEL_VALUE_RUNNER_PARSE_STATUS_FAILED = "failed";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_NETWORK_OBSERVER = "network_observer";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_NETWORK_SECURITY = "network_security";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_FILE_OBSERVER = "file_observer";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_FILE_SECURITY = "file_security";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_PROCESS_OBSERVER = "process_observer";
const string METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_PROCESS_SECURITY = "process_security";

const string METRIC_RUNNER_EBPF_LOSS_KERNEL_EVENTS_TOTAL = "runner_loss_kernel_events_total";
const string METRIC_RUNNER_EBPF_NETWORK_OBSERVER_CONNTRACKER_NUM = "runner_network_observer_conntracker_num";
const string METRIC_RUNNER_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL = "runner_network_observer_worker_handle_events_total";
const string METRIC_RUNNER_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL = "runner_network_observer_parse_records_total";
const string METRIC_RUNNER_EBPF_NETWORK_OBSERVER_AGGREGATE_EVENTS_TOTAL = "runner_network_observer_aggregate_events_total";
const string METRIC_RUNNER_EBPF_NETWORK_OBSERVER_AGGREGATE_KEY_NUM = "runner_network_observer_aggregate_key_num";
const string METRIC_RUNNER_EBPF_PROCESS_CACHE_ENTRIES_NUM = "runner_process_cache_entries_num";
const string METRIC_RUNNER_EBPF_PROCESS_CACHE_MISS_TOTAL = "runner_process_cache_miss_total";

} // namespace logtail