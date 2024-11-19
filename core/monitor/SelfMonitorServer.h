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
#include <string>
#include <map>

#include "Pipeline.h"

namespace logtail {

struct SelfMonitorMetricRule {
    enum class SelfMonitorMetricRuleTarget {
        LOCAL_FILE,
        SLS_STATUS,
        SLS_SHENNONG
    };
    SelfMonitorMetricRuleTarget mTarget;
    int mInterval;
};

struct SelfMonitorMetricRules {
    SelfMonitorMetricRule mAgentMetricsRule;
    SelfMonitorMetricRule mPipelineMetricsRule;
    SelfMonitorMetricRule mFileCollectMetricsRule;
    SelfMonitorMetricRule mPluginMetricsRule;
    SelfMonitorMetricRule mComponentMetricsRule;
};

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

    void SendMetrics();
    void SendAlarms();

    std::future<void> mThreadRes;
    std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    std::condition_variable mStopCV;

    std::unordered_map<std::string, SelfMonitorMetricRules*> mMetricPipelines;
    std::mutex mMetricPipelineMux;
    PipelineContext* mAlarmPipelineCtx;
    std::mutex mAlarmPipelineMux;
};

} // namespace logtail