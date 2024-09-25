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
const std::string METRIC_LABEL_LOGSTORE = "logstore";
const std::string METRIC_LABEL_PIPELINE_NAME = "pipeline_name";
const std::string METRIC_LABEL_REGION = "region";

// metric keys
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENTS_TOTAL = "pipeline_processors_in_events_total";
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENT_GROUPS_TOTAL = "pipeline_processors_in_event_groups_total";
const std::string METRIC_PIPELINE_PROCESSORS_IN_EVENT_GROUP_SIZE_BYTES
    = "pipeline_processors_in_event_group_size_bytes";
const std::string METRIC_PIPELINE_PROCESSORS_TOTAL_DELAY_MS = "pipeline_processors_total_delay_ms";
const std::string METRIC_PIPELINE_START_TIME = "pipeline_start_time";

}