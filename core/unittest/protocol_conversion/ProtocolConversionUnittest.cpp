// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "protobuf/models/ProtocolConversion.h"
#include "unittest/Unittest.h"
#include <iostream>

using namespace std;

namespace logtail {

class ProtocolConversionUnittest : public ::testing::Test {
public:
    void TestTransferPBToLogEvent() const;
    void TestTransferPBToMetricEvent() const;
    void TestTransferPBToSpanEvent() const;
    void TestTransferPBToPipelineEventGroupLog() const;
    void TestTransferPBToPipelineEventGroupMetric() const;
    void TestTransferPBToPipelineEventGroupSpan() const;
};

void ProtocolConversionUnittest::TestTransferPBToLogEvent() const {
    int64_t timestamp = 1622547800000000000; 
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    std::string level = "INFO";
    int64_t fileoffset = 12345;
    int64_t rawsize = 67890;

    logtail::models::LogEvent src;
    src.set_timestamp(timestamp);
    auto* content1 = src.add_contents();
    content1->set_key(key1);
    content1->set_value(value1);
    auto* content2 = src.add_contents();
    content2->set_key(key2);
    content2->set_value(value2);
    src.set_level(level);
    src.set_fileoffset(fileoffset);
    src.set_rawsize(rawsize);

    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto dst = group->CreateLogEvent();
    std::string errMsg;

    bool result = TransferPBToLogEvent(src, *dst, errMsg);

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTimestamp(), timestamp / 1000000000);
    APSARA_TEST_EQUAL(dst->GetTimestampNanosecond().value_or(0), timestamp % 1000000000);
    APSARA_TEST_EQUAL(dst->GetContent(key1), value1);
    APSARA_TEST_EQUAL(dst->GetContent(key2), value2);
    APSARA_TEST_EQUAL(dst->GetLevel(), level);
    APSARA_TEST_EQUAL(dst->GetPosition().first, fileoffset);
    APSARA_TEST_EQUAL(dst->GetPosition().second, rawsize);
}

void ProtocolConversionUnittest::TestTransferPBToMetricEvent() const {
    int64_t timestamp = 1622547800000000000;
    std::string name = "metric_name";
    double value = 123.45;
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";

    logtail::models::MetricEvent src;
    src.set_timestamp(timestamp);
    src.set_name(name);
    auto untypedValue = src.mutable_untypedsinglevalue();
    untypedValue->set_value(value);
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});

    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto dst = group->CreateMetricEvent();
    std::string errMsg;

    bool result = TransferPBToMetricEvent(src, *dst, errMsg);

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTimestamp(), timestamp / 1000000000);
    APSARA_TEST_EQUAL(dst->GetTimestampNanosecond().value_or(0), timestamp % 1000000000);
    APSARA_TEST_EQUAL(dst->GetName(), name);
    auto untypedSingleValue = dst->GetValue<logtail::UntypedSingleValue>();
    APSARA_TEST_NOT_EQUAL(untypedSingleValue, nullptr);
    APSARA_TEST_EQUAL(untypedSingleValue->mValue, value);
    APSARA_TEST_EQUAL(dst->GetTag(key1), value1);
    APSARA_TEST_EQUAL(dst->GetTag(key2), value2);
}

