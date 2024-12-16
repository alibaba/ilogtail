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

#include <thread>
#include <mutex>
#include <iostream>
#include <metadata/K8sMetadata.h>
#include <unordered_set>

#include "common/Lock.h"
#include "ebpf/handler/ObserveHandler.h"
#include "pipeline/PipelineContext.h"
#include "common/RuntimeUtil.h"
#include "ebpf/SourceManager.h"
#include "models/SpanEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEvent.h"
#include "logger/Logger.h"
#include "pipeline/queue/ProcessQueueManager.h"
#include "pipeline/queue/ProcessQueueItem.h"

namespace logtail {
namespace ebpf {

#define ADD_STATUS_METRICS(METRIC_NAME, FIELD_NAME, VALUE) \
    { \
        if (!inner->FIELD_NAME) { \
            return; \
        } \
        auto* event = group.AddMetricEvent(); \
        for (const auto& tag : measure->tags_) { \
            event->SetTag(tag.first, tag.second); \
        } \
        event->SetTag(std::string("status_code"), std::string(VALUE)); \
        event->SetName(METRIC_NAME); \
        event->SetTimestamp(ts); \
        event->SetValue(UntypedSingleValue{(double)inner->FIELD_NAME}); \
    }

#define GENERATE_METRICS(FUNC_NAME, MEASURE_TYPE, INNER_TYPE, METRIC_NAME, FIELD_NAME) \
    void FUNC_NAME(PipelineEventGroup& group, const std::unique_ptr<Measure>& measure, uint64_t tsSec, uint64_t tsNano) { \
        if (measure->type_ != (MEASURE_TYPE)) { \
            return; \
        } \
        const auto* inner = static_cast<INNER_TYPE*>(measure->inner_measure_.get()); \
        if (!inner->FIELD_NAME) { \
            return; \
        } \
        auto* event = group.AddMetricEvent(); \
        for (const auto& tag : measure->tags_) { \
            event->SetTag(tag.first, tag.second); \
        } \
        event->SetName(METRIC_NAME); \
        event->SetTimestamp(tsSec, tsNano); \
        event->SetValue(UntypedSingleValue{(double)inner->FIELD_NAME}); \
    }

    void OtelMeterHandler::handle(const std::vector<std::unique_ptr<ApplicationBatchMeasure>>& measures,
                                  uint64_t timestamp) {
        if (measures.empty()) {
            return;
        }
        for (const auto& appBatchMeasures : measures) {
            PipelineEventGroup eventGroup(std::make_shared<SourceBuffer>());
            for (const auto& measure : appBatchMeasures->measures_) {
                auto type = measure->type_;
                if (type == MeasureType::MEASURE_TYPE_APP) {
                    auto* inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
                    auto* event = eventGroup.AddMetricEvent();
                    for (const auto& tag : measure->tags_) {
                        event->SetTag(tag.first, tag.second);
                    }
                    event->SetName("service_requests_total");
                    event->SetTimestamp(timestamp);
                    event->SetValue(UntypedSingleValue{(double)inner->request_total_});
                }
                mProcessTotalCnt++;
            }
#ifdef APSARA_UNIT_TEST_MAIN
        continue;
#endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item))) {
            LOG_WARNING(sLogger,
                        ("configName", mCtx->GetConfigName())("pluginIdx",
                                                              mPluginIdx)("[Otel Metrics] push queue failed!", ""));
        }
    }
    }

void OtelSpanHandler::handle(const std::vector<std::unique_ptr<ApplicationBatchSpan>>& spans) {
    if (spans.empty()) {
        return;
    }
    for (const auto& span : spans) {
        std::shared_ptr<SourceBuffer> sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        for (const auto& x : span->single_spans_) {
            auto* spanEvent = eventGroup.AddSpanEvent();
            for (const auto& tag : x->tags_) {
                spanEvent->SetTag(tag.first, tag.second);
            }
            spanEvent->SetName(x->span_name_);
            spanEvent->SetKind(static_cast<SpanEvent::Kind>(x->span_kind_));
            spanEvent->SetStartTimeNs(x->start_timestamp_);
            spanEvent->SetEndTimeNs(x->end_timestamp_);
            spanEvent->SetTraceId(x->trace_id_);
            spanEvent->SetSpanId(x->span_id_);
            mProcessTotalCnt++;
        }
#ifdef APSARA_UNIT_TEST_MAIN
        continue;
#endif
        {
            ReadLock lk(mCtxLock);
            if (!mCtx) {
                continue;
            }
            std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
            if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item))) {
                LOG_WARNING(
                    sLogger,
                    ("configName", mCtx->GetConfigName())("pluginIdx", mPluginIdx)("[Span] push queue failed!", ""));
            }
        }
    }
}

