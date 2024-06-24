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

#include "ebpf/observer/ObserverServer.h"
#include "common/RuntimeUtil.h"
#include "ebpf/SourceManager.h"
#include "models/SpanEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/PipelineEvent.h"
#include "logger/Logger.h"
#include "queue/ProcessQueueManager.h"
#include "queue/ProcessQueueItem.h"

#include <thread>
#include <mutex>
#include <iostream>

namespace logtail {

// 负责接收ebpf返回的数据，然后将数据推送到对应的队列中
// TODO: 目前暂时没有考虑并发Start的问题
void ObserverServer::Start(BPFObserverPipelineType type) {
    if (mIsRunning) {
        return;
    } else {
        ObserverServer::InitBPF();
        mIsRunning = true;
        // TODO: 创建一个线程，用于接收ebpf返回的数据，并将数据推送到对应的队列中
        LOG_INFO(sLogger, ("observer ebpf server", "started"));
    }
}

void ObserverServer::Stop(BPFObserverPipelineType type) {
    // TODO: ebpf_stop(); 停止所有类型的ebpf探针
    mIsRunning = false;
}

void ObserverServer::Stop() {
    // TODO: ebpf_stop(); 停止所有类型的ebpf探针
    mIsRunning = false;
}

// FOR APP METRICS
void GenerateRequestsTotalMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = reinterpret_cast<AppSingleMeasure*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<AppSingleMeasure*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<AppSingleMeasure*>(measure->inner_measure_.get());
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
        // TODO @qianlu.kk
        event->SetTag(std::string("status_code"), std::string("5xx"));
        event->SetName("arms_rpc_requests_by_status_count");
        event->SetTimestamp(ts);
        event->SetValue(UntypedSingleValue{(double)inner->status_5xx_count_});
    }
}

void GenerateRequestsSlowMetrics(PipelineEventGroup& group, std::unique_ptr<Measure>& measure, uint64_t ts) {
    if (measure->type_ != MeasureType::MEASURE_TYPE_APP) return;
    auto inner = reinterpret_cast<AppSingleMeasure*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<AppSingleMeasure*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
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
    auto inner = reinterpret_cast<NetSingleMeasre*>(measure->inner_measure_.get());
    if (!inner->send_byte_total_) return;
    auto event = group.AddMetricEvent();
    for (auto& tag : measure->tags_) {
        event->SetTag(tag.first, tag.second);
    }
    
    event->SetName("arms_npm_sent_bytes_total");
    event->SetTimestamp(ts);
    event->SetValue(UntypedSingleValue{(double)inner->send_byte_total_});
}

void ObserverServer::HandleMeasures(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp) {
    LOG_INFO(sLogger, ("HandleMeasures measure size", measures.size()) ("ts", timestamp));
    if (measures.empty()) return;
    auto ctx = network_config_.second;
    if (!ctx) {
        LOG_WARNING(sLogger, ("HandleSpans", "ctx is null, exit"));
        return;
    }

    for (auto& app_batch_measures : measures) {
        std::shared_ptr<SourceBuffer> mSourceBuffer = std::make_shared<SourceBuffer>();;
        PipelineEventGroup mTestEventGroup(mSourceBuffer);
        
        // source_ip
        mTestEventGroup.SetTag(std::string("appId"), app_batch_measures->app_id_);
        mTestEventGroup.SetTag(std::string("ip"), app_batch_measures->ip_);
        for (auto& measure : app_batch_measures->measures_) {
            auto type = measure->type_;
            if (type == MeasureType::MEASURE_TYPE_APP) {
                GenerateRequestsTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateRequestsSlowMetrics(mTestEventGroup, measure, timestamp);
                GenerateRequestsErrorMetrics(mTestEventGroup, measure, timestamp);
                GenerateRequestsDurationSumMetrics(mTestEventGroup, measure, timestamp);
                GenerateRequestsStatusMetrics(mTestEventGroup, measure, timestamp);
                
            } else if (type == MeasureType::MEASURE_TYPE_NET) {
                GenerateTcpDropTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpRetransTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpConnectionTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpRecvPktsTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpRecvBytesTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpSendPktsTotalMetrics(mTestEventGroup, measure, timestamp);
                GenerateTcpSendBytesTotalMetrics(mTestEventGroup, measure, timestamp);
            }
        }
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(mTestEventGroup), 0);
        if (ProcessQueueManager::GetInstance()->PushQueue(ctx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Metrics] push queue failed!", "a"));
        }
    }
    return;
}