void ProtocolConversionUnittest::TestTransferPBToSpanEvent() const {
    int64_t timestamp = 1622547800000000000; 
    std::string traceId = "trace_id";
    std::string spanId = "span_id";
    std::string traceState = "trace_state";
    std::string parentSpanId = "parent_span_id";
    std::string name = "span_name";
    logtail::models::SpanEvent::SpanKind kind = logtail::models::SpanEvent::SpanKind::SpanEvent_SpanKind_INTERVAL;
    int64_t startTime = 1622547800000000000;
    int64_t endTime = 1622547801000000000;
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    logtail::models::SpanEvent::StatusCode status = logtail::models::SpanEvent::StatusCode::SpanEvent_StatusCode_Ok;

    logtail::models::SpanEvent src;
    src.set_timestamp(timestamp);
    src.set_traceid(traceId);
    src.set_spanid(spanId);
    src.set_tracestate(traceState);
    src.set_parentspanid(parentSpanId);
    src.set_name(name);
    src.set_kind(kind);
    src.set_starttime(startTime);
    src.set_endtime(endTime);
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    src.set_status(status);

    // inner events
    auto innerEvent = src.add_events();
    innerEvent->set_timestamp(1622547802000000000);
    innerEvent->set_name("inner_event_name");
    innerEvent->mutable_tags()->insert({"inner_key1", "inner_value1"});

    // span links
    auto spanLink = src.add_links();
    spanLink->set_traceid("link_trace_id");
    spanLink->set_spanid("link_span_id");
    spanLink->set_tracestate("link_trace_state");
    spanLink->mutable_tags()->insert({"link_key1", "link_value1"});

    // scope tags
    src.mutable_scopetags()->insert({"scope_key1", "scope_value1"});

    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto dst = group->CreateSpanEvent();
    std::string errMsg;

    // transfer
    bool result = TransferPBToSpanEvent(src, *dst, errMsg);

    // check
    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTimestamp(), timestamp / 1000000000);
    APSARA_TEST_EQUAL(dst->GetTimestampNanosecond().value_or(0), timestamp % 1000000000);
    APSARA_TEST_EQUAL(dst->GetTraceId(), traceId);
    APSARA_TEST_EQUAL(dst->GetSpanId(), spanId);
    APSARA_TEST_EQUAL(dst->GetTraceState(), traceState);
    APSARA_TEST_EQUAL(dst->GetParentSpanId(), parentSpanId);
    APSARA_TEST_EQUAL(dst->GetName(), name);
    APSARA_TEST_EQUAL(dst->GetKind(), static_cast<logtail::SpanEvent::Kind>(kind));
    APSARA_TEST_EQUAL(dst->GetStartTimeNs(), startTime);
    APSARA_TEST_EQUAL(dst->GetEndTimeNs(), endTime);
    APSARA_TEST_EQUAL(dst->GetTag(key1), value1);
    APSARA_TEST_EQUAL(dst->GetTag(key2), value2);
    APSARA_TEST_EQUAL(dst->GetStatus(), static_cast<logtail::SpanEvent::StatusCode>(status));

    // check inner events
    APSARA_TEST_EQUAL(dst->GetEvents().size(), 1);
    const auto& dstInnerEvent = dst->GetEvents()[0];
    APSARA_TEST_EQUAL(dstInnerEvent.GetTimestampNs(), 1622547802000000000);
    APSARA_TEST_EQUAL(dstInnerEvent.GetName(), "inner_event_name");
    APSARA_TEST_EQUAL(dstInnerEvent.GetTag("inner_key1"), "inner_value1");

    // check span links
    APSARA_TEST_EQUAL(dst->GetLinks().size(), 1);
    const auto& dstLink = dst->GetLinks()[0];
    APSARA_TEST_EQUAL(dstLink.GetTraceId(), "link_trace_id");
    APSARA_TEST_EQUAL(dstLink.GetSpanId(), "link_span_id");
    APSARA_TEST_EQUAL(dstLink.GetTraceState(), "link_trace_state");
    APSARA_TEST_EQUAL(dstLink.GetTag("link_key1"), "link_value1");

    // check scope tags
    APSARA_TEST_EQUAL(dst->GetScopeTag("scope_key1"), "scope_value1");
}

void ProtocolConversionUnittest::TestTransferPBToPipelineEventGroupLog() const {
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    int cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (int i = 0; i < cnt; i++)
        src.mutable_logs()->add_events();
    
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> dst(new PipelineEventGroup(sourceBuffer));
    std::string errMsg;

    // transfer
    bool result = TransferPBToPipelineEventGroup(src, *dst, errMsg);
    std::cout << errMsg << std::endl;

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTag(key1), value1);
    APSARA_TEST_EQUAL(dst->GetTag(key2), value2);
    APSARA_TEST_EQUAL(dst->GetEvents().size(), cnt);
    for (int i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::LOG);
    }
}

void ProtocolConversionUnittest::TestTransferPBToPipelineEventGroupMetric() const {
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    int cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (int i = 0; i < cnt; i++) {
        auto event = src.mutable_metrics()->add_events();
        event->mutable_untypedsinglevalue()->set_value(1.0f);
    }
    
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> dst(new PipelineEventGroup(sourceBuffer));
    std::string errMsg;

    // transfer
    bool result = TransferPBToPipelineEventGroup(src, *dst, errMsg);
    std::cout << errMsg << std::endl;

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTag(key1), value1);
    APSARA_TEST_EQUAL(dst->GetTag(key2), value2);
    APSARA_TEST_EQUAL(dst->GetEvents().size(), cnt);
    for (int i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::METRIC);
    }
}

void ProtocolConversionUnittest::TestTransferPBToPipelineEventGroupSpan() const {
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    int cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (int i = 0; i < cnt; i++)
        src.mutable_spans()->add_events();
    
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> dst(new PipelineEventGroup(sourceBuffer));
    std::string errMsg;

    // transfer
    bool result = TransferPBToPipelineEventGroup(src, *dst, errMsg);
    std::cout << errMsg << std::endl;

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst->GetTag(key1), value1);
    APSARA_TEST_EQUAL(dst->GetTag(key2), value2);
    APSARA_TEST_EQUAL(dst->GetEvents().size(), cnt);
    for (int i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::SPAN);
    }
}

UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToLogEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToMetricEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToSpanEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupLog)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupMetric)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupSpan)

} // namespace logtail

UNIT_TEST_MAIN
