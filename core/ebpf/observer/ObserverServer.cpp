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

void ObserverServer::Start() {
    // observer ebpf采集线程启动相关操作，包含ebpf_init启动

    // static bool initialized = false;
    // if (!initialized) {
    //     initialized = true;
    //     // init global funcs
    //     ebpf_init();
    //     for (auto& input : mInputMap) {
    //         // TODO: 启动一个线程
    //         ebpf_init(input);
    //     }
    //     ebpf_start();
    // } else {
    //     ebpf_stop();
    //     // init global funcs
    //     ebpf_init();
    //     for (auto& input : mInputMap) {
    //         // TODO: 启动一个线程
    //         ebpf_init(input);
    //     }
    //     ebpf_start();
    // }
    LOG_INFO(sLogger, ("observer ebpf server", "started"));
}

void ObserverServer::Stop() {
    // todo 暂停特定类型的ebpf的 security采集线程，包含ebpf_stop
    // ebpf_stop();
}

void ObserverServer::AddObserverOption(const std::string& name, const ObserverOptions* option) {
    mInputMap[name] = option;
}
void ObserverServer::RemoveObserverOption(const std::string& name) {
    mInputMap.erase(name);
}

} // namespace logtail