void ObserverServer::HandleSpans(std::vector<std::unique_ptr<ApplicationBatchSpan>>&& spans) {
    LOG_INFO(sLogger, ("HandleSpans spans size", spans.size()));
    if (spans.empty()) return;

    auto ctx = network_config_.second;
    if (!ctx) {
        LOG_WARNING(sLogger, ("HandleSpans", "ctx is null, exit"));
        return;
    }

    for (auto& span : spans) {
        std::shared_ptr<SourceBuffer> mSourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup mTestEventGroup(mSourceBuffer);
        mTestEventGroup.SetTag("arms.appId", span->app_id_);
        for (auto& x : span->single_spans_) {
            auto spanEvent = mTestEventGroup.AddSpanEvent();
            for (auto& tag : x->tags_) {
                spanEvent->SetTag(tag.first, tag.second);
            }
            spanEvent->SetName(x->span_name_);
            spanEvent->SetKind(static_cast<SpanEvent::Kind>(x->span_kind_));
            spanEvent->SetStartTimeNs(x->start_timestamp_);
            spanEvent->SetEndTimeNs(x->end_timestamp_);
            spanEvent->SetTraceId(x->trace_id_);
            spanEvent->SetSpanId(x->span_id_);
        }
        
        std::unique_ptr<ProcessQueueItem> item = std::make_unique<ProcessQueueItem>(std::move(mTestEventGroup), 0);
        if (ProcessQueueManager::GetInstance()->PushQueue(ctx->GetProcessQueueKey(), std::move(item))) {
            LOG_WARNING(sLogger, ("[Span] push queue failed!", "a"));
        }

        // TODO @qianlu.kk 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return;

}

// 插件配置注册逻辑
// 负责启动对应的ebpf程序
void ObserverServer::AddObserverOptions(const std::string& name,
                                        size_t index,
                                        const ObserverOptions* options,
                                        const PipelineContext* ctx) {
    std::string key = name + "#" + std::to_string(index);
    mInputConfigMap[key] = std::make_pair(options, ctx);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (options->mType) {
        case ObserverType::FILE: {
            // TODO: ebpf_start(type);
            file_config_ = std::make_pair(options, ctx);
            break;
        }
        case ObserverType::PROCESS: {
            process_config_ = std::make_pair(options, ctx);
            break;
        }
        case ObserverType::NETWORK: {
            network_config_ = std::make_pair(options, ctx);
            break;
        }
        default:
            break;
    }
}

// 插件配置注销逻辑
// TODO: 目前处理配置变更，先stop掉该类型的探针，然后在map里remove配置
void ObserverServer::RemoveObserverOptions(const std::string& name, size_t index) {
    std::string key = name + "#" + std::to_string(index);
    // TODO: 目前一种类型的input只能处理一个，后续需要修改
    switch (mInputConfigMap[key].first->mType) {
        case ObserverType::FILE: {
            file_config_ = std::make_pair(nullptr, nullptr);
            // TODO: ebpf_stop(type);
            StopBPF();
            break;
        }
        case ObserverType::PROCESS: {
            process_config_ = std::make_pair(nullptr, nullptr);
            StopBPF();
            // TODO: ebpf_stop(type);
            break;
        }
        case ObserverType::NETWORK: {
            network_config_ = std::make_pair(nullptr, nullptr);
            StopBPF();
            // TODO: ebpf_stop(type);
            break;
        }
        default:
            break;
    }
    mInputConfigMap.erase(key);
}

void ObserverServer::Init() {
    InitBPF();
}

void ObserverServer::StopBPF() {
    ref_.fetch_add(-1);
    if (ref_.load() > 0) {
        return;
    }

    if (!ebpf::SourceManager::GetInstance()->StopSockettraceProbe()) {
        LOG_ERROR(sLogger, ("failed to load and stop dynamic lib", "libsockettrace.so"));
    }
}

void ObserverServer::InitBPF() {
    LOG_INFO(sLogger, ("try to init bpf", " ... "));
    // memory is managed by ebpf plugin ... 
    SockettraceConfig* config = new SockettraceConfig;
    config->enable_so_ = false;
    config->measure_cb_ = std::bind(&ObserverServer::HandleMeasures, this, std::placeholders::_1,std::placeholders::_2);
    config->span_cb_ = std::bind(&ObserverServer::HandleSpans, this, std::placeholders::_1);
    if (!ebpf::SourceManager::GetInstance()->LoadAndStartDynamicLib(ebpf::eBPFPluginType::SOCKETTRACE, config)) {
        LOG_ERROR(sLogger, ("failed to load and start dynamic lib", "libsockettrace.so"));
    } else {
        ref_.fetch_add(1);
    }
}

void ObserverServer::CollectEvents() {
    
}

} // namespace logtail
