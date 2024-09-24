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

//////////////////////////////////////////////////////////////////////////
// labels
//////////////////////////////////////////////////////////////////////////

// common
const std::string METRIC_LABEL_PROJECT = "project";

// agent
const std::string METRIC_LABEL_ALIUIDS = "aliuids";
const std::string METRIC_LABEL_INSTANCE_ID = "instance_id";
const std::string METRIC_LABEL_IP = "ip";
const std::string METRIC_LABEL_OS = "os";
const std::string METRIC_LABEL_OS_DETAIL = "os_detail";
const std::string METRIC_LABEL_USER_DEFINED_ID = "user_defined_id";
const std::string METRIC_LABEL_UUID = "uuid";
const std::string METRIC_LABEL_VERSION = "version";

// pipeline
const std::string METRIC_LABEL_LOGSTORE = "logstore";
const std::string METRIC_LABEL_REGION = "region";
const std::string METRIC_LABEL_PIPELINE_NAME = "pipeline_name";

// plugin
const std::string METRIC_LABEL_PLUGIN_TYPE = "plugin_type";
const std::string METRIC_LABEL_PLUGIN_ID = "plugin_id";
const std::string METRIC_LABEL_NODE_ID = "node_id";
const std::string METRIC_LABEL_CHILD_NODE_ID = "child_node_id";
// input file plugin labels
const std::string METRIC_LABEL_FILE_DEV = "file_dev";
const std::string METRIC_LABEL_FILE_INODE = "file_inode";
const std::string METRIC_LABEL_FILE_NAME = "file_name";

// component
const std::string METRIC_LABEL_KEY_COMPONENT_NAME = "component_name";
const std::string METRIC_LABEL_KEY_QUEUE_TYPE = "queue_type";
const std::string METRIC_LABEL_KEY_EXACTLY_ONCE_FLAG = "is_exactly_once";
const std::string METRIC_LABEL_KEY_FLUSHER_NODE_ID = "flusher_node_id";

// runner
const std::string METRIC_LABEL_KEY_RUNNER_NAME = "runner_name";

//////////////////////////////////////////////////////////////////////////
// agent
//////////////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////////////
// plugin
//////////////////////////////////////////////////////////////////////////

// common metrics
const std::string METRIC_PLUGIN_COST_TIME_MS = "plugin_cost_time_ms";
const std::string METRIC_PLUGIN_IN_BUFFER_TOTAL = "plugin_in_buffer_total";
const std::string METRIC_PLUGIN_IN_BUFFER_SIZE_BYTES = "plugin_in_buffer_size_bytes";
const std::string METRIC_PLUGIN_OUT_BUFFER_TOTAL = "plugin_out_buffer_total";
const std::string METRIC_PLUGIN_OUT_BUFFER_SIZE_BYTES = "plugin_out_buffer_size_bytes";
const std::string METRIC_PLUGIN_IN_EVENTS_TOTAL = "plugin_in_events_total";
const std::string METRIC_PLUGIN_IN_EVENTS_SIZE_BYTES = "plugin_in_events_size_bytes";
const std::string METRIC_PLUGIN_OUT_EVENTS_TOTAL = "plugin_out_events_total";
const std::string METRIC_PLUGIN_OUT_EVENTS_SIZE_BYTES = "plugin_out_events_size_bytes";
const std::string METRIC_PLUGIN_IN_EVENT_GROUPS_TOTAL = "plugin_in_event_groups_total";
const std::string METRIC_PLUGIN_IN_EVENT_GROUPS_SIZE_BYTES = "plugin_in_event_groups_size_bytes";
const std::string METRIC_PLUGIN_OUT_EVENT_GROUPS_TOTAL = "plugin_in_event_groups_total";
const std::string METRIC_PLUGIN_OUT_EVENT_GROUPS_SIZE_BYTES = "plugin_out_event_groups_size_bytes";
const std::string METRIC_PLUGIN_DISCARD_EVENTS_TOTAL = "plugin_discard_events_total";
const std::string METRIC_PLUGIN_ERROR_TOTAL = "plugin_error_total";

// input input_file/input_container_stdio cunstom metrics
const std::string METRIC_PLUGIN_READ_FILE_SIZE_BYTES = "plugin_read_file_size_bytes";
const std::string METRIC_PLUGIN_READ_FILE_OFFSET_BYTES = "plugin_read_file_offset_bytes";
const std::string METRIC_PLUGIN_MONITOR_FILE_TOTAL = "plugin_monitor_file_total";

// processor processor_parse_regex_native cunstom metrics
const std::string METRIC_PLUGIN_KEY_COUNT_NOT_MATCH_ERROR_TOTAL = "plugin_key_count_not_match_error_total";

// processor processor_parse_apsara_native/processor_parse_timestamp_native cunstom metrics
const std::string METRIC_PLUGIN_HISTORY_FAILURE_TOTAL = "plugin_history_failure_total";

// processor  processor_split_multiline_log_string_native cunstom metrics
const std::string METRIC_PLUGIN_SPLIT_MULTILINE_LOG_MATCHED_RECORDS_TOTAL
    = "plugin_split_multiline_log_matched_records_total";
const std::string METRIC_PLUGIN_SPLIT_MULTILINE_LOG_MATCHED_LINES_TOTAL
    = "plugin_split_multiline_log_matched_lines_total";
const std::string METRIC_PLUGIN_SPLIT_MULTILINE_LOG_UNMATCHED_LINES_TOTAL
    = "plugin_split_multiline_log_unmatched_lines_total";

// processor processor_desensitize cunstom metrics
const std::string METRIC_PLUGIN_DESENSITIZE_RECORDS_TOTAL = "plugin_desensitize_records_total";

// processor processor_merge_multiline_log_native cunstom metrics
const std::string METRIC_PLUGIN_MERGE_MULTILINE_LOG_MERGED_RECORDS_TOTAL
    = "plugin_merge_multiline_log_merged_records_total";
const std::string METRIC_PLUGIN_MERGE_MULTILINE_LOG_UNMATCHED_RECORDS_TOTAL
    = "plugin_merge_multiline_log_unmatched_records_total";

// processor processor_parse_container_log_native cunstom metrics
const std::string METRIC_PLUGIN_PARSE_STDOUT_TOTAL = "plugin_parse_stdout_total";
const std::string METRIC_PLUGIN_PARSE_STDERR_TOTAL = "plugin_parse_stderr_total";

// flusher flusher_sls cunstom metrics
const std::string METRIC_PLUGIN_NETWORK_ERROR_TOTAL = "plugin_network_error_total";
const std::string METRIC_PLUGIN_QUOTA_ERROR_TOTAL = "plugin_quota_error_total";
const std::string METRIC_PLUGIN_RETRIES_TOTAL = "plugin_retries_total";

//////////////////////////////////////////////////////////////////////////
// component
//////////////////////////////////////////////////////////////////////////

// common metrics
const std::string METRIC_COMPONENT_IN_EVENTS_TOTAL = "component_in_events_total";
const std::string METRIC_COMPONENT_IN_ITEMS_TOTAL = "component_in_items_total";
const std::string METRIC_COMPONENT_IN_EVENT_GROUP_SIZE_BYTES = "component_in_event_group_size_bytes";
const std::string METRIC_COMPONENT_IN_ITEM_SIZE_BYTES = "component_in_item_size_bytes";
const std::string METRIC_COMPONENT_OUT_EVENTS_TOTAL = "component_out_events_total";
const std::string METRIC_COMPONENT_OUT_ITEMS_TOTAL = "component_out_items_total";
const std::string METRIC_COMPONENT_OUT_ITEM_SIZE_BYTES = "component_out_item_size_bytes";
const std::string METRIC_COMPONENT_TOTAL_DELAY_MS = "component_total_delay_ms";
const std::string METRIC_COMPONENT_DISCARDED_ITEMS_TOTAL = "component_discarded_items_total";
const std::string METRIC_COMPONENT_DISCARDED_ITEMS_SIZE_BYTES = "component_discarded_item_size_bytes";

// batcher metrics
const std::string METRIC_COMPONENT_BATCHER_EVENT_BATCHES_TOTAL = "component_event_batches_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_GROUPS_TOTAL = "component_buffered_groups_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_EVENTS_TOTAL = "component_buffered_events_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_SIZE_BYTES = "component_buffered_size_bytes";

// queue metrics
const std::string METRIC_COMPONENT_QUEUE_SIZE_TOTAL = "component_queue_size";
const std::string METRIC_COMPONENT_QUEUE_SIZE_BYTES = "component_queue_size_bytes";
const std::string METRIC_COMPONENT_QUEUE_VALID_TO_PUSH_FLAG = "component_valid_to_push";
const std::string METRIC_COMPONENT_QUEUE_EXTRA_BUFFER_SIZE = "component_extra_buffer_size";
const std::string METRIC_COMPONENT_QUEUE_EXTRA_BUFFER_SIZE_BYTES = "component_extra_buffer_size";
const std::string METRIC_COMPONENT_QUEUE_DISCARDED_EVENTS_TOTAL = "component_discarded_events_total";

//////////////////////////////////////////////////////////////////////////
// pipeline
//////////////////////////////////////////////////////////////////////////

const std::string METRIC_PIPELINE_START_TIME = "pipeline_start_time";
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENTS_TOTAL = "pipeline_processors_in_events_total";
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENT_GROUPS_TOTAL = "pipeline_processors_in_event_groups_total";
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENT_GROUP_SIZE_BYTES
    = "pipeline_processors_in_event_group_size_bytes";
const std::string METRIC_PIPELINE_PROCESSORS_TOTAL_DELAY_MS = "pipeline_processors_total_delay_ms";

//////////////////////////////////////////////////////////////////////////
// runner
//////////////////////////////////////////////////////////////////////////

// common metrics
const std::string METRIC_RUNNER_IN_EVENTS_TOTAL = "runner_in_events_total";
const std::string METRIC_RUNNER_IN_EVENT_GROUPS_TOTAL = "runner_in_event_groups_total";
const std::string METRIC_RUNNER_IN_ITEMS_TOTAL = "runner_in_items_total";
const std::string METRIC_RUNNER_IN_EVENT_GROUP_SIZE_BYTES = "runner_in_event_group_size_bytes";
const std::string METRIC_RUNNER_IN_ITEM_SIZE_BYTES = "runner_in_item_size_bytes";
const std::string METRIC_RUNNER_OUT_ITEMS_TOTAL = "runner_out_items_total";
const std::string METRIC_RUNNER_TOTAL_DELAY_MS = "runner_total_delay_ms";
const std::string METRIC_RUNNER_LAST_RUN_TIME = "runner_last_run_time";

// http sink metrics
const std::string METRIC_RUNNER_HTTP_SINK_OUT_SUCCESSFUL_ITEMS_TOTAL = "runner_out_successful_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_OUT_FAILED_ITEMS_TOTAL = "runner_out_failed_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_SENDING_ITEMS_TOTAL = "runner_sending_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_SEND_CONCURRENCY = "runner_send_concurrency";

// flusher runner metrics
const std::string METRIC_RUNNER_FLUSHER_IN_ITEM_RAW_SIZE_BYTES = "runner_in_item_raw_size_bytes";
const std::string METRIC_RUNNER_FLUSHER_WAITING_ITEMS_TOTAL = "runner_waiting_items_total";

// file server metrics
const std::string METRIC_RUNNER_FILE_WATCHED_DIRS_TOTAL = "runner_watched_dirs_total";
const std::string METRIC_RUNNER_FILE_ACTIVE_READERS_TOTAL = "runner_active_readers_total";
const std::string METRIC_RUNNER_FILE_ENABLE_FILE_INCLUDED_BY_MULTI_CONFIGS_FLAG
    = "runner_enable_file_included_by_multi_configs";
const std::string METRIC_RUNNER_FILE_POLLING_MODIFY_CACHE_SIZE = "runner_polling_modify_cache_size";
const std::string METRIC_RUNNER_FILE_POLLING_DIR_CACHE_SIZE = "runner_polling_dir_cache_size";
const std::string METRIC_RUNNER_FILE_POLLING_FILE_CACHE_SIZE = "runner_polling_file_cache_size";

} // namespace logtail
