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
#include <array>
#include <atomic>

#include "ebpf/include/export.h"
#include "monitor/PluginMetricManager.h"
#include "common/Lock.h"
#include "monitor/LoongCollectorMetricTypes.h"
#include "monitor/metric_constants/MetricConstants.h"

namespace logtail {
namespace ebpf {

class BaseBPFMonitor {
public:
    virtual void HandleStatistic(nami::eBPFStatistics& stats);
    virtual void InitMetric();
    nami::eBPFStatistics GetLastStats() const;
    virtual void ReleaseMetric();
    virtual ~BaseBPFMonitor() = default;
protected:
    BaseBPFMonitor(const std::string& name, PluginMetricManagerPtr mgr, MetricsRecordRef& ref, const nami::eBPFStatistics& lastStats) 
        : mPipelineName(name), mPluginMetricMgr(mgr), mRef(ref), mLastStat(lastStats) {}

    std::string PluginTypeToString(const nami::PluginType pluginType);

    // attention: not thread safe!!
    void InitInnerMetric();

    // attention: not thread safe!!
    void UpdateInnerMetric(nami::eBPFStatistics& currStat);

    mutable ReadWriteLock mMonitorMtx;
    std::string mPipelineName;
    PluginMetricManagerPtr mPluginMetricMgr;
    mutable std::mutex mStatsMtx;
    MetricsRecordRef& mRef;
    std::vector<std::pair<ReentrantMetricsRecordRef, MetricLabels>> mRefAndLabels;

    nami::eBPFStatistics mLastStat;
    std::atomic_bool mMetricInited = false;

    CounterPtr mRecvKernelEventsTotal;
    CounterPtr mLossKernelEventsTotal;
    CounterPtr mPushEventsTotal;
    CounterPtr mPushSpansTotal;
    CounterPtr mPushMetricsTotal;
    IntGaugePtr mProcessCacheEntitiesNum;
    CounterPtr mProcessCacheMissTotal;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

class NetworkObserverSelfMonitor : public BaseBPFMonitor {
public:
    NetworkObserverSelfMonitor(const std::string& name, PluginMetricManagerPtr mgr, MetricsRecordRef& ref, const nami::eBPFStatistics& lastStats) 
        : BaseBPFMonitor(name, mgr, ref, lastStats) {}

    void InitMetric() override;

    void HandleStatistic(nami::eBPFStatistics& stats) override;

private:

    // recv kernel events metric
    CounterPtr mRecvConnStatsTotal;
    CounterPtr mRecvCtrlEventsTotal;
    CounterPtr mRecvHTTPDataEventsTotal;

    // cache relative metric
    IntGaugePtr mConnTrackerNum;

    // protocol parsing metric
    CounterPtr mParseHTTPEventsSuccessTotal;
    CounterPtr mParseHTTPEventsFailTotal;

    // aggregation relative metric
    IntGaugePtr mAggMapEntitiesNum;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

class NetworkSecuritySelfMonitor : public BaseBPFMonitor {
public:
    NetworkSecuritySelfMonitor(const std::string& name, PluginMetricManagerPtr mgr, MetricsRecordRef& ref, const nami::eBPFStatistics& lastStats) 
        : BaseBPFMonitor(name, mgr, ref, lastStats) {}

    void HandleStatistic(nami::eBPFStatistics& stats) override {
        if (!stats.updated_) return;
        std::lock_guard<std::mutex> lk(mStatsMtx);
        UpdateInnerMetric(stats);
    }
};

class ProcessSecuritySelfMonitor : public BaseBPFMonitor {
public:
    ProcessSecuritySelfMonitor(const std::string& name, PluginMetricManagerPtr mgr, MetricsRecordRef& ref, const nami::eBPFStatistics& lastStats) 
        : BaseBPFMonitor(name, mgr, ref, lastStats) {}
};

class FileSecuritySelfMonitor : public BaseBPFMonitor {
public:
    FileSecuritySelfMonitor(const std::string& name, PluginMetricManagerPtr mgr, MetricsRecordRef& ref, const nami::eBPFStatistics& lastStats) 
        : BaseBPFMonitor(name, mgr, ref, lastStats) {}
};

/**
 * eBPFSelfMonitorMgr is only used to manage the self-monitoring data in libnetwork_observer.so, updating the statistics through callbacks.
 */
class eBPFSelfMonitorMgr {
public:
    eBPFSelfMonitorMgr() : mSelfMonitors({}), mInited({}) {}
    void Init(const nami::PluginType type, MetricsRecordRef& ref, const std::string& name, const std::string& logstore);
    void Release(const nami::PluginType type);
    void Suspend(const nami::PluginType type);
    void HandleStatistic(std::vector<nami::eBPFStatistics>&& stats);
private:
    std::array<std::unique_ptr<BaseBPFMonitor>, int(nami::PluginType::MAX)> mSelfMonitors;
    std::array<std::atomic_bool, int(nami::PluginType::MAX)> mInited;
    
#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

} // ebpf
} // logtail