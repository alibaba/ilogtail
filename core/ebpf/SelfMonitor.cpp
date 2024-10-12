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
    UpdateInnerMetric(stats);
}

void BaseBPFMonitor::InitMetric() {
    if (mMetricInited) return;
    mMetricInited = true;
    InitInnerMetric();
}

void BaseBPFMonitor::ReleaseMetric() {
    if (!mMetricInited) return;
    for (auto& item : mRefAndLabels) {
        auto labels = item.second;
        if (mPluginMetricMgr) {
            mPluginMetricMgr->ReleaseReentrantMetricsRecordRef(labels);
        }
    }

    mMetricInited = false;
}

std::string BaseBPFMonitor::PluginTypeToString(const nami::PluginType pluginType) {
    switch (pluginType)
    {
    case nami::PluginType::NETWORK_OBSERVE:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_NETWORK_OBSERVER;
        break;
    case nami::PluginType::NETWORK_SECURITY:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_NETWORK_SECURITY;
        break;
    case nami::PluginType::PROCESS_OBSERVE:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_PROCESS_OBSERVER;
        break;
    case nami::PluginType::PROCESS_SECURITY:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_PROCESS_SECURITY;
        break;
    case nami::PluginType::FILE_OBSERVE:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_FILE_OBSERVER;
        break;
    case nami::PluginType::FILE_SECURITY:
        return METRIC_LABEL_VALUE_RUNNER_PLUGIN_TYPE_FILE_SECURITY;
        break;
    default:
        return "";
        break;
    }
}

void BaseBPFMonitor::InitInnerMetric() {
    // init base metrics, only plugin relative
    // poll kernel events
    MetricLabels pollKernelEventsLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_POLL_KERNEL}
    };
    auto ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(pollKernelEventsLabels);
    mRecvKernelEventsTotal = ref->GetCounter(METRIC_RUNNER_IN_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, pollKernelEventsLabels));

    // loss kernel events
    mLossKernelEventsTotal = mRef.CreateCounter(METRIC_RUNNER_EBPF_LOSS_KERNEL_EVENTS_TOTAL);
    mProcessCacheEntitiesNum = mRef.CreateIntGauge(METRIC_RUNNER_EBPF_PROCESS_CACHE_ENTRIES_NUM);
    mProcessCacheMissTotal = mRef.CreateCounter(METRIC_RUNNER_EBPF_PROCESS_CACHE_MISS_TOTAL);

    // push logs/spans/metrics
    MetricLabels pushLogsLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_REPORT_TO_LC},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_LOG}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(pushLogsLabels);
    mPushEventsTotal = ref->GetCounter(METRIC_RUNNER_IN_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, pushLogsLabels));

    MetricLabels pushMetricsLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_REPORT_TO_LC},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_METRIC}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(pushMetricsLabels);
    mPushMetricsTotal = ref->GetCounter(METRIC_RUNNER_IN_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, pushMetricsLabels));

    MetricLabels pushSpansLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_REPORT_TO_LC},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_TRACE}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(pushSpansLabels);
    mPushSpansTotal = ref->GetCounter(METRIC_RUNNER_IN_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, pushSpansLabels));
}

void BaseBPFMonitor::UpdateInnerMetric(nami::eBPFStatistics& currStat) {
    if (!currStat.updated_) return;
    mProcessCacheEntitiesNum->Set(currStat.process_cache_entities_num_);
    mRecvKernelEventsTotal->Add(currStat.recv_kernel_events_total_);
    mLossKernelEventsTotal->Add(currStat.loss_kernel_events_total_);
    mProcessCacheMissTotal->Add(currStat.miss_process_cache_total_);
    mPushMetricsTotal->Add(currStat.push_metrics_total_);
    mPushSpansTotal->Add(currStat.push_spans_total_);
    mPushEventsTotal->Add(currStat.push_events_total_);
}

/////////////////////////// NetworkObserverSelfMonitor ///////////////////////////
void NetworkObserverSelfMonitor::InitMetric() {
    if (mMetricInited) return;
    mMetricInited = true;

    InitInnerMetric();

    // use default labels ... 
    mConnTrackerNum = mRef.CreateIntGauge(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_CONNTRACKER_NUM);
    mAggMapEntitiesNum = mRef.CreateIntGauge(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_AGGREGATE_KEY_NUM);

    // event type relative labels ... 
    MetricLabels eventTypeLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_CONN_STATS}
    };
    auto ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeLabels);
    mRecvConnStatsTotal = ref->GetCounter(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, eventTypeLabels));

    eventTypeLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_CTRL_EVENT}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeLabels);
    mRecvCtrlEventsTotal = ref->GetCounter(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, eventTypeLabels));

    // 
    MetricLabels eventTypeAndProtocolLbales = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_DATA_EVENT},
        {METRIC_LABEL_KEY_RUNNER_PARSER_PROTOCOL, METRIC_LABEL_VALUE_RUNNER_PARSER_PROTOCOL_HTTP},
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(eventTypeAndProtocolLbales);
    mRecvHTTPDataEventsTotal = ref->GetCounter(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, eventTypeAndProtocolLbales));

    // protocol relative labels ...
    MetricLabels httpSuccessLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_DATA_EVENT},
        {METRIC_LABEL_KEY_RUNNER_PARSER_PROTOCOL, METRIC_LABEL_VALUE_RUNNER_PARSER_PROTOCOL_HTTP},
        {METRIC_LABEL_KEY_RUNNER_PARSE_STATUS, METRIC_LABEL_VALUE_RUNNER_PARSE_STATUS_SUCCESS}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(httpSuccessLabels);
    mParseHTTPEventsSuccessTotal = ref->GetCounter(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, httpSuccessLabels));

    MetricLabels httpFailLabels = {
        {METRIC_LABEL_KEY_RUNNER_RECV_EVENT_STAGE, METRIC_LABEL_VALUE_RUNNER_RECV_EVENT_STAGE_AFTER_PERF_WORKER},
        {METRIC_LABEL_KEY_RUNNER_EVENT_TYPE, METRIC_LABEL_VALUE_RUNNER_EVENT_TYPE_DATA_EVENT},
        {METRIC_LABEL_KEY_RUNNER_PARSER_PROTOCOL, METRIC_LABEL_VALUE_RUNNER_PARSER_PROTOCOL_HTTP},
        {METRIC_LABEL_KEY_RUNNER_PARSE_STATUS, METRIC_LABEL_VALUE_RUNNER_PARSE_STATUS_FAILED}
    };
    ref = mPluginMetricMgr->GetOrCreateReentrantMetricsRecordRef(httpFailLabels);
    mParseHTTPEventsFailTotal = ref->GetCounter(METRIC_RUNNER_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL);
    mRefAndLabels.emplace_back(std::make_pair<>(ref, httpFailLabels));
}

