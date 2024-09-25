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
const std::string METRIC_LABEL_KEY_COMPONENT_NAME = "component_name";
const std::string METRIC_LABEL_KEY_FLUSHER_NODE_ID = "flusher_node_id";
const std::string METRIC_LABEL_KEY_EXACTLY_ONCE_FLAG = "is_exactly_once";
const std::string METRIC_LABEL_KEY_QUEUE_TYPE = "queue_type";

// label values
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_BATCHER = "batcher";
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_COMPRESSOR = "compressor";
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_PROCESS_QUEUE = "process_queue";
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_ROUTER = "router";
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_SENDER_QUEUE = "sender_queue";
const std::string METRIC_LABEL_VALUE_COMPONENT_NAME_SERIALIZER = "serializer";

// metric keys
const std::string METRIC_COMPONENT_IN_EVENTS_TOTAL = "component_in_events_total";
const std::string METRIC_COMPONENT_IN_EVENT_GROUP_SIZE_BYTES = "component_in_event_group_size_bytes";
const std::string METRIC_COMPONENT_IN_ITEMS_TOTAL = "component_in_items_total";
const std::string METRIC_COMPONENT_IN_ITEM_SIZE_BYTES = "component_in_item_size_bytes";
const std::string METRIC_COMPONENT_OUT_EVENTS_TOTAL = "component_out_events_total";
const std::string METRIC_COMPONENT_OUT_ITEMS_TOTAL = "component_out_items_total";
const std::string METRIC_COMPONENT_OUT_ITEM_SIZE_BYTES = "component_out_item_size_bytes";
const std::string METRIC_COMPONENT_TOTAL_DELAY_MS = "component_total_delay_ms";
const std::string METRIC_COMPONENT_TOTAL_PROCESS_MS = "component_total_process_ms";
const std::string METRIC_COMPONENT_DISCARDED_ITEMS_TOTAL = "component_discarded_items_total";
const std::string METRIC_COMPONENT_DISCARDED_ITEMS_SIZE_BYTES = "component_discarded_item_size_bytes";

/**********************************************************
*   batcher
**********************************************************/
const std::string METRIC_COMPONENT_BATCHER_EVENT_BATCHES_TOTAL = "component_event_batches_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_GROUPS_TOTAL = "component_buffered_groups_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_EVENTS_TOTAL = "component_buffered_events_total";
const std::string METRIC_COMPONENT_BATCHER_BUFFERED_SIZE_BYTES = "component_buffered_size_bytes";

/**********************************************************
*   queue
**********************************************************/
const std::string METRIC_COMPONENT_QUEUE_SIZE_TOTAL = "component_queue_size_total";
const std::string METRIC_COMPONENT_QUEUE_SIZE_BYTES = "component_queue_size_bytes";
const std::string METRIC_COMPONENT_QUEUE_VALID_TO_PUSH_FLAG = "component_valid_to_push";
const std::string METRIC_COMPONENT_QUEUE_EXTRA_BUFFER_SIZE_TOTAL = "component_extra_buffer_size_total";
const std::string METRIC_COMPONENT_QUEUE_EXTRA_BUFFER_SIZE_BYTES = "component_extra_buffer_size_bytes";
const std::string METRIC_COMPONENT_QUEUE_DISCARDED_EVENTS_TOTAL = "component_discarded_events_total";

}