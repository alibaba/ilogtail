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

#include "ebpf/SelfMonitor.h"
#include "logger/Logger.h"

namespace logtail {
namespace ebpf {

void BaseBPFMonitor::HandleStatistic(nami::eBPFStatistics& stats) {
    if (!stats.updated_) return;
    std::lock_guard<std::mutex> lk(mStatsMtx);
    UpdateInnerMetric(stats);
    mLastStat = stats;
}

nami::eBPFStatistics BaseBPFMonitor::GetLastStats() const {
    std::lock_guard<std::mutex> lk(mStatsMtx);
    return mLastStat;
}

void BaseBPFMonitor::InitMetric() {
    if (mMetricInited) return;
    mMetricInited = true;
    InitInnerMetric();
}

void BaseBPFMonitor::ReleaseMetric() {
    if (!mMetricInited) return;
    for (auto& ref : mRefs) {
        if (!ref) continue;
        auto labels = ref->GetLabels();
        if (mPluginMetricMgr) {
            mPluginMetricMgr->ReleaseReentrantMetricsRecordRef(*labels.get());
        }
    }

    // release default
    if (mPluginMetricMgr) {
        mPluginMetricMgr->ReleaseReentrantMetricsRecordRef(mDefaultLabels);
    }
    mMetricInited = false;
}

std::string BaseBPFMonitor::PluginTypeToString(const nami::PluginType pluginType) {
    switch (pluginType)
    {
    case nami::PluginType::NETWORK_OBSERVE:
        return "network_observer";
        break;
    case nami::PluginType::NETWORK_SECURITY:
        return "network_security";
        break;
    case nami::PluginType::PROCESS_OBSERVE:
        return "process_observer";
        break;
    case nami::PluginType::PROCESS_SECURITY:
        return "process_security";
        break;
    case nami::PluginType::FILE_OBSERVE:
        return "file_observer";
        break;
    case nami::PluginType::FILE_SECURITY:
        return "file_security";
        break;
    default:
        return "";
        break;
    }
}

void BaseBPFMonitor::InitInnerMetric() {
    // init base metrics, only plugin relative
    mReentrantMetricRecordRef = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(mDefaultLabels);
    mRecvKernelEventsTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_RECV_KERNEL_EVENTS_TOTAL);
    mLossKernelEventsTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_LOSS_KERNEL_EVENTS_TOTAL);
    mPushEventsTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_CALLBACK_EVENTS_TOTAL);
    mPushSpansTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_CALLBACK_SPANS_TOTAL);
    mPushMetricsTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_CALLBACK_METRICS_TOTAL);
    mProcessCacheEntitiesNum = mReentrantMetricRecordRef->GetIntGauge(METRIC_EBPF_PROCESS_CACHE_ENTRIES_NUM);
    mProcessCacheMissTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PROCESS_CACHE_MISS_TOTAL);

    mPushLogsToQueueTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_QUEUE_EVENTS_TOTAL);
    mPushTracesToQueueTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_QUEUE_EVENTS_TOTAL);
    mPushMetricsToQueueTotal = mReentrantMetricRecordRef->GetCounter(METRIC_EBPF_PUSH_QUEUE_EVENTS_TOTAL);
}

void BaseBPFMonitor::UpdateInnerMetric(nami::eBPFStatistics& currStat) {
    if (!currStat.updated_) return;
    mProcessCacheEntitiesNum->Set(currStat.process_cache_entities_num_);
    if (currStat.recv_kernel_events_total_ > mLastStat.recv_kernel_events_total_) {
        mRecvKernelEventsTotal->Add(currStat.recv_kernel_events_total_ - mLastStat.recv_kernel_events_total_);
    }
    if (currStat.loss_kernel_events_total_ > mLastStat.loss_kernel_events_total_) {
        mLossKernelEventsTotal->Add(currStat.loss_kernel_events_total_ - mLastStat.loss_kernel_events_total_);
    }
    if (currStat.miss_process_cache_total_ > mLastStat.miss_process_cache_total_) {
        mProcessCacheMissTotal->Add(currStat.miss_process_cache_total_ - mLastStat.miss_process_cache_total_);
    }
    if (currStat.push_metrics_total_ > mLastStat.push_metrics_total_) {
        mPushMetricsTotal->Add(currStat.push_metrics_total_ - mLastStat.push_metrics_total_);
    }
    if (currStat.push_spans_total_ > mLastStat.push_spans_total_) {
        mPushSpansTotal->Add(currStat.push_spans_total_ - mLastStat.push_spans_total_);
    }
    if (currStat.push_events_total_ > mLastStat.push_events_total_) {
        mPushEventsTotal->Add(currStat.push_events_total_ - mLastStat.push_events_total_);
    }
}

