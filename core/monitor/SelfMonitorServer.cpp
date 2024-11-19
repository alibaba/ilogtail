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

#include "SelfMonitorServer.h"

#include "MetricExportor.h"
#include "PipelineManager.h"
#include "common/LogtailCommonFlags.h"
#include "runner/ProcessorRunner.h"

using namespace std;

namespace logtail {

SelfMonitorServer::SelfMonitorServer() {
}

SelfMonitorServer* SelfMonitorServer::GetInstance() {
    static SelfMonitorServer* ptr = new SelfMonitorServer();
    return ptr;
}

void SelfMonitorServer::Init() {
    mThreadRes = async(launch::async, &SelfMonitorServer::Monitor, this);
}

void SelfMonitorServer::Monitor() {
    LOG_INFO(sLogger, ("self-monitor", "started"));
    int32_t lastMonitorTime = time(NULL);
    {
        unique_lock<mutex> lock(mThreadRunningMux);
        while (mIsThreadRunning) {
            if (mStopCV.wait_for(lock, std::chrono::seconds(1), [this]() { return !mIsThreadRunning; })) {
                break;
            }
            int32_t monitorTime = time(NULL);
            if ((monitorTime - lastMonitorTime) >= INT32_FLAG(monitor_interval) * 2) { // 60s
                lastMonitorTime = monitorTime;
                SendMetrics();
                SendAlarms();
            }
        }
    }
    SendMetrics();
    SendAlarms();
}

void SelfMonitorServer::Stop() {
    {
        lock_guard<mutex> lock(mThreadRunningMux);
        mIsThreadRunning = false;
    }
    mStopCV.notify_one();
    future_status s = mThreadRes.wait_for(chrono::seconds(1));
    if (s == future_status::ready) {
        LOG_INFO(sLogger, ("self-monitor", "stopped successfully"));
    } else {
        LOG_WARNING(sLogger, ("self-monitor", "forced to stopped"));
    }
}

void SelfMonitorServer::UpdateMetricPipeline(PipelineContext* ctx, SelfMonitorMetricRules& rules) {
    lock_guard<mutex> lock(mMetricPipelineMux);
    mMetricPipelines[ctx->GetConfigName()] = &rules;
}

void SelfMonitorServer::RemoveMetricPipeline(PipelineContext* ctx) {
    lock_guard<mutex> lock(mMetricPipelineMux);
    if (ctx->GetConfigName() != INNER_METRIC_PIPELINE_NAME) {
        mMetricPipelines.erase(ctx->GetConfigName());
    }
}

void SelfMonitorServer::UpdateAlarmPipeline(PipelineContext* ctx) {
    lock_guard<mutex> lock(mAlarmPipelineMux);
    mAlarmPipelineCtx = ctx;
}

void SelfMonitorServer::SendMetrics() {
    LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "start"));
    ReadMetrics::GetInstance()->UpdateMetrics();

    map<string, PipelineEventGroup> pipelineEventGroupMap;
    // Todo: delete MetricExportor
    MetricExportor::GetInstance()->PushMetrics();
    ReadMetrics::GetInstance()->ReadAsPipelineEventGroup(pipelineEventGroupMap);

    lock_guard<mutex> lock(mMetricPipelineMux);
    for (auto pipelineInfo = mMetricPipelines.begin(); pipelineInfo != mMetricPipelines.end(); pipelineInfo++) {
        LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "prepare")("pipeline", pipelineInfo->first));
        shared_ptr<Pipeline> pipeline = PipelineManager::GetInstance()->FindConfigByName(pipelineInfo->first);
        if (pipeline != nullptr) {
            LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "prepare")("pipeline get success", pipeline->GetContext().GetConfigName()));
            for (auto it = pipelineEventGroupMap.begin(); it != pipelineEventGroupMap.end(); it++) {
                LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "sending")("region", it->first)("pipeline", pipeline->GetContext().GetConfigName()));
                ProcessorRunner::GetInstance()->PushQueue(
                    pipeline->GetContext().GetProcessQueueKey(), 0, it->second.Copy());
            }
        }
    }
    LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "success"));
}

void SelfMonitorServer::SendAlarms() {
    LOG_INFO(sLogger, ("self-monitor", "send self-monitor alarms"));
}

} // namespace logtail