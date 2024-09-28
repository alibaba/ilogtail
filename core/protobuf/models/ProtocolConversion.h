#pragma once

#include "models/PipelineEventGroup.h"
#include "models/LogEvent.h"
#include "models/MetricEvent.h"
#include "models/SpanEvent.h"
#include "protobuf/models/pipeline_event_group.pb.h"
#include "protobuf/models/log_event.pb.h"
#include "protobuf/models/metric_event.pb.h"
#include "protobuf/models/span_event.pb.h"

namespace logtail {

bool TransferPBToPipelineEventGroup(const logtail::models::PipelineEventGroup& src, logtail::PipelineEventGroup& dst, std::string& errMsg);
bool TransferPBToLogEvent(const logtail::models::LogEvent& src, logtail::LogEvent& dst, std::string& errMsg);
bool TransferPBToMetricEvent(const logtail::models::MetricEvent& src, logtail::MetricEvent& dst, std::string& errMsg);
bool TransferPBToSpanEvent(const logtail::models::SpanEvent& src, logtail::SpanEvent& dst, std::string& errMsg);

bool TransferPipelineEventGroupToPB(const logtail::PipelineEventGroup& src, logtail::models::PipelineEventGroup& dst, std::string& errMsg);
bool TransferLogEventToPB(const logtail::LogEvent& src, logtail::models::LogEvent& dst, std::string& errMsg);
bool TransferMetricEventToPB(const logtail::MetricEvent& src, logtail::models::MetricEvent& dst, std::string& errMsg);
bool TransferSpanEventToPB(const logtail::SpanEvent& src, logtail::models::SpanEvent& dst, std::string& errMsg);


} // namespace logtail
