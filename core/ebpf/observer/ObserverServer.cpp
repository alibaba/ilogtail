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

#include "ebpf/observer/ObserverServer.h"


namespace logtail {

// 负责接收ebpf返回的数据，然后将数据推送到对应的队列中
// TODO: 目前暂时没有考虑并发Start的问题
void ObserverServer::Start() {
    if (mIsRunning) {
        return;
    } else {
        mIsRunning = false;
        // TODO: 创建一个线程，用于接收ebpf返回的数据，并将数据推送到对应的队列中
        LOG_INFO(sLogger, ("observer ebpf server", "started"));
    }
}

void ObserverServer::Stop() {
    // TODO: ebpf_stop(); 停止所有类型的ebpf探针
    mIsRunning = false;
}

// 插件配置注册逻辑
// 负责启动对应的ebpf程序
void ObserverServer::AddObserverOptions(const std::string& name,
                                        size_t index,
                                        const ObserverOptions* options,
                                        const PipelineContext* ctx) {
    std::string key = name + "#" + std::to_string(index);
    mInputConfigMap[key] = std::make_pair(options, ctx);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (options->mType) {
        case ObserverType::FILE: {
            // TODO: ebpf_start(type);
            break;
        }
        case ObserverType::PROCESS: {
            // TODO: ebpf_start(type);
            break;
        }
        case ObserverType::NETWORK: {
            // TODO: ebpf_start(type);
            break;
        }
        default:
            break;
    }
}

// 插件配置注销逻辑
// TODO: 目前处理配置变更，先stop掉该类型的探针，然后在map里remove配置
void ObserverServer::RemoveObserverOptions(const std::string& name, size_t index) {
    std::string key = name + "#" + std::to_string(index);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (mInputConfigMap[key].first->mType) {
        case ObserverType::FILE: {
            // TODO: ebpf_stop(type);
            break;
        }
        case ObserverType::PROCESS: {
            // TODO: ebpf_stop(type);
            break;
        }
        case ObserverType::NETWORK: {
            // TODO: ebpf_stop(type);
            break;
        }
        default:
            break;
    }
    mInputConfigMap.erase(key);
}

} // namespace logtail