void EventHandler::handle(const std::vector<std::unique_ptr<ApplicationBatchEvent>>& events) {
    if (events.empty()) {
        return;
    }
    for (const auto& appEvents : events) {
        if (!appEvents || appEvents->events_.empty()) {
            continue;
        }
        std::shared_ptr<SourceBuffer> sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        for (const auto& event : appEvents->events_) {
            if (!event || event->GetAllTags().empty()) {
                continue;
            }
            auto* logEvent = eventGroup.AddLogEvent();
            for (const auto& tag : event->GetAllTags()) {
                logEvent->SetContent(tag.first, tag.second);
                auto seconds
                    = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::nanoseconds(event->GetTimestamp()));
                logEvent->SetTimestamp(seconds.count(), event->GetTimestamp() - seconds.count() * 1e9);
            }
            mProcessTotalCnt++;
        }
        for (const auto& tag : appEvents->tags_) {
            eventGroup.SetTag(tag.first, tag.second);
        }
#ifdef APSARA_UNIT_TEST_MAIN
        continue;
#endif
        {
            ReadLock lk(mCtxLock);
            if (!mCtx) {
                continue;
            }
            std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
            if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item))) {
                LOG_WARNING(sLogger, ("configName", mCtx->GetConfigName())("pluginIdx",mPluginIdx)("[Event] push queue failed!", ""));
            }
        }
    }
}

HostMetadataHandler::HostMetadataHandler(const logtail::PipelineContext* ctx, QueueKey key, uint32_t idx, int intervalSec)
    : AbstractHandler(ctx, key, idx), mIntervalSec(intervalSec) {
    mFlag = true;
    // TODO @qianlu.kk we need to move this into start function
    mReporter = std::thread(&HostMetadataHandler::ReportAgentInfo, this);
}

HostMetadataHandler::~HostMetadataHandler() {
    mFlag = false;
    if (mReporter.joinable()) {
        mReporter.join();
    }
}

const std::string pidKey = "pid";
const std::string appNameKey = "appName";
const std::string ipKey = "ip";
const std::string hostNameKey = "hostname";
const std::string agentVersionKey = "agentVersion";
const std::string startTimestampKey = "startTimeStamp";
const std::string dataTypeKey = "data_type";
const std::string agentInfoStr = "agent_info";

void HostMetadataHandler::ReportAgentInfo() {
    while(mFlag) {
        std::shared_ptr<SourceBuffer> sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        eventGroup.SetTag(dataTypeKey, agentInfoStr);
        auto nowSec = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        {
            ReadLock lk(mLock);
            for (const auto& [ip, spi] : mHostPods) {
                // generate agentinfo
                LOG_INFO(sLogger, ("AGENT_INFO appid", spi->mAppId)
                    ("appname", spi->mAppName)
                    ("ip", spi->mPodIp) ("hostname", spi->mPodName) ("agentVersion", "1.0.0") ("startTs", std::to_string(spi->mStartTime)));
                auto logEvent = eventGroup.AddLogEvent();
                logEvent->SetContent(pidKey, spi->mAppId);
                logEvent->SetContent(appNameKey, spi->mAppName);
                logEvent->SetContent(ipKey, spi->mPodIp);
                logEvent->SetContent(hostNameKey, spi->mPodName);
                logEvent->SetContent(agentVersionKey, "1.0.0");
                logEvent->SetContent(startTimestampKey, std::to_string(spi->mStartTime));
                logEvent->SetTimestamp(nowSec);

                // TODO @qianlu.kk generate host tags
                // auto metricEvent = eventGroup.AddMetricEvent();
                // metricEvent->SetName("arms_tag_entity");
                // metricEvent->SetTagNoCopy("hostname", "");
                // metricEvent->SetValue(UntypedSingleValue{1.0});
            }
        }

        {
            ReadLock lk(mCtxLock);
            if (!mCtx) {
                continue;
            }

            std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
            auto res = ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item));
            if (res) {
                LOG_WARNING(sLogger, ("[AgentInfo] push queue failed! status", res));
            } else {
                LOG_DEBUG(sLogger, ("[AgentInfo] push queue success!", ""));
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(mIntervalSec));
    }
}

