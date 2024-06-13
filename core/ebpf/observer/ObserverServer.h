/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string>
#include <unordered_map>
#include <utility>

#include "ebpf/observer/ObserverOptions.h"
#include "pipeline/PipelineContext.h"

namespace logtail {

class ObserverServer {
public:
    ObserverServer(const ObserverServer&) = delete;
    ObserverServer& operator=(const ObserverServer&) = delete;

    static ObserverServer* GetInstance() {
        static ObserverServer instance;
        return &instance;
    }

    void Start();
    void Stop();

    // 其他函数注册：配置注册、注销等
    void AddObserverOption(const PipelineContext* context, size_t index, const ObserverOptions* option);
    void RemoveObserverOption(const std::string& name, size_t index);


private:
    ObserverServer() = default;
    ~ObserverServer() = default;

    bool mStop;
    std::unordered_map<std::string, const ObserverOptions*> mInputMap;
    std::unordered_map<std::string, const PipelineContext*> mInputContextMap;
};

} // namespace logtail