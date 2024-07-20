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


// FOR APP METRICS
void GenerateRequestsTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    if (!inner->request_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_rpc_requests_count");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->request_total_});
}

void GenerateRequestsDurationSumMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    if (!inner->duration_ms_sum_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_rpc_requests_seconds");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->duration_ms_sum_});
}

void GenerateRequestsStatusMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    if (inner->status_2xx_count_ > 0) {
        auto event = group.AddMetricEvent();
        for (auto& tag : measure->tags_) {
            event->SetTag(tag.first, tag.second);
        }
        event->SetTag(std::string("status_code"), std::string("2xx"));
        event->SetName("arms_rpc_requests_by_status_count");
        event->SetTimestamp(ts);
        event->SetValue(UntypedSingleValue{(double)inner->status_2xx_count_});
    }

    if (inner->status_2xx_count_ > 0) {
        auto event = group.AddMetricEvent();
        for (auto& tag : measure->tags_) {
            event->SetTag(tag.first, tag.second);
        }
        event->SetTag(std::string("status_code"), std::string("3xx"));
        event->SetName("arms_rpc_requests_by_status_count");
        event->SetTimestamp(ts);
        event->SetValue(UntypedSingleValue{(double)inner->status_3xx_count_});
    }

    if (inner->status_4xx_count_ > 0) {
        auto event = group.AddMetricEvent();
        for (auto& tag : measure->tags_) {
            event->SetTag(tag.first, tag.second);
        }
        event->SetTag(std::string("status_code"), std::string("4xx"));
        event->SetName("arms_rpc_requests_by_status_count");
        event->SetTimestamp(ts);
        event->SetValue(UntypedSingleValue{(double)inner->status_4xx_count_});
    }

    if (inner->status_5xx_count_ > 0) {
        auto event = group.AddMetricEvent();
        for (auto& tag : measure->tags_) {
            event->SetTag(tag.first, tag.second);
        }
        event->SetTag(std::string("status_code"), std::string("5xx"));
        event->SetName("arms_rpc_requests_by_status_count");
        event->SetTimestamp(ts);
        event->SetValue(UntypedSingleValue{(double)inner->status_5xx_count_});
    }
}

void GenerateRequestsSlowMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    if (!inner->slow_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_rpc_requests_slow_count");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->slow_total_});
}

void GenerateRequestsErrorMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = static_cast<AppSingleMeasure*>(measure->inner_measure_.get());
    if (!inner->error_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_rpc_requests_error_count");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->error_total_});
}

// FOR NET METRICS
void GenerateTcpDropTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->tcp_drop_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_tcp_drop_count");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->tcp_drop_total_});
}

void GenerateTcpRetransTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->tcp_retran_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_tcp_retrans_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->tcp_retran_total_});
}

void GenerateTcpConnectionTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->tcp_connect_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_tcp_count_by_state");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->tcp_connect_total_});
}

void GenerateTcpRecvPktsTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->recv_pkt_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_recv_packets_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->recv_pkt_total_});
}

void GenerateTcpRecvBytesTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->recv_byte_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_recv_bytes_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->recv_byte_total_});
}

void GenerateTcpSendPktsTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->send_pkt_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_sent_packets_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->send_pkt_total_});
}

void GenerateTcpSendBytesTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_NET) return;
    auto inner = static_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->send_byte_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_sent_bytes_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->send_byte_total_});
}


void ArmsSpanHandler::handle(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans) {
    LOG_INFO(sLogger, ("HandleSpans spans size", spans.size()));
    if (spans.empty()) return;

    if (!ctx_) {
        LOG_WARNING(sLogger, ("HandleSpans", "ctx is null, exit"));
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
            process_total_count_++;
        }
        #ifdef APSARA_UNIT_TEST_MAIN
            continue;
        #endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), plugin_idx_);
        if (ProcessQueueManager::GetInstance()->PushQueue(ctx_->GetProcessQueueKey(), std::move(item))) {
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
    if (!ctx_) {
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
            process_total_count_++;
        }
        
        #ifdef APSARA_UNIT_TEST_MAIN
            continue;
        #endif
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(event_group), plugin_idx_);
        if (ProcessQueueManager::GetInstance()->PushQueue(ctx_->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Metrics] push queue failed!", "a"));
        }
    }
    return;
}

}
}