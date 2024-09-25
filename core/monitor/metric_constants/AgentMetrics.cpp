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
const std::string METRIC_LABEL_ALIUIDS = "aliuids";
const std::string METRIC_LABEL_INSTANCE_ID = "instance_id";
const std::string METRIC_LABEL_IP = "ip";
const std::string METRIC_LABEL_OS = "os";
const std::string METRIC_LABEL_OS_DETAIL = "os_detail";
const std::string METRIC_LABEL_USER_DEFINED_ID = "user_defined_id";
const std::string METRIC_LABEL_UUID = "uuid";
const std::string METRIC_LABEL_VERSION = "version";

// metric keys
const std::string METRIC_AGENT_CONSOLE_PIPELINE_CONFIG_TOTAL = "agent_console_pipeline_config_total";
const std::string METRIC_AGENT_CPU = "agent_cpu_percent";
const std::string METRIC_AGENT_CPU_GO = "agent_go_cpu_percent";
const std::string METRIC_AGENT_CRD_PIPELINE_CONFIG_TOTAL = "agent_crd_pipeline_config_total";
const std::string METRIC_AGENT_ENV_PIPELINE_CONFIG_TOTAL = "agent_env_pipeline_config_total";
const std::string METRIC_AGENT_GO_ROUTINES_TOTAL = "agent_go_routines_total";
const std::string METRIC_AGENT_INSTANCE_CONFIG_TOTAL = "agent_instance_config_total";
const std::string METRIC_AGENT_MEMORY = "agent_memory_used_mb";
const std::string METRIC_AGENT_MEMORY_GO = "agent_go_memory_used_mb";
const std::string METRIC_AGENT_OPEN_FD_TOTAL = "agent_open_fd_total";
const std::string METRIC_AGENT_PIPELINE_CONFIG_TOTAL = "agent_pipeline_config_total";
const std::string METRIC_AGENT_PLUGIN_TOTAL = "agent_plugin_total";

}