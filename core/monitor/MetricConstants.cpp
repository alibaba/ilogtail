// Copyright 2023 iLogtail Authors
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

namespace logtail {


const std::string METRIC_FIELD_REGION = "region";
const std::string METRIC_REGION_DEFAULT = "default";
const std::string METRIC_SLS_LOGSTORE_NAME = "shennong_log_profile";
const std::string METRIC_TOPIC_TYPE = "logtail_metric";
const std::string METRIC_TOPIC_FIELD_NAME = "__topic__";

const std::string LABEL_PREFIX = "label.";
const std::string VALUE_PREFIX = "value.";

// global metrics labels

const std::string METRIC_LABEL_ALIUIDS = "aliuids";
const std::string METRIC_LABEL_INSTANCE_ID = "instance_id";
const std::string METRIC_LABEL_IP = "ip";
const std::string METRIC_LABEL_OS = "os";
const std::string METRIC_LABEL_OS_DETAIL = "os_detail";
const std::string METRIC_LABEL_USER_DEFINED_ID = "user_defined_id";
const std::string METRIC_LABEL_UUID = "uuid";
const std::string METRIC_LABEL_VERSION = "version";

// global metrics values

const std::string METRIC_AGENT_CPU = "agent_cpu_percent";
const std::string METRIC_AGENT_CPU_GO = "agent_go_cpu_percent";
const std::string METRIC_AGENT_MEMORY = "agent_memory_used_mb";
const std::string METRIC_AGENT_MEMORY_GO = "agent_go_memory_used_mb";
const std::string METRIC_AGENT_GO_ROUTINES_TOTAL = "agent_go_routines_total";
const std::string METRIC_AGENT_OPEN_FD_TOTAL = "agent_open_fd_total";
const std::string METRIC_AGENT_INSTANCE_CONFIG_TOTAL = "agent_instance_config_total";
const std::string METRIC_AGENT_PIPELINE_CONFIG_TOTAL = "agent_pipeline_config_total";
const std::string METRIC_AGENT_ENV_PIPELINE_CONFIG_TOTAL = "agent_env_pipeline_config_total";
const std::string METRIC_AGENT_CRD_PIPELINE_CONFIG_TOTAL = "agent_crd_pipeline_config_total";
const std::string METRIC_AGENT_CONSOLE_PIPELINE_CONFIG_TOTAL = "agent_console_pipeline_config_total";
const std::string METRIC_AGENT_PLUGIN_TOTAL = "agent_plugin_total";

// common plugin labels
const std::string METRIC_LABEL_PROJECT = "project";
const std::string METRIC_LABEL_LOGSTORE = "logstore";
const std::string METRIC_LABEL_REGION = "region";
const std::string METRIC_LABEL_CONFIG_NAME = "config_name";
const std::string METRIC_LABEL_PLUGIN_NAME = "plugin_name";
const std::string METRIC_LABEL_PLUGIN_ID = "plugin_id";
const std::string METRIC_LABEL_NODE_ID = "node_id";
const std::string METRIC_LABEL_CHILD_NODE_ID = "child_node_id";

const std::string METRIC_LABEL_KEY_COMPONENT_NAME = "component_name";
const std::string METRIC_LABEL_KEY_RUNNER_NAME = "runner_name";
const std::string METRIC_LABEL_KEY_QUEUE_TYPE = "queue_type";
const std::string METRIC_LABEL_KEY_EXACTLY_ONCE_FLAG = "is_exactly_once";
const std::string METRIC_LABEL_KEY_FLUSHER_NODE_ID = "flusher_node_id";

// input file plugin labels
const std::string METRIC_LABEL_FILE_DEV = "file_dev";
const std::string METRIC_LABEL_FILE_INODE = "file_inode";
const std::string METRIC_LABEL_FILE_NAME = "file_name";

// input file metrics
const std::string METRIC_INPUT_RECORDS_TOTAL = "input_records_total";
const std::string METRIC_INPUT_RECORDS_SIZE_BYTES = "input_records_size_bytes";
const std::string METRIC_INPUT_BATCH_TOTAL = "input_batch_total";
const std::string METRIC_INPUT_READ_TOTAL = "input_read_total";
const std::string METRIC_INPUT_FILE_SIZE_BYTES = "input_file_size_bytes";
const std::string METRIC_INPUT_FILE_READ_DELAY_TIME_MS = "input_file_read_delay_time_ms";
const std::string METRIC_INPUT_FILE_OFFSET_BYTES = "input_file_offset_bytes";
const std::string METRIC_INPUT_FILE_MONITOR_TOTAL = "input_file_monitor_total";

// processor common metrics
const std::string METRIC_PROC_IN_RECORDS_TOTAL = "proc_in_records_total";
const std::string METRIC_PROC_IN_RECORDS_SIZE_BYTES = "proc_in_records_size_bytes";
const std::string METRIC_PROC_OUT_RECORDS_TOTAL = "proc_out_records_total";
const std::string METRIC_PROC_OUT_RECORDS_SIZE_BYTES = "proc_out_records_size_bytes";
const std::string METRIC_PROC_DISCARD_RECORDS_TOTAL = "proc_discard_records_total";
const std::string METRIC_PROC_TIME_MS = "proc_time_ms";

// processor cunstom metrics
const std::string METRIC_PROC_PARSE_IN_SIZE_BYTES = "proc_parse_in_size_bytes";
const std::string METRIC_PROC_PARSE_OUT_SIZE_BYTES = "proc_parse_out_size_bytes";

const std::string METRIC_PROC_PARSE_ERROR_TOTAL = "proc_parse_error_total";
const std::string METRIC_PROC_PARSE_SUCCESS_TOTAL = "proc_parse_success_total";
const std::string METRIC_PROC_KEY_COUNT_NOT_MATCH_ERROR_TOTAL = "proc_key_count_not_match_error_total";
const std::string METRIC_PROC_HISTORY_FAILURE_TOTAL = "proc_history_failure_total";

const std::string METRIC_PROC_SPLIT_MULTILINE_LOG_MATCHED_RECORDS_TOTAL
    = "proc_split_multiline_log_matched_records_total";
const std::string METRIC_PROC_SPLIT_MULTILINE_LOG_MATCHED_LINES_TOTAL = "proc_split_multiline_log_matched_lines_total";
const std::string METRIC_PROC_SPLIT_MULTILINE_LOG_UNMATCHED_LINES_TOTAL
    = "proc_split_multiline_log_unmatched_lines_total";

// processor filter metrics
const std::string METRIC_PROC_FILTER_IN_SIZE_BYTES = "proc_filter_in_size_bytes";
const std::string METRIC_PROC_FILTER_OUT_SIZE_BYTES = "proc_filter_out_size_bytes";
const std::string METRIC_PROC_FILTER_ERROR_TOTAL = "proc_filter_error_total";
const std::string METRIC_PROC_FILTER_RECORDS_TOTAL = "proc_filter_records_total";

// processore plugin name
const std::string PLUGIN_PROCESSOR_PARSE_REGEX_NATIVE = "processor_parse_regex_native";

// processor desensitize metrics
const std::string METRIC_PROC_DESENSITIZE_RECORDS_TOTAL = "proc_desensitize_records_total";

// processor merge multiline log metrics
const std::string METRIC_PROC_MERGE_MULTILINE_LOG_MERGED_RECORDS_TOTAL
    = "proc_merge_multiline_log_merged_records_total";
const std::string METRIC_PROC_MERGE_MULTILINE_LOG_UNMATCHED_RECORDS_TOTAL
    = "proc_merge_multiline_log_unmatched_records_total";

// processor parse container log native metrics
const std::string METRIC_PROC_PARSE_STDOUT_TOTAL = "proc_parse_stdout_total";
const std::string METRIC_PROC_PARSE_STDERR_TOTAL = "proc_parse_stderr_total";

// flusher common metrics
const std::string METRIC_FLUSHER_ERROR_TOTAL = "flusher_error_total";
const std::string METRIC_FLUSHER_DISCARD_RECORDS_TOTAL = "flusher_discard_records_total";
const std::string METRIC_FLUSHER_SUCCESS_RECORDS_TOTAL = "flusher_success_records_total";
const std::string METRIC_FLUSHER_SUCCESS_TIME_MS = "flusher_success_time_ms";
const std::string METRIC_FLUSHER_ERROR_TIME_MS = "flusher_error_time_ms";

// flusher sls metrics
const std::string METRIC_FLUSHER_NETWORK_ERROR_TOTAL = "flusher_network_error_total";
const std::string METRIC_FLUSHER_QUOTA_ERROR_TOTAL = "flusher_quota_error_total";
const std::string METRIC_FLUSHER_RETRIES_TOTAL = "flusher_retries_total";
const std::string METRIC_FLUSHER_RETRIES_ERROR_TOTAL = "flusher_retries_error_total";

const std::string METRIC_IN_EVENTS_CNT = "in_events_cnt";
const std::string METRIC_IN_EVENT_GROUPS_CNT = "in_event_groups_cnt";
const std::string METRIC_IN_ITEMS_CNT = "in_items_cnt";
const std::string METRIC_IN_EVENT_GROUP_SIZE_BYTES = "in_event_group_data_size_bytes";
const std::string METRIC_IN_ITEM_SIZE_BYTES = "in_item_data_size_bytes";
const std::string METRIC_OUT_EVENTS_CNT = "out_events_cnt";
const std::string METRIC_OUT_ITEMS_CNT = "out_items_cnt";
const std::string METRIC_OUT_EVENT_GROUP_SIZE_BYTES = "out_event_group_data_size_bytes";
const std::string METRIC_OUT_ITEM_SIZE_BYTES = "out_item_data_size_bytes";
const std::string METRIC_TOTAL_DELAY_MS = "total_delay_ms";
const std::string METRIC_LAST_RUN_TIME = "last_run_time";

} // namespace logtail
