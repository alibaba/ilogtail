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

#include "ebpf/security/SecurityOptions.h"
#include "pipeline/PipelineContext.h"

namespace logtail {

class SecurityServer {
public:
    SecurityServer(const SecurityServer&) = delete;
    SecurityServer& operator=(const SecurityServer&) = delete;

    static SecurityServer* GetInstance() {
        static SecurityServer instance;
        return &instance;
    }

    void Start();
    void Stop();

    // 其他函数注册：配置注册、注销等
    void AddSecurityOptions(const PipelineContext* context, size_t index, SecurityOptions* options);
    void RemoveSecurityOptions(const std::string& name, size_t index);

private:
    SecurityServer() = default;
    ~SecurityServer() = default;

    bool mStop;
    std::unordered_map<std::string, const SecurityOptions*> mInputMap;
    std::unordered_map<std::string, const PipelineContext*> mInputContextMap;
    // std::unordered_map<pair<std::string, size_t>, const pointer*> mEbpfPointerMap;
};

} // namespace logtail