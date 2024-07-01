//
// Created by lurious on 2024/6/17.
//

#include "serializer/ArmsSerializer.h"

#include "arms_metrics_pb/MeasureBatches.pb.h"
#include "logger/Logger.h"
#include "models/MetricEvent.h"
#include "models/PipelineEvent.h"
#include "models/SpanEvent.h"
#include "span_pb/trace.pb.h"

const std::map<std::string, proto::EnumUnit> metricUnitMap{
    {"arms_rpc_requests_count", proto::EnumUnit::COUNT},
    {"arms_rpc_requests_error_count", proto::EnumUnit::COUNT},
    {"arms_rpc_requests_seconds", proto::EnumUnit::MILLISECONDS},
    {"arms_rpc_requests_slow_count", proto::EnumUnit::COUNT},
    {"arms_rpc_requests_by_status_count", proto::EnumUnit::COUNT},
    {"arms_npm_sent_bytes_total", proto::EnumUnit::BYTE},
    {"arms_npm_recv_bytes_total", proto::EnumUnit::BYTE},
    {"arms_npm_sent_packets_total", proto::EnumUnit::COUNT},
    {"arms_npm_recv_packets_total", proto::EnumUnit::COUNT},
    {"arms_npm_tcp_retrans_total", proto::EnumUnit::COUNT},
    {"arms_npm_tcp_drop_count", proto::EnumUnit::COUNT},
    {"arms_npm_tcp_conn_stats_count", proto::EnumUnit::COUNT},
    {"arms_npm_tcp_count_by_state", proto::EnumUnit::COUNT},
    {"arms_npm_tcp_rtt_avg", proto::EnumUnit::MILLISECONDS},
};

const std::string COUNT = "count";
const std::string SUM = "SUM";


const std::map<std::string, std::string> metricValueTypeMap{
    {"arms_rpc_requests_count", COUNT},
    {"arms_rpc_requests_error_count", COUNT},
    {"arms_rpc_requests_seconds", SUM},
    {"arms_rpc_requests_slow_count", COUNT},
    {"arms_rpc_requests_by_status_count", COUNT},
    {"arms_npm_sent_bytes_total", COUNT},
    {"arms_npm_recv_bytes_total", COUNT},
    {"arms_npm_sent_packets_total", COUNT},
    {"arms_npm_recv_packets_total", COUNT},
    {"arms_npm_tcp_retrans_total", COUNT},
    {"arms_npm_tcp_drop_count", COUNT},
    {"arms_npm_tcp_conn_stats_count", COUNT},
    {"arms_npm_tcp_count_by_state", COUNT},
    {"arms_npm_tcp_rtt_avg", SUM},
};


