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

#include "ebpf/security/SecurityOptions.h"

using namespace std;
namespace logtail {

bool SecurityOptions::Init(const Json::Value& config, const PipelineContext& ctx, const string& pluginName) {
    // todo 根据不同mode , 参数解析

    return true;
}

// todo app_config中定义的进程级别配置获取


} // namespace logtail