void NetworkObserverSelfMonitor::HandleStatistic(nami::eBPFStatistics& stats) {
    if (!stats.updated_) return;
    // std::lock_guard<std::mutex> lk(mStatsMtx);
    UpdateInnerMetric(stats);
    // recv kernel events metric
    assert(stats.plugin_type_ == nami::PluginType::NETWORK_OBSERVE);
    nami::NetworkObserverStatistics* currNetworkStatsPtr = static_cast<nami::NetworkObserverStatistics*>(&stats);

    mRecvConnStatsTotal->Add(currNetworkStatsPtr->recv_conn_stat_events_total_);
    mRecvCtrlEventsTotal->Add(currNetworkStatsPtr->recv_ctrl_events_total_);
    mRecvHTTPDataEventsTotal->Add(currNetworkStatsPtr->recv_http_data_events_total_);

    // cache relative metric
    mConnTrackerNum->Set(currNetworkStatsPtr->conntracker_num_);

    mParseHTTPEventsSuccessTotal->Add(currNetworkStatsPtr->parse_http_records_success_total_);
    mParseHTTPEventsFailTotal->Add(currNetworkStatsPtr->parse_http_records_failed_total_);
    mAggMapEntitiesNum->Set(currNetworkStatsPtr->agg_map_entities_num_);
}

void eBPFSelfMonitorMgr::Init(const nami::PluginType type, MetricsRecordRef& ref, const std::string& name, const std::string& logstore) {
    if (mInited[int(type)]) {
        return;
    }

    static const std::unordered_map<std::string, MetricType> metricKeys = {
        {METRIC_RUNNER_IN_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_RUNNER_EBPF_LOSS_KERNEL_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
        {METRIC_RUNNER_EBPF_PROCESS_CACHE_ENTRIES_NUM, MetricType::METRIC_TYPE_INT_GAUGE},
        {METRIC_RUNNER_EBPF_PROCESS_CACHE_MISS_TOTAL, MetricType::METRIC_TYPE_COUNTER},
    };

    // Add project to labels
    // Add Runner Name to labels
    MetricLabelsPtr labelsPtr = ref.GetLabels();
    labelsPtr->push_back({METRIC_LABEL_KEY_RUNNER_NAME, METRIC_LABEL_VALUE_RUNNER_NAME_EBPF_SERVER});
    labelsPtr->push_back({METRIC_LABEL_KEY_LOGSTORE, logstore});


    switch (type)
    {
    case nami::PluginType::NETWORK_OBSERVE: {
        std::unordered_map<std::string, MetricType> keys = metricKeys;
        keys.insert({METRIC_RUNNER_EBPF_NETWORK_OBSERVER_CONNTRACKER_NUM, MetricType::METRIC_TYPE_INT_GAUGE});
        keys.insert({METRIC_RUNNER_EBPF_NETWORK_OBSERVER_AGGREGATE_KEY_NUM, MetricType::METRIC_TYPE_INT_GAUGE});
        keys.insert({METRIC_RUNNER_EBPF_NETWORK_OBSERVER_WORKER_HANDLE_EVENTS_TOTAL, MetricType::METRIC_TYPE_COUNTER});
        keys.insert({METRIC_RUNNER_EBPF_NETWORK_OBSERVER_PROTOCOL_PARSE_RECORDS_TOTAL, MetricType::METRIC_TYPE_COUNTER});
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labelsPtr, keys);
        mSelfMonitors[int(type)] = std::make_unique<NetworkObserverSelfMonitor>(name, pluginMetricManager, ref);
        break;
    }
    case nami::PluginType::NETWORK_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labelsPtr, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<NetworkSecuritySelfMonitor>(name, pluginMetricManager, ref);
        break;
    }
    case nami::PluginType::FILE_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labelsPtr, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<FileSecuritySelfMonitor>(name, pluginMetricManager, ref);
        break;
    }
    case nami::PluginType::PROCESS_SECURITY: {
        auto pluginMetricManager
            = std::make_shared<PluginMetricManager>(labelsPtr, metricKeys);
        mSelfMonitors[int(type)] = std::make_unique<ProcessSecuritySelfMonitor>(name, pluginMetricManager, ref);
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