/////////////////////////// NetworkObserverSelfMonitor ///////////////////////////
void NetworkObserverSelfMonitor::InitMetric() {
    if (mMetricInited) return;
    mMetricInited = true;

    InitInnerMetric();

    // use default labels ... 
    mConnTrackerNum = mReentrantMetricRecordRef->GetIntGauge(METRIC_EBPF_NETWORK_OBSERVER_CONNTRACKER_NUM);
    mAggMapEntitiesNum = mReentrantMetricRecordRef->GetIntGauge(METRIC_EBPF_NETWORK_OBSERVER_AGGREGATE_KEY_NUM);

    // event type relative labels ... 
    MetricLabels eventTypeLabels = {{"event.type", "conn_stats"}};
    auto ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeLabels);
    mRecvConnStatsTotal = ref->GetCounter(METRIC_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefs.emplace_back(ref);

    eventTypeLabels = {{"event.type", "ctrl_event"}};
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeLabels);
    mRecvCtrlEventsTotal = ref->GetCounter(METRIC_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefs.emplace_back(ref);

    // 
    MetricLabels eventTypeAndProtocolLbales = {{"event.type", "data_event"}, {"protocol", "http"}};
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeAndProtocolLbales);
    mRecvHTTPDataEventsTotal = ref->GetCounter(METRIC_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefs.emplace_back(ref);

    // protocol relative labels ...
    MetricLabels httpSuccessLabels = {{{"event.type", "data_event"}, {"protocol", "http"}, {"status", "success"}}};
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(httpSuccessLabels);
    mParseHTTPEventsSuccessTotal = ref->GetCounter(METRIC_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL);
    mRefs.emplace_back(ref);

    MetricLabels httpFailLabels = {{{"event.type", "data_event"}, {"protocol", "http"}, {"status", "failed"}}};
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(httpFailLabels);
    mParseHTTPEventsFailTotal = ref->GetCounter(METRIC_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL);
    mRefs.emplace_back(ref);
}

void NetworkObserverSelfMonitor::HandleStatistic(nami::eBPFStatistics& stats) {
    if (!stats.updated_) return;
    std::lock_guard<std::mutex> lk(mStatsMtx);
    UpdateInnerMetric(stats);
    // recv kernel events metric
    assert(stats.plugin_type_ == nami::PluginType::NETWORK_OBSERVE);

    nami::NetworkObserverStatistics* currNetworkStatsPtr = dynamic_cast<nami::NetworkObserverStatistics*>(&stats);
    nami::NetworkObserverStatistics* lastNetworkStatsPtr = dynamic_cast<nami::NetworkObserverStatistics*>(&mLastStat);
    nami::NetworkObserverStatistics currNetworkStats, lastNetworkStats;
    if (lastNetworkStatsPtr) {
        lastNetworkStats = *lastNetworkStatsPtr;
    }
    if (currNetworkStatsPtr) currNetworkStats = *currNetworkStatsPtr;
    // update conn tracker num
    if (mRecvConnStatsTotal && (currNetworkStats.recv_conn_stat_events_total_ > lastNetworkStats.recv_conn_stat_events_total_)) {
        mRecvConnStatsTotal->Add(currNetworkStats.recv_conn_stat_events_total_ - lastNetworkStats.recv_conn_stat_events_total_);
    }
    if (mRecvCtrlEventsTotal && (currNetworkStats.recv_ctrl_events_total_ > lastNetworkStats.recv_ctrl_events_total_)) {
        mRecvCtrlEventsTotal->Add(currNetworkStats.recv_ctrl_events_total_ - lastNetworkStats.recv_ctrl_events_total_);
    }
    if (mRecvHTTPDataEventsTotal && (currNetworkStats.recv_http_data_events_total_ > lastNetworkStats.recv_http_data_events_total_)) {
        mRecvHTTPDataEventsTotal->Add(currNetworkStats.recv_http_data_events_total_ - lastNetworkStats.recv_http_data_events_total_);
    }

    // cache relative metric
    mConnTrackerNum->Set(currNetworkStats.conntracker_num_);

    // protocol parsing metric
    if (currNetworkStats.parse_http_records_success_total_ > lastNetworkStats.parse_http_records_success_total_) {
        mParseHTTPEventsSuccessTotal->Add(currNetworkStats.parse_http_records_success_total_ - lastNetworkStats.parse_http_records_success_total_);
    }
    if (currNetworkStats.parse_http_records_failed_total_ > lastNetworkStats.parse_http_records_failed_total_) {
        mParseHTTPEventsFailTotal->Add(currNetworkStats.parse_http_records_failed_total_ - lastNetworkStats.parse_http_records_failed_total_);
    }

    // aggregation relative metric
    mAggMapEntitiesNum->Set(currNetworkStats.agg_map_entities_num_);

    // update
    mLastStat = stats;
}

