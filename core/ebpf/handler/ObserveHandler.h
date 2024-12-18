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

#pragma once

#include <vector>
#include <unordered_map>
#include <set>

#include "ebpf/handler/AbstractHandler.h"
#include "ebpf/include/export.h"
#include "ebpf/config.h"

namespace logtail {
namespace ebpf {

class MeterHandler : public AbstractHandler {
public:
    MeterHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : AbstractHandler(ctx, key, idx) {}

    virtual void handle(const std::vector<std::unique_ptr<ApplicationBatchMeasure>>&, uint64_t) = 0;
};

class OtelMeterHandler : public MeterHandler {
public:
    OtelMeterHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : MeterHandler(ctx, key, idx) {}
    void handle(const std::vector<std::unique_ptr<ApplicationBatchMeasure>>& measures, uint64_t tsSec) override;
};

class SpanHandler : public AbstractHandler {
public:
    SpanHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : AbstractHandler(ctx, key, idx) {}
    virtual void handle(const std::vector<std::unique_ptr<ApplicationBatchSpan>>&) = 0;
};

class OtelSpanHandler : public SpanHandler {
public:
    OtelSpanHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : SpanHandler(ctx, key, idx) {}
    void handle(const std::vector<std::unique_ptr<ApplicationBatchSpan>>&) override;
};

class EventHandler : public AbstractHandler {
public:
    EventHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : AbstractHandler(ctx, key, idx) {}
    void handle(const std::vector<std::unique_ptr<ApplicationBatchEvent>>&);
};


class SimplePodInfo {
public:
    SimplePodInfo(uint64_t startTime, 
        const std::string& appId, 
        const std::string& appName, 
        const std::string& podIp, 
        const std::string& podName, 
        std::vector<std::string>& cids) : mStartTime(startTime), mAppId(appId), mAppName(appName), mPodIp(podIp), mPodName(podName) {}

    uint64_t mStartTime;
    std::string mAppId;
    std::string mAppName;
    std::string mPodIp;
    std::string mPodName;
    std::vector<std::string> mContainerIds;
};

class HostMetadataHandler : public AbstractHandler {
public:
    using UpdatePluginCallbackFunc = std::function<bool(nami::PluginType, UpdataType updateType, const std::variant<SecurityOptions*, nami::ObserverNetworkOption*>)>;
    HostMetadataHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx, int intervalSec = 60);
    ~HostMetadataHandler();
    void RegisterUpdatePluginCallback(UpdatePluginCallbackFunc&& fn) { mUpdateFunc = fn; }
    void DegisterUpdatePluginCallback() { mUpdateFunc = nullptr; }
    bool handle(uint32_t pluginIndex, std::vector<std::string>& podIpVec);
    void ReportAgentInfo();
private:
    // key is podIp, value is cids
    std::unordered_map<std::string, std::unique_ptr<SimplePodInfo>> mHostPods;
    std::unordered_set<std::string> mCids;
    std::thread mReporter;
    std::atomic_bool mFlag;
    ReadWriteLock mLock;
    int mIntervalSec;
    UpdatePluginCallbackFunc mUpdateFunc = nullptr;
};

#ifdef __ENTERPRISE__

class ArmsMeterHandler : public MeterHandler {
public:
    ArmsMeterHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : MeterHandler(ctx, key, idx) {}
    void handle(const std::vector<std::unique_ptr<ApplicationBatchMeasure>>& measures, uint64_t tsSec) override;
};

class ArmsSpanHandler : public SpanHandler {
public:
    ArmsSpanHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx) : SpanHandler(ctx, key, idx) {}
    void handle(const std::vector<std::unique_ptr<ApplicationBatchSpan>>&) override;
};

#endif

}
}