bool HostMetadataHandler::handle(uint32_t pluginIndex, std::vector<std::string>& podIpVec) {
    std::vector<std::string> newContainerIds;
    std::vector<std::string> expiredContainerIds;
    std::unordered_set<std::string> newPodIps;
    std::vector<std::string> addedContainers;
    std::vector<std::string> removedContainers;

    std::unordered_map<std::string, std::unique_ptr<SimplePodInfo>> currentHostPods;
    std::unordered_set<std::string> currentContainers;


    for (const auto& ip : podIpVec) {
        auto podInfo = K8sMetadata::GetInstance().GetInfoByIpFromCache(ip);
        if (!podInfo || podInfo->appId == "") {
            // filter appid ...
            LOG_INFO(sLogger, (ip, "cannot fetch pod metadata or doesn't have arms label"));
            continue;
        }

        std::unique_ptr<SimplePodInfo> spi = std::make_unique<SimplePodInfo>(
            uint64_t(podInfo->timestamp), 
            podInfo->appId,
            podInfo->appName,
            podInfo->podIp, 
            podInfo->podName,
            podInfo->containerIds);
        currentHostPods[ip] = std::move(spi);
        std::string cids;
        for (auto& cid : podInfo->containerIds) {
            cids += cid + ",";
            currentContainers.insert(cid);
            if (!mCids.count(cid)) {
                // if cid doesn't exist in last cid set
                addedContainers.push_back(cid);
            }
        }
        LOG_INFO(sLogger, ("appId", podInfo->appId) ("appName", podInfo->appName) ("podIp", podInfo->podIp) ("podName", podInfo->podName) ("containerId", cids));
    }

    LOG_INFO(sLogger, ("begin to update local host metadata, pod list size:", newPodIps.size()) ("input pod list size", podIpVec.size()) ("host pod list size", mHostPods.size()));

    for (auto& cid : mCids) {
        if (!currentContainers.count(cid)) {
            // if cid doesn't exist in current container ids list
            removedContainers.push_back(cid);
        }
    }

    // update cids ...
    mCids = std::move(currentContainers);

    {
        WriteLock lk(mLock);
        // update cache ... 
        mHostPods = std::move(currentHostPods);
    }

    std::string addPodsStr;
    for (auto& cid : addedContainers) {
        addPodsStr += cid;
        addPodsStr += ",";
    }

    std::string removePodsStr;
    for (auto& cid : removedContainers) {
        removePodsStr += cid;
        removePodsStr += ",";
    }

    std::string hostPodsStr;
    for (auto& pod : mHostPods) {
        hostPodsStr += pod.first;
        hostPodsStr += ",";
    }
    if (removedContainers.size() || addedContainers.size()) {
        nami::ObserverNetworkOption ops;
        ops.mDisableCids = removedContainers;
        ops.mEnableCids = addedContainers;
        ops.mEnableCidFilter = true;
        bool ret = mUpdateFunc(nami::PluginType::NETWORK_OBSERVE, UpdataType::OBSERVER_UPDATE_TYPE_CHANGE_WHITELIST, &ops);
        LOG_INFO(sLogger, ("after update, self pod list size", mHostPods.size()) 
            ("add cids", addPodsStr) 
            ("remove cids", removePodsStr) 
            ("host pods ip", hostPodsStr) 
            ("ret", ret) ("ops.mEnableCids.size", ops.mEnableCids.size()) ("ops.mDisableCids.size", ops.mDisableCids.size()));
        return ret;
    }    

    return true;
}

#ifdef __ENTERPRISE__

const static std::string app_name_key = "service";
const static std::string app_id_key = "pid";
const static std::string ip_key = "serverIp";
const static std::string host_key = "host";

const static std::string app_id_key_span = "arms.appId";
const static std::string service_name_key = "service.name";
const static std::string host_ip_key = "host.ip";
const static std::string host_name_key = "host.name";

const static std::string rpc_request_total_count = "arms_rpc_requests_count";
const static std::string rpc_request_slow_count = "arms_rpc_requests_slow_count";
const static std::string rpc_request_err_count = "arms_rpc_requests_error_count";
const static std::string rpc_request_rt = "arms_rpc_requests_seconds";
const static std::string rpc_request_status_count = "arms_rpc_requests_by_status_count";
static const std::string status_2xx_key = "2xx";
static const std::string status_3xx_key = "2xx";
static const std::string status_4xx_key = "2xx";
static const std::string status_5xx_key = "2xx";

// FOR APP METRICS
GENERATE_METRICS(GenerateRequestsTotalMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, rpc_request_total_count, request_total_)
GENERATE_METRICS(GenerateRequestsSlowMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, rpc_request_slow_count, slow_total_)
GENERATE_METRICS(GenerateRequestsErrorMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, rpc_request_err_count, error_total_)
GENERATE_METRICS(GenerateRequestsDurationSumMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, rpc_request_status_count, duration_ms_sum_)

void GenerateRequestsStatusMetrics(PipelineEventGroup& group, const std::unique_ptr<Measure>& measure, uint64_t ts, uint64_t tsNano) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    const auto* inner = static_cast<const AppSingleMeasure*>(measure->inner_measure_.get());
    ADD_STATUS_METRICS(rpc_request_status_count, status_2xx_count_, status_2xx_key);
    ADD_STATUS_METRICS(rpc_request_status_count, status_3xx_count_, status_3xx_key);
    ADD_STATUS_METRICS(rpc_request_status_count, status_4xx_count_, status_4xx_key);
    ADD_STATUS_METRICS(rpc_request_status_count, status_5xx_count_, status_5xx_key);
}

const static std::string npm_tcp_drop_total = "arms_npm_tcp_drop_count";
const static std::string npm_tcp_retrans_total = "arms_npm_tcp_retrans_total";
const static std::string npm_tcp_count_total = "arms_npm_tcp_count_by_state";
const static std::string npm_recv_pkt_total = "arms_npm_recv_packets_total";
const static std::string npm_recv_byte_total = "arms_npm_recv_bytes_total";
const static std::string npm_send_pkt_total = "arms_npm_sent_packets_total";
const static std::string npm_send_byte_total = "arms_npm_sent_bytes_total";

// FOR NET METRICS
GENERATE_METRICS(GenerateTcpDropTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_tcp_drop_total, tcp_drop_total_)
GENERATE_METRICS(GenerateTcpRetransTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_tcp_retrans_total, tcp_retran_total_)
GENERATE_METRICS(GenerateTcpConnectionTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_tcp_count_total, tcp_connect_total_)
GENERATE_METRICS(GenerateTcpRecvPktsTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_recv_pkt_total, recv_pkt_total_)
GENERATE_METRICS(GenerateTcpRecvBytesTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_recv_byte_total, recv_byte_total_)
GENERATE_METRICS(GenerateTcpSendPktsTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_send_pkt_total, send_pkt_total_)
GENERATE_METRICS(GenerateTcpSendBytesTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, npm_send_byte_total, send_byte_total_)

