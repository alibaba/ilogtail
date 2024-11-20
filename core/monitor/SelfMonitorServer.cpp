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

#include "Monitor.h"
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
    if (mMetricEventMaps.find(ctx->GetConfigName()) == mMetricEventMaps.end()) {
        mMetricEventMaps[ctx->GetConfigName()] = SelfMonitorMetricEventMap();
    }
}

void SelfMonitorServer::RemoveMetricPipeline(PipelineContext* ctx) {
    lock_guard<mutex> lock(mMetricPipelineMux);
    if (ctx->GetConfigName() != INNER_METRIC_PIPELINE_NAME) {
        mMetricPipelines.erase(ctx->GetConfigName());
        mMetricEventMaps.erase(ctx->GetConfigName());
    }
}

void SelfMonitorServer::SendMetrics() {
    LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "start"));
    ReadMetrics::GetInstance()->UpdateMetrics();
    // new pipeline
    vector<SelfMonitorMetricEvent> metricEventList;
    ReadMetrics::GetInstance()->ReadAsMetricEvents(metricEventList);
    PushMetricEventIntoMetricEventMap(metricEventList);

    map<string, map<string, PipelineEventGroup> > pipelineEventGroupMap;
    ReadPipelineEventGroupsFromMetricEventMap(pipelineEventGroupMap);

    for (auto configPipelineGroup = pipelineEventGroupMap.begin(); configPipelineGroup != pipelineEventGroupMap.end();
         configPipelineGroup++) {
        shared_ptr<Pipeline> pipeline = PipelineManager::GetInstance()->FindConfigByName(configPipelineGroup->first);
        if (pipeline != nullptr) {
            for (auto pipelineEventGroup = configPipelineGroup->second.begin();
                 pipelineEventGroup != configPipelineGroup->second.end();
                 pipelineEventGroup++) {
                if (pipelineEventGroup->second.GetEvents().size() > 0) {
                    ProcessorRunner::GetInstance()->PushQueue(
                        pipeline->GetContext().GetProcessQueueKey(), 0, std::move(pipelineEventGroup->second));
                }
            }
        }
    }
    LOG_INFO(sLogger, ("self-monitor send self-monitor metrics", "success"));
}

void SelfMonitorServer::PushMetricEventIntoMetricEventMap(std::vector<SelfMonitorMetricEvent>& events) {
    lock_guard<mutex> lock(mMetricPipelineMux);

    for (auto pipelineRule = mMetricPipelines.begin(); pipelineRule != mMetricPipelines.end(); pipelineRule++) {
        string pipelineName = pipelineRule->first;
        auto rules = mMetricPipelines[pipelineName];
        for (auto event : events) {
            auto eventCopy = event;
            if (mMetricEventMaps.find(pipelineName) != mMetricEventMaps.end()) {
                SelfMonitorMetricEventMap& metricEventMap = mMetricEventMaps[pipelineName];
                if (eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_AGENT
                    || eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_RUNNER) {
                    if (!rules->mAgentMetricsRule.mEnable) {
                        if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                            metricEventMap.erase(eventCopy.mKey);
                        }
                        continue;
                    };
                    eventCopy.SetTarget(rules->mAgentMetricsRule.mTarget);
                    eventCopy.SetInterval(rules->mAgentMetricsRule.mInterval);
                }
                if (eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_COMPONENT) {
                    if (!rules->mComponentMetricsRule.mEnable) {
                        if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                            metricEventMap.erase(eventCopy.mKey);
                        }
                        continue;
                    };
                    eventCopy.SetTarget(rules->mComponentMetricsRule.mTarget);
                    eventCopy.SetInterval(rules->mComponentMetricsRule.mInterval);
                }
                if (eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_PIPELINE) {
                    if (!rules->mPipelineMetricsRule.mEnable) {
                        if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                            metricEventMap.erase(eventCopy.mKey);
                        }
                        continue;
                    };
                    eventCopy.SetTarget(rules->mPipelineMetricsRule.mTarget);
                    eventCopy.SetInterval(rules->mPipelineMetricsRule.mInterval);
                }
                if (eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_PLUGIN) {
                    if (!rules->mPluginMetricsRule.mEnable) {
                        if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                            metricEventMap.erase(eventCopy.mKey);
                        }
                        continue;
                    };
                    eventCopy.SetTarget(rules->mPluginMetricsRule.mTarget);
                    eventCopy.SetInterval(rules->mPluginMetricsRule.mInterval);
                }
                if (eventCopy.mCategory == MetricCategory::METRIC_CATEGORY_PLUGIN_SOURCE) {
                    if (!rules->mFileCollectMetricsRule.mEnable) {
                        if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                            metricEventMap.erase(eventCopy.mKey);
                        }
                        continue;
                    };
                    eventCopy.SetTarget(rules->mFileCollectMetricsRule.mTarget);
                    eventCopy.SetInterval(rules->mFileCollectMetricsRule.mInterval);
                }
                if (metricEventMap.find(eventCopy.mKey) != metricEventMap.end()) {
                    metricEventMap[eventCopy.mKey].Merge(eventCopy);
                } else {
                    metricEventMap[eventCopy.mKey] = std::move(eventCopy);
                }
            }
        }
    }
}

void SelfMonitorServer::ReadPipelineEventGroupsFromMetricEventMap(
    map<string, map<string, PipelineEventGroup> >& pipelineEventGroupMap) {
    lock_guard<mutex> lock(mMetricPipelineMux);
    for (auto configMetricEventMap = mMetricEventMaps.begin(); configMetricEventMap != mMetricEventMaps.end();
         configMetricEventMap++) {
        string pipelineName = configMetricEventMap->first;
        if (pipelineEventGroupMap.find(pipelineName) == pipelineEventGroupMap.end()) {
            pipelineEventGroupMap[pipelineName] = map<string, PipelineEventGroup>();
        }
        for (auto metricEvent = configMetricEventMap->second.begin(); metricEvent != configMetricEventMap->second.end();
             metricEvent++) {
            if (metricEvent->second.ShouldSend()) {
                metricEvent->second.Collect();
                const set<string>& targets = metricEvent->second.GetTargets();
                for (auto target : targets) {
                    LogEvent* logEventPtr;
                    map<string, PipelineEventGroup>::iterator iter = pipelineEventGroupMap[pipelineName].find(target);
                    if (iter != pipelineEventGroupMap[pipelineName].end()) {
                        logEventPtr = iter->second.AddLogEvent();
                    } else {
                        PipelineEventGroup pipelineEventGroup(make_shared<SourceBuffer>());
                        pipelineEventGroup.SetTag("__target__", target);
                        pipelineEventGroup.SetTagNoCopy(LOG_RESERVED_KEY_SOURCE, LoongCollectorMonitor::mIpAddr);
                        pipelineEventGroup.SetTagNoCopy(LOG_RESERVED_KEY_TOPIC, METRIC_TOPIC_TYPE);
                        logEventPtr = pipelineEventGroup.AddLogEvent();
                        pipelineEventGroupMap[pipelineName].insert(
                            pair<string, PipelineEventGroup>(target, move(pipelineEventGroup)));
                    }
                    metricEvent->second.ReadAsLogEvent(logEventPtr);
                }
            }
        }
    }
}

void SelfMonitorServer::UpdateAlarmPipeline(PipelineContext* ctx) {
    lock_guard<mutex> lock(mAlarmPipelineMux);
    mAlarmPipelineCtx = ctx;
}

void SelfMonitorServer::SendAlarms() {
}

} // namespace logtail