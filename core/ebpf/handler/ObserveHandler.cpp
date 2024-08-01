#include <thread>
#include <mutex>
#include <iostream>

#include "ebpf/handler/ObserveHandler.h"
#include "pipeline/PipelineContext.h"

#include "common/RuntimeUtil.h"
#include "ebpf/SourceManager.h"
#include "models/SpanEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEvent.h"
#include "logger/Logger.h"
#include "queue/ProcessQueueManager.h"
#include "queue/ProcessQueueItem.h"

namespace logtail {
namespace ebpf {

#define ADD_STATUS_METRICS(METRIC_NAME, FIELD_NAME, VALUE) \
    {if (!inner->FIELD_NAME) return; \
    auto event = group.AddMetricEvent(); \
    for (auto& tag : measure->tags_) { \
        event->SetTag(tag.first, tag.second); \
    } \
    event->SetTag(std::string("status_code"), std::string(VALUE)); \
    event->SetName(METRIC_NAME); \
    event->SetTimestamp(ts); \
    event->SetValue(UntypedSingleValue{(double)inner->FIELD_NAME});} \

#define GENERATE_METRICS(FUNC_NAME, MEASURE_TYPE, INNER_TYPE, METRIC_NAME, FIELD_NAME) \
void FUNC_NAME(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) { \
    if (measure->type_ != MEASURE_TYPE) return; \
    auto inner = static_cast<INNER_TYPE*>(measure->inner_measure_.get()); \
    if (!inner->FIELD_NAME) return; \
    auto event = group.AddMetricEvent(); \
    for (auto& tag : measure->tags_) { \
        event->SetTag(tag.first, tag.second); \
    } \
    event->SetName(METRIC_NAME); \
    event->SetTimestamp(ts); \
    event->SetValue(UntypedSingleValue{(double)inner->FIELD_NAME}); \
}

void OtelMeterHandler::handle(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp) {
        LOG_INFO(sLogger, ("Otel HandleMeasures measure size", measures.size()) ("ts", timestamp));
    if (measures.empty()) return;
    if (!mCtx) {
        LOG_WARNING(sLogger, ("Otel HandleSpans", "ctx is null, exit"));
        return;
    }

    for (auto& app_batch_measures : measures) {
        std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();;
        PipelineEventGroup event_group(source_buffer);
        for (auto& measure : app_batch_measures->measures_) {
            auto type = measure->type_;
            if (type == MeasureType::MEASURE_TYPE_APP) {
                auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
                auto event = event_group.AddMetricEvent();
                for (auto& tag : measure->tags_) {
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
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mCtx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Otel Metrics] push queue failed!", "a"));
        }
    }
    return;
}

void OtelSpanHandler::handle(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans) {
    LOG_INFO(sLogger, ("Otel HandleSpans spans size", spans.size()));
    if (spans.empty()) return;

    if (!mCtx) {
        LOG_WARNING(sLogger, ("Otel HandleSpans", "ctx is null, exit"));
        return;
    }
    std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup event_group(source_buffer);

    for (auto& span : spans) {
        for (auto& x : span->single_spans_) {
            auto spanEvent = event_group.AddSpanEvent();
            for (auto& tag : x->tags_) {
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
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mCtx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Span] push queue failed!", ""));
        }
    }

    return;
}

#ifdef __ENTERPRISE__

// FOR APP METRICS
GENERATE_METRICS(GenerateRequestsTotalMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, "arms_rpc_requests_count", request_total_)
GENERATE_METRICS(GenerateRequestsSlowMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, "arms_rpc_requests_slow_count", slow_total_)
GENERATE_METRICS(GenerateRequestsErrorMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, "arms_rpc_requests_error_count", error_total_)
GENERATE_METRICS(GenerateRequestsDurationSumMetrics, MeasureType::MEASURE_TYPE_APP, AppSingleMeasure, "arms_rpc_requests_seconds", duration_ms_sum_)

void GenerateRequestsStatusMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    ADD_STATUS_METRICS("arms_rpc_requests_by_status_count", status_2xx_count_, "2xx");
    ADD_STATUS_METRICS("arms_rpc_requests_by_status_count", status_3xx_count_, "3xx");
    ADD_STATUS_METRICS("arms_rpc_requests_by_status_count", status_4xx_count_, "4xx");
    ADD_STATUS_METRICS("arms_rpc_requests_by_status_count", status_5xx_count_, "5xx");
}

// FOR NET METRICS
GENERATE_METRICS(GenerateTcpDropTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_tcp_drop_count", tcp_drop_total_)
GENERATE_METRICS(GenerateTcpRetransTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_tcp_retrans_total", tcp_retran_total_)
GENERATE_METRICS(GenerateTcpConnectionTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_tcp_count_by_state", tcp_connect_total_)
GENERATE_METRICS(GenerateTcpRecvPktsTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_recv_packets_total", recv_pkt_total_)
GENERATE_METRICS(GenerateTcpRecvBytesTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_recv_bytes_total", recv_byte_total_)
GENERATE_METRICS(GenerateTcpSendPktsTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_sent_packets_total", send_pkt_total_)
GENERATE_METRICS(GenerateTcpSendBytesTotalMetrics, MeasureType::MEASURE_TYPE_NET, NetSingleMeasure, "arms_npm_sent_bytes_total", send_byte_total_)

void ArmsSpanHandler::handle(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans) {
    LOG_INFO(sLogger, ("ARMS HandleSpans spans size", spans.size()));
    if (spans.empty()) return;

    if (!mCtx) {
        LOG_WARNING(sLogger, ("ARMS HandleSpans", "ctx is null, exit"));
        return;
    }

    for (auto& span : spans) {
        std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup event_group(source_buffer);
        event_group.SetTag("arms.appId", span->app_id_);
        for (auto& x : span->single_spans_) {
            auto spanEvent = event_group.AddSpanEvent();
            for (auto& tag : x->tags_) {
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
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mCtx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Span] push queue failed!", "a"));
        }

        // TODO @qianlu.kk 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return;
}

void ArmsMeterHandler::handle(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp) {
    LOG_INFO(sLogger, ("HandleMeasures measure size", measures.size()) ("ts", timestamp));
    if (measures.empty()) return;
    if (!mCtx) {
        LOG_WARNING(sLogger, ("HandleSpans", "ctx is null, exit"));
        return;
    }

    for (auto& app_batch_measures : measures) {
        std::shared_ptr<SourceBuffer> source_buffer = std::make_shared<SourceBuffer>();;
        PipelineEventGroup event_group(source_buffer);
        
        // source_ip
        event_group.SetTag(std::string("appId"), app_batch_measures->app_id_);
        event_group.SetTag(std::string("ip"), app_batch_measures->ip_);
        for (auto& measure : app_batch_measures->measures_) {
            auto type = measure->type_;
            if (type == MeasureType::MEASURE_TYPE_APP) {
                GenerateRequestsTotalMetrics(event_group, measure, timestamp);
                GenerateRequestsSlowMetrics(event_group, measure, timestamp);
                GenerateRequestsErrorMetrics(event_group, measure, timestamp);
                GenerateRequestsDurationSumMetrics(event_group, measure, timestamp);
                GenerateRequestsStatusMetrics(event_group, measure, timestamp);
                
            } else if (type == MeasureType::MEASURE_TYPE_NET) {
                GenerateTcpDropTotalMetrics(event_group, measure, timestamp);
                GenerateTcpRetransTotalMetrics(event_group, measure, timestamp);
                GenerateTcpConnectionTotalMetrics(event_group, measure, timestamp);
                GenerateTcpRecvPktsTotalMetrics(event_group, measure, timestamp);
                GenerateTcpRecvBytesTotalMetrics(event_group, measure, timestamp);
                GenerateTcpSendPktsTotalMetrics(event_group, measure, timestamp);
                GenerateTcpSendBytesTotalMetrics(event_group, measure, timestamp);
            }
            mProcessTotalCnt++;
        }
        
        #ifdef APSARA_UNIT_TEST_MAIN
            continue;
        #endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), mPluginIdx);
        if (ProcessQueueManager::GetInstance()->PushQueue(mCtx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Metrics] push queue failed!", "a"));
        }
    }
    return;
}

#endif

}
}