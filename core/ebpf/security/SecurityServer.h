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
    void Stop(bool isConfigUpdate = true);

    // todo 其他函数注册：配置注册、注销等

private:
    SecurityServer() = default;
    ~SecurityServer() = default;

    void PauseInner();
};
}