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

#include "ebpf/security/SecurityServer.h"


namespace logtail {

// 负责接收ebpf返回的数据，然后将数据推送到对应的队列中
void SecurityServer::Start() {
    static bool initialized = false;
    if (initialized) {
        return;
    } else {
        mStop = false;
        initialized = true;
        // TODO: 创建一个线程，用于接收ebpf返回的数据，并将数据推送到对应的队列中
        LOG_INFO(sLogger, ("security ebpf server", "started"));
    }
}

void SecurityServer::Stop() {
    // TODO: ebpf_stop(); 停止所有类型的ebpf探针
    mStop = true;
}

// 插件配置注册逻辑
// 负责启动对应的ebpf程序
void SecurityServer::AddSecurityOptions(const PipelineContext* context, size_t index, SecurityOptions* options) {
    std::string key = context->GetConfigName() + "#" + std::to_string(index);
    mInputMap[key] = options;
    mInputContextMap[key] = context;
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (options->mFilterType) {
        case SecurityFilterType::FILE: {
            // TODO: ebpf_start(type);
            break;
        }
        case SecurityFilterType::PROCESS: {
            // TODO: ebpf_start(type);
            break;
        }
        case SecurityFilterType::NETWORK: {
            // TODO: ebpf_start(type);
            break;
        }
        default:
            break;
    }
}
// 插件配置注销逻辑
void SecurityServer::RemoveSecurityOptions(const std::string& name, size_t index) {
    std::string key = name + "#" + std::to_string(index);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (mInputMap[name + "#" + std::to_string(index)]->mFilterType) {
        case SecurityFilterType::FILE: {
            // TODO: ebpf_stop(type);
            break;
        }
        case SecurityFilterType::PROCESS: {
            // TODO: ebpf_stop(type);
            break;
        }
        case SecurityFilterType::NETWORK: {
            // TODO: ebpf_stop(type);
            break;
        }
        default:
            break;
    }
    mInputMap.erase(key);
    mInputContextMap.erase(key);
}

} // namespace logtail