namespace logtail {

bool ArmsMetricsEventGroupListSerializer::Serialize(std::vector<BatchedEventsList>&& v,
                                                    std::string& res,
                                                    std::string& errorMsg) {
    // auto measureBatches = new proto::MeasureBatches();
    std::shared_ptr<proto::MeasureBatches> measureBatches = std::make_shared<proto::MeasureBatches>();
    for (auto& batchedEventsList : v) {
        ConvertBatchedEventsListToMeasureBatch(std::move(batchedEventsList), measureBatches.get());
    }
    res = measureBatches->SerializeAsString();
    return true;
}

void ArmsMetricsEventGroupListSerializer::ConvertBatchedEventsListToMeasureBatch(
    BatchedEventsList&& batchedEventsList, proto::MeasureBatches* measureBatches) {
    for (auto& batchedEvents : batchedEventsList) {
        auto tags = batchedEvents.mTags.mInner;
        auto measureBatch = measureBatches->add_measurebatches();
        measureBatch->set_type("app");
        measureBatch->set_ip(GetIpFromTags(batchedEvents.mTags));
        measureBatch->set_time(GetMeasureTimestamp(batchedEvents));
        measureBatch->set_version("v1");
        measureBatch->set_pid(GetAppIdFromTags(batchedEvents.mTags));
        measureBatch->mutable_commontags()->insert({"source", "ebpf"});
        measureBatch->mutable_commontags()->insert({"appType", "EBPF"});
        for (auto& kv : batchedEvents.mTags.mInner) {
            measureBatch->mutable_commontags()->insert({kv.first.to_string(), kv.second.to_string()});
        }
        ConvertBatchedEventsToMeasures(std::move(batchedEvents), measureBatch);
    }
}

int64_t ArmsMetricsEventGroupListSerializer::GetMeasureTimestamp(BatchedEvents& batchedEvents) {
    for (auto&& event : batchedEvents.mEvents) {
        auto timestamp = event->GetTimestamp();
        if (timestamp != 0) {
            return static_cast<int64_t>(timestamp);
        }
    }
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    int64_t current_time_millis = static_cast<int64_t>(millis);
    return current_time_millis;
}

void ArmsMetricsEventGroupListSerializer::ConvertBatchedEventsToMeasures(BatchedEvents&& batchedEvents,
                                                                         proto::MeasureBatch* measureBatch) {
    for (const auto& event : batchedEvents.mEvents) {
        auto measures = measureBatch->add_measures();
        auto measure = measures->add_measures();
        auto& eventData = event.Cast<MetricEvent>();
        for (auto& kv : eventData.GetTags()) {
            measures->mutable_labels()->insert({kv.first.to_string(), kv.second.to_string()});
        }
        eventData.GetTimestamp();
        std::string metricName(eventData.GetName().data(), eventData.GetName().size());
        measure->set_name(metricName);
        measure->set_valuetype(GetValueTypeByMetricName(metricName));
        if (eventData.Is<UntypedSingleValue>()) {
            auto value = eventData.GetValue<UntypedSingleValue>()->mValue;
            measure->set_value(value);
        }
        measure->set_unit(GetUnitByMetricName(metricName));
    }
}


void ArmsMetricsEventGroupListSerializer::ConvertEventsToMeasure(EventsContainer&& events, proto::Measures* measures) {
    for (const auto& event : events) {
        auto measure = measures->add_measures();
        auto& eventData = event.Cast<MetricEvent>();
        eventData.GetTimestamp();
        std::string metricName(eventData.GetName().data(), eventData.GetName().size());
        measure->set_name(metricName);
        measure->set_valuetype(GetValueTypeByMetricName(metricName));
        if (eventData.Is<UntypedSingleValue>()) {
            auto value = eventData.GetValue<UntypedSingleValue>()->mValue;
            measure->set_value(value);
        }
        measure->set_unit(GetUnitByMetricName(metricName));
    }
}

proto::EnumUnit ArmsMetricsEventGroupListSerializer::GetUnitByMetricName(std::string metricName) {
    auto it = metricUnitMap.find(metricName);
    if (it != metricUnitMap.end()) {
        return it->second;
    } else {
        return proto::EnumUnit::UNKNOWN;
    }
}

std::string ArmsMetricsEventGroupListSerializer::GetValueTypeByMetricName(std::string metricName) {
    auto it = metricValueTypeMap.find(metricName);
    if (it != metricValueTypeMap.end()) {
        return it->second;
    } else {
        return COUNT;
    }
}


std::string ArmsMetricsEventGroupListSerializer::GetIpFromTags(SizedMap& mTags) {
    auto& mTagsInner = mTags.mInner;
    auto it = mTagsInner.find("ip");
    if (it != mTagsInner.end()) {
        return it->second.to_string();
    } else {
        LOG_WARNING(sLogger, ("GetIpFromTags", "do not find ip, no tag"));
    }
    return "unkown";
}


std::string ArmsMetricsEventGroupListSerializer::GetAppIdFromTags(SizedMap& mTags) {
    auto& mTagsInner = mTags.mInner;
    auto it = mTagsInner.find("appId");
    if (it != mTagsInner.end()) {
        return it->second.to_string();
    } else {
        LOG_WARNING(sLogger, ("GetAppIdFromTags", "do not find appId, no tag!"));
    }
    return "unkown";
}


///////////////////////////////////////////////// Span Serializer /////////////////////////////////////////////////

bool ArmsSpanEventGroupListSerializer::Serialize(std::vector<BatchedEventsList>&& v,
                                                 std::string& res,
                                                 std::string& errorMsg) {
    LOG_INFO(sLogger, ("[Serialize] std::vector<BatchedEventsList> v.size ", v.size()));

    if (v.empty()) {
        return true;
    }

    TracesData traces_data;

    // set resource
    ResourceSpans* resource_spans = traces_data.add_resource_spans();
    auto resource = resource_spans->mutable_resource();

    auto scope_span = resource_spans->add_scope_spans();

    // TODO @qianlu.kk unnecessary
    auto scope = scope_span->mutable_scope();
    scope->set_name("hhh");
    std::map<StringView, StringView> group_resource;

    for (auto& batched_events_list : v) {
        for (auto& batch_events : batched_events_list) {
            if (group_resource.empty()) {
                group_resource = batch_events.mTags.mInner;
            } else {
                auto it1 = group_resource.find("arms.appId");
                auto it2 = batch_events.mTags.mInner.find("arms.appId");
                if (it1 != group_resource.end() && it2 != batch_events.mTags.mInner.end()) {
                    LOG_INFO(sLogger, ("[Span][Serialize] group arms.appId tags equal ? ", it1->second == it2->second));
                }
            }

            for (auto& event_ptr : batch_events.mEvents) {
                if (!event_ptr.Is<SpanEvent>())
                    continue;
                // !!! attention !!! @qianlu.kk
                // SpanEvent should not hold any tags because we don't have interface to extract thoes tags
                // so, all tags need to be stored in `batch_events::mTags`
                SpanEvent& span_event_ref = event_ptr.Cast<SpanEvent>();
                span_event_ref.GetEvents();

                // add to scope spans
                auto span = scope_span->add_spans();
                span->set_trace_id(std::string(span_event_ref.GetTraceId()));
                span->set_span_id(std::string(span_event_ref.GetSpanId()));
                span->set_parent_span_id(std::string(span_event_ref.GetParentSpanId()));
                span->set_kind(static_cast<Span_SpanKind>(span_event_ref.GetKind()));
                span->set_start_time_unix_nano(span_event_ref.GetStartTimeNs());
                span->set_end_time_unix_nano(span_event_ref.GetEndTimeNs());
                span->set_name(std::string(span_event_ref.GetName()));

                auto all_tags = span_event_ref.GetTags();

                // TODO @qianlu.kk this logic need to be move to upper ...
                // but protobuf may be NOT support memory reuse ...
                for (auto it : all_tags) {
                    // set attrs
                    auto attr = span->add_attributes();
                    attr->set_key(std::string(it.first));
                    auto attr_val = attr->mutable_value();
                    attr_val->set_string_value(std::string(it.second));
                }
            }
        }
    }
    for (auto& it : common_resources_) {
        auto attr = resource->add_attributes();
        attr->set_key(it.first);
        auto val = attr->mutable_value();
        val->set_string_value(it.second);
    }
    for (auto& it : group_resource) {
        auto attr = resource->add_attributes();
        attr->set_key(std::string(it.first));
        auto val = attr->mutable_value();
        val->set_string_value(std::string(it.second));
    }

    res = traces_data.SerializeAsString();

    return true;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace logtail