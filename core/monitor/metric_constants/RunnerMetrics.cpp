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

#include "../MetricConstants.h"

namespace logtail {

// label keys
const std::string METRIC_LABEL_KEY_RUNNER_NAME = "runner_name";

// label values
const std::string METRIC_LABEL_VALUE_RUNNER_NAME_FILE_SERVER = "file_server";
const std::string METRIC_LABEL_VALUE_RUNNER_NAME_FLUSHER = "flusher_runner";
const std::string METRIC_LABEL_VALUE_RUNNER_NAME_HTTP_SINK = "http_sink";
const std::string METRIC_LABEL_VALUE_RUNNER_NAME_PROCESSOR = "processor_runner";

// metric keys
const std::string METRIC_RUNNER_IN_EVENTS_TOTAL = "runner_in_events_total";
const std::string METRIC_RUNNER_IN_EVENT_GROUPS_TOTAL = "runner_in_event_groups_total";
const std::string METRIC_RUNNER_IN_EVENT_GROUP_SIZE_BYTES = "runner_in_event_group_size_bytes";
const std::string METRIC_RUNNER_IN_ITEMS_TOTAL = "runner_in_items_total";
const std::string METRIC_RUNNER_IN_ITEM_SIZE_BYTES = "runner_in_item_size_bytes";
const std::string METRIC_RUNNER_LAST_RUN_TIME = "runner_last_run_time";
const std::string METRIC_RUNNER_OUT_ITEMS_TOTAL = "runner_out_items_total";
const std::string METRIC_RUNNER_TOTAL_DELAY_MS = "runner_total_delay_ms";

/**********************************************************
*   http sink
**********************************************************/
const std::string METRIC_RUNNER_HTTP_SINK_OUT_SUCCESSFUL_ITEMS_TOTAL = "runner_out_successful_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_OUT_FAILED_ITEMS_TOTAL = "runner_out_failed_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_SENDING_ITEMS_TOTAL = "runner_sending_items_total";
const std::string METRIC_RUNNER_HTTP_SINK_SEND_CONCURRENCY = "runner_send_concurrency";

/**********************************************************
*   flusher runner
**********************************************************/
const std::string METRIC_RUNNER_FLUSHER_IN_ITEM_RAW_SIZE_BYTES = "runner_in_item_raw_size_bytes";
const std::string METRIC_RUNNER_FLUSHER_WAITING_ITEMS_TOTAL = "runner_waiting_items_total";

/**********************************************************
*   file server
**********************************************************/
const std::string METRIC_RUNNER_FILE_WATCHED_DIRS_TOTAL = "runner_watched_dirs_total";
const std::string METRIC_RUNNER_FILE_ACTIVE_READERS_TOTAL = "runner_active_readers_total";
const std::string METRIC_RUNNER_FILE_ENABLE_FILE_INCLUDED_BY_MULTI_CONFIGS_FLAG
    = "runner_enable_file_included_by_multi_configs";
const std::string METRIC_RUNNER_FILE_POLLING_MODIFY_CACHE_SIZE = "runner_polling_modify_cache_size";
const std::string METRIC_RUNNER_FILE_POLLING_DIR_CACHE_SIZE = "runner_polling_dir_cache_size";
const std::string METRIC_RUNNER_FILE_POLLING_FILE_CACHE_SIZE = "runner_polling_file_cache_size";

}