void ArmsSpanHandler::handle(const std::vector<std::unique_ptr<ApplicationBatchSpan>>& spans) {
    if (spans.empty()) {
        return;
    }
    for (const auto& span : spans) {
        std::shared_ptr<SourceBuffer> sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        eventGroup.SetTag(app_id_key_span, span->app_id_);
        eventGroup.SetTag(service_name_key, span->app_name_);
        eventGroup.SetTag(host_ip_key, span->host_ip_);
        eventGroup.SetTag(host_name_key, span->host_name_);
        eventGroup.SetTagNoCopy("data_type", "trace");
        eventGroup.SetTagNoCopy("arms.app.type", "ebpf");
        eventGroup.SetTag(app_id_key, span->app_id_);
        for (const auto& x : span->single_spans_) {
            auto* spanEvent = eventGroup.AddSpanEvent();
            for (const auto& tag : x->tags_) {
                spanEvent->SetTag(tag.first, tag.second);
            }
            spanEvent->SetName(x->span_name_);
            spanEvent->SetKind(static_cast<SpanEvent::Kind>(x->span_kind_));
            spanEvent->SetStartTimeNs(x->start_timestamp_);
            spanEvent->SetEndTimeNs(x->end_timestamp_);
            spanEvent->SetTraceId(x->trace_id_);
            spanEvent->SetSpanId(x->span_id_);
            mProcessTotalCnt++;
        }
#ifdef APSARA_UNIT_TEST_MAIN
        continue;
#endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item))) {
            LOG_WARNING(sLogger, ("configName", mCtx->GetConfigName())("pluginIdx",mPluginIdx)("[Span] push queue failed!", ""));
        }
    }
}

void ArmsMeterHandler::handle(const std::vector<std::unique_ptr<ApplicationBatchMeasure>>& measures,
                              uint64_t tsSec) {
    if (measures.empty()) {
        return;
    }
    for (const auto& appBatchMeasures : measures) {
        std::shared_ptr<SourceBuffer> sourceBuffer = std::make_shared<SourceBuffer>();;
        PipelineEventGroup eventGroup(sourceBuffer);
        LOG_DEBUG(sLogger, ("receive measures size", measures.size()) ("second", tsSec) 
            ("app name", appBatchMeasures->app_name_)
            ("app id", appBatchMeasures->app_id_)
            ("ip", appBatchMeasures->ip_)
            ("host", appBatchMeasures->host_));
        eventGroup.SetTag(app_name_key, appBatchMeasures->app_name_);
        eventGroup.SetTag(app_id_key, appBatchMeasures->app_id_);
        eventGroup.SetTag(ip_key, appBatchMeasures->ip_);
        eventGroup.SetTag(host_key, appBatchMeasures->host_);
        eventGroup.SetTagNoCopy("source", "ebpf");
        eventGroup.SetTagNoCopy("data_type", "metric");
        for (const auto& measure : appBatchMeasures->measures_) {
            auto type = measure->type_;
            if (type == MeasureType::MEASURE_TYPE_APP) {
                GenerateRequestsTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateRequestsSlowMetrics(eventGroup, measure, tsSec, 0);
                GenerateRequestsErrorMetrics(eventGroup, measure, tsSec, 0);
                GenerateRequestsDurationSumMetrics(eventGroup, measure, tsSec, 0);
                GenerateRequestsStatusMetrics(eventGroup, measure, tsSec, 0);
            } else if (type == MeasureType::MEASURE_TYPE_NET) {
                GenerateTcpDropTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpRetransTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpConnectionTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpRecvPktsTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpRecvBytesTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpSendPktsTotalMetrics(eventGroup, measure, tsSec, 0);
                GenerateTcpSendBytesTotalMetrics(eventGroup, measure, tsSec, 0);
            }
            mProcessTotalCnt++;
        }
        
#ifdef APSARA_UNIT_TEST_MAIN
        continue;
#endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(eventGroup), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item))) {
            LOG_WARNING(sLogger, ("configName", mCtx->GetConfigName())("pluginIdx",mPluginIdx)("[Metrics] push queue failed!", ""));
        }
    }
}

#endif

}
}
