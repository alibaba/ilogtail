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

#include "config_manager/ConfigManager.h"


namespace logtail {

void SecurityServer::Start() {
    // todo security ebpf采集线程启动相关操作，包含ebpf_init启动
    LOG_INFO(sLogger, ("security ebpf server", "started"));
}

void SecurityServer::Stop(bool isConfigUpdate) {
    // todo 暂停特定类型的ebpf的 security采集线程，包含ebpf_stop
}

// todo function1 插件配置注册逻辑

// todo function2 插件配置注销逻辑
}