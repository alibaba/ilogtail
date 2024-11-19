/*
 * Copyright 2024 iLogtail Authors
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

#include <atomic>
#include <map>
#include <string>

#include "Pipeline.h"

namespace logtail {

class SelfMonitorServer {
public:
    SelfMonitorServer(const SelfMonitorServer&) = delete;
    SelfMonitorServer& operator=(const SelfMonitorServer&) = delete;
    static SelfMonitorServer* GetInstance();

    void Init();
    void Monitor();
    void Stop();

    void UpdateMetricPipeline(PipelineContext* ctx, SelfMonitorMetricRules& rules);
    void RemoveMetricPipeline(PipelineContext* ctx);
    void UpdateAlarmPipeline(PipelineContext* ctx); // Todo
private:
    SelfMonitorServer();
    ~SelfMonitorServer() = default;

    std::future<void> mThreadRes;
    std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    std::condition_variable mStopCV;

    void SendMetrics();
    void PushMetricEventIntoMetricEventMap(std::vector<SelfMonitorMetricEvent>& events);
    void ReadPipelineEventGroupsFromMetricEventMap(std::map<std::string, std::map<std::string, PipelineEventGroup> >& pipelineEventGroupMap);

    std::unordered_map<std::string, SelfMonitorMetricRules*> mMetricPipelines;
    std::unordered_map<std::string, SelfMonitorMetricEventMap> mMetricEventMaps;
    std::mutex mMetricPipelineMux;

    void SendAlarms();

    PipelineContext* mAlarmPipelineCtx;
    std::mutex mAlarmPipelineMux;
};

} // namespace logtail