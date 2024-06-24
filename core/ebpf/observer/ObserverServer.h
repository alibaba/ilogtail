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
#include <mutex>
#include <thread>

#include "ebpf/observer/ObserverOptions.h"
#include "pipeline/PipelineContext.h"
#include "ebpf/SourceManager.h"
#include "ebpf/include/SockettraceApi.h"

namespace logtail {

enum class BPFObserverPipelineType {
    UNKNOWN,
    PIPELINE_PROCESS,
    PIPELINE_NETWORK,
    PIPELINE_FILE,
    MAX,
};

class ObserverServer {
public:
    ObserverServer(const ObserverServer&) = delete;
    ObserverServer& operator=(const ObserverServer&) = delete;

    static ObserverServer* GetInstance() {
        static ObserverServer instance;
        return &instance;
    }

    void Start(BPFObserverPipelineType);
    void Stop(BPFObserverPipelineType);
    void Stop();

    // 其他函数注册：配置注册、注销等
    void AddObserverOptions(const std::string& name,
                            size_t index,
                            const ObserverOptions* options,
                            const PipelineContext* ctx);
    void RemoveObserverOptions(const std::string& name, size_t index);

    void HandleMeasures(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp);
    void HandleSpans(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans);

private:
    ObserverServer() = default;
    ~ObserverServer() = default;

    void Init();
    void InitBPF();
    void StopBPF();
    void CollectEvents();


    bool mIsRunning = false;
    std::atomic_int ref_;
    // TODO: 目前配置更新时，会停止ebpf探针、重新加载配置、重新启动ebpf探针，后续优化时需要考虑这里的并发问题
    std::unordered_map<std::string, ObserverConfig> mInputConfigMap;

    ObserverConfig network_config_;
    ObserverConfig process_config_;
    ObserverConfig file_config_;
};

} // namespace logtail