void eBPFSelfMonitorMgr::Init(const nami::PluginType type, const std::string& name, const MetricLabelsPtr& labels) {
    if (mInited[int(type)]) {
        return;
    }

    static const std::unordered_map<std::string, MetricType> metricKeys = {
        {METRIC_EBPF_RECV_KERNEL_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_LOSS_KERNEL_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_PUSH_CALLBACK_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_PUSH_CALLBACK_SPANS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_PUSH_CALLBACK_METRICS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_PROCESS_CACHE_ENTRIES_NUM, MetricType::METRIC_TYPE_INT_GAUGE},
        {METRIC_EBPF_PROCESS_CACHE_MISS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_EBPF_PUSH_QUEUE_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };

    nami::eBPFStatistics lastStats;
    if (mSelfMonitors[int(type)]) {
        lastStats = mSelfMonitors[int(type)]->GetLastStats();
    }

    switch (type)
    {
    case nami::PluginType::NETWORK_OBSERVE: {
        std::unordered_map<std::string, MetricType> keys = metricKeys;
        keys.insert({METRIC_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER});
        keys.insert({METRIC_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL, MetricType::METRIC_TYPE_COUNTER});
        keys.insert({METRIC_EBPF_NETWORK_OBSERVER_AGGREGATE_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER});
        keys.insert({METRIC_EBPF_NETWORK_OBSERVER_AGGREGATE_KEY_NUM, MetricType::METRIC_TYPE_INT_GAUGE});
        keys.insert({METRIC_EBPF_NETWORK_OBSERVER_CONNTRACKER_NUM, MetricType::METRIC_TYPE_INT_GAUGE});
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labels, keys);
        mSelfMonitors[int(type)] = std::make_unique<NetworkObserverSelfMonitor>(name, pluginMetricManager, labels, lastStats);
        break;
    }
    case nami::PluginType::NETWORK_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labels, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<NetworkSecuritySelfMonitor>(name, pluginMetricManager, labels, lastStats);
        break;
    }
    case nami::PluginType::FILE_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labels, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<FileSecuritySelfMonitor>(name, pluginMetricManager, labels, lastStats);
        break;
    }
    case nami::PluginType::PROCESS_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labels, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<ProcessSecuritySelfMonitor>(name, pluginMetricManager, labels, lastStats);
        break;
    }
    default:
        break;
    }
    mSelfMonitors[int(type)]->InitMetric();
    mInited[int(type)] = true;
}

void eBPFSelfMonitorMgr::Release(const nami::PluginType type) {
    if (!mInited[int(type)]) {
        return;
    }
    if (mSelfMonitors[int(type)]) {
        mSelfMonitors[int(type)]->ReleaseMetric();
        mSelfMonitors[int(type)].reset();
    }
    mInited[int(type)] = false;
}

void eBPFSelfMonitorMgr::Suspend(const nami::PluginType type) {
    if (!mInited[int(type)]) {
        return;
    }
    mInited[int(type)] = false;
}

void eBPFSelfMonitorMgr::HandleStatistic(std::vector<nami::eBPFStatistics>&& stats) {
    for (auto& stat : stats) {
        if (!stat.updated_) {
            continue;
        }
        auto type = stat.plugin_type_;
        if (mInited[int(type)] && mSelfMonitors[int(type)]) {
            mSelfMonitors[int(type)]->HandleStatistic(stat);
        }
    }
}

}
}
