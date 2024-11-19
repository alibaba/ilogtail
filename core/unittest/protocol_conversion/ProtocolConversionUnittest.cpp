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

    void TestTransferLogEventToPB() const;
    void TestTransferMetricEventToPB() const;
    void TestTransferMetricEventToPBFailEmptyValue() const;
    void TestTransferSpanEventToPB() const;
    void TestTransferPipelineEventToPBLog() const;
    void TestTransferPipelineEventToPBMetric() const;
    void TestTransferPipelineEventToPBSpan() const;
    void TestTransferPipelineEventToPBFailEmptyEvents() const;
    void TestTransferPipelineEventToPBFailDiffEvents() const;
};

void ProtocolConversionUnittest::TestTransferPBToLogEvent() const {
    uint64_t timestamp = 1622547800000000000; 
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    std::string level = "INFO";
    uint64_t fileoffset = 12345;
    uint64_t rawsize = 67890;

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
    uint64_t timestamp = 1622547800000000000;
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
    uint64_t timestamp = 1622547800000000000; 
    std::string traceId = "trace_id";
    std::string spanId = "span_id";
    std::string traceState = "trace_state";
    std::string parentSpanId = "parent_span_id";
    std::string name = "span_name";
    logtail::models::SpanEvent::SpanKind kind = logtail::models::SpanEvent::SpanKind::SpanEvent_SpanKind_INTERVAL;
    uint64_t startTime = 1622547800000000000;
    uint64_t endTime = 1622547801000000000;
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
    APSARA_TEST_EQUAL(dst->GetEvents().size(), 1U);
    const auto& dstInnerEvent = dst->GetEvents()[0];
    APSARA_TEST_EQUAL(dstInnerEvent.GetTimestampNs(), 1622547802000000000);
    APSARA_TEST_EQUAL(dstInnerEvent.GetName(), "inner_event_name");
    APSARA_TEST_EQUAL(dstInnerEvent.GetTag("inner_key1"), "inner_value1");

    // check span links
    APSARA_TEST_EQUAL(dst->GetLinks().size(), 1U);
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
    size_t cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (size_t i = 0; i < cnt; i++)
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
    for (size_t i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::LOG);
    }
}

void ProtocolConversionUnittest::TestTransferPBToPipelineEventGroupMetric() const {
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    size_t cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (size_t i = 0; i < cnt; i++) {
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
    for (size_t i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::METRIC);
    }
}

void ProtocolConversionUnittest::TestTransferPBToPipelineEventGroupSpan() const {
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    size_t cnt = 10;

    logtail::models::PipelineEventGroup src;
    // TODO: add metadata
    src.mutable_tags()->insert({key1, value1});
    src.mutable_tags()->insert({key2, value2});
    for (size_t i = 0; i < cnt; i++)
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
    for (size_t i = 0; i < cnt; i++) {
        APSARA_TEST_EQUAL(dst->GetEvents()[i]->GetType(), PipelineEvent::Type::SPAN);
    }
}

void ProtocolConversionUnittest::TestTransferLogEventToPB() const {
    uint64_t timestamp = 1622547800; 
    uint64_t timestampNs = 500000000; 
    std::string key1 = "key1";
    std::string value1 = "value1";
    std::string key2 = "key2";
    std::string value2 = "value2";
    std::string level = "INFO";
    uint64_t fileoffset = 12345;
    uint64_t rawsize = 67890;

    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto src = group->CreateLogEvent();

    src->SetTimestamp(timestamp, timestampNs);
    src->SetContent(key1, value1);
    src->SetContent(key2, value2);
    src->SetLevel(level);
    src->SetPosition(fileoffset, rawsize);

    logtail::models::LogEvent dst;
    std::string errMsg;

    bool result = TransferLogEventToPB(*src, dst, errMsg);

    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst.timestamp(), timestamp * 1000000000 + timestampNs);
    APSARA_TEST_EQUAL(dst.contents_size(), 2);
    APSARA_TEST_EQUAL(dst.contents(0).key(), key1);
    APSARA_TEST_EQUAL(dst.contents(0).value(), value1);
    APSARA_TEST_EQUAL(dst.contents(1).key(), key2);
    APSARA_TEST_EQUAL(dst.contents(1).value(), value2);
    APSARA_TEST_EQUAL(dst.level(), level);
    APSARA_TEST_EQUAL(dst.fileoffset(), fileoffset);
    APSARA_TEST_EQUAL(dst.rawsize(), rawsize);
}

void ProtocolConversionUnittest::TestTransferMetricEventToPB() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto src = group->CreateMetricEvent();
    logtail::models::MetricEvent dst;
    std::string errMsg;

    // Set timestamp
    src->SetTimestamp(1627846261, 123456789);

    // Set name
    src->SetName("metric_name");

    // Set value
    logtail::UntypedSingleValue value{42.0};
    src->SetValue(value);

    // Set tags
    std::string tag1 = "tag1";
    std::string tag2 = "tag2";
    std::string value1 = "value1";
    std::string value2 = "value2";
    src->SetTag(tag1, value1);
    src->SetTag(tag2, value2);

    // Perform conversion
    bool result = TransferMetricEventToPB(*src, dst, errMsg);

    // Validate result
    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst.timestamp(), 1627846261123456789);
    APSARA_TEST_EQUAL(dst.name(), "metric_name");
    APSARA_TEST_EQUAL(dst.untypedsinglevalue().value(), 42.0);
    APSARA_TEST_EQUAL(dst.tags().at("tag1"), "value1");
    APSARA_TEST_EQUAL(dst.tags().at("tag2"), "value2");
}

void ProtocolConversionUnittest::TestTransferMetricEventToPBFailEmptyValue() const {

}

void ProtocolConversionUnittest::TestTransferSpanEventToPB() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> group(new PipelineEventGroup(sourceBuffer));
    auto src = group->CreateSpanEvent();
    logtail::models::SpanEvent dst;
    std::string errMsg;

    // Set timestamp
    src->SetTimestamp(1627846261, 123456789);

    // Set trace details
    src->SetTraceId("trace_id");
    src->SetSpanId("span_id");
    src->SetTraceState("trace_state");
    src->SetParentSpanId("parent_span_id");
    src->SetName("span_name");
    src->SetKind(logtail::SpanEvent::Kind::Internal);
    src->SetStartTimeNs(1627846261123456789);
    src->SetEndTimeNs(1627846262123456789);

    // Set tags
    std::string tag1 = "tag1";
    std::string tag2 = "tag2";
    std::string value1 = "value1";
    std::string value2 = "value2";
    src->SetTag(tag1, value1);
    src->SetTag(tag2, value2);

    // Set inner events
    auto innerEvent = src->AddEvent();
    innerEvent->SetTimestampNs(1627846261123456789);
    innerEvent->SetName("inner_event_name");
    std::string innerTag1 = "inner_tag1";
    std::string innerValue1 = "inner_value1";
    innerEvent->SetTag(innerTag1, innerValue1);

    // Set span links
    auto spanLink = src->AddLink();
    spanLink->SetTraceId("link_trace_id");
    spanLink->SetSpanId("link_span_id");
    spanLink->SetTraceState("link_trace_state");
    std::string linkTag1 = "link_tag1";
    std::string linkValue1 = "link_value1";
    spanLink->SetTag(linkTag1, linkValue1);

    // Set scope tags
    std::string scopeTag1 = "scope_tag1";
    std::string scopeTag2 = "scope_tag2";
    std::string scopeValue1 = "scope_value1";
    std::string scopeValue2 = "scope_value2";
    src->SetScopeTag(scopeTag1, scopeValue1);
    src->SetScopeTag(scopeTag2, scopeValue2);

    // Perform conversion
    bool result = TransferSpanEventToPB(*src, dst, errMsg);

    // Validate result
    APSARA_TEST_TRUE(result);
    APSARA_TEST_EQUAL(dst.timestamp(), 1627846261123456789);
    APSARA_TEST_EQUAL(dst.traceid(), "trace_id");
    APSARA_TEST_EQUAL(dst.spanid(), "span_id");
    APSARA_TEST_EQUAL(dst.tracestate(), "trace_state");
    APSARA_TEST_EQUAL(dst.parentspanid(), "parent_span_id");
    APSARA_TEST_EQUAL(dst.name(), "span_name");
    APSARA_TEST_EQUAL(dst.kind(), logtail::models::SpanEvent_SpanKind::SpanEvent_SpanKind_INTERVAL);
    APSARA_TEST_EQUAL(dst.starttime(), 1627846261123456789);
    APSARA_TEST_EQUAL(dst.endtime(), 1627846262123456789);
    APSARA_TEST_EQUAL(dst.tags().at("tag1"), "value1");
    APSARA_TEST_EQUAL(dst.tags().at("tag2"), "value2");

    APSARA_TEST_EQUAL(dst.events_size(), 1);
    APSARA_TEST_EQUAL(dst.events(0).timestamp(), 1627846261123456789);
    APSARA_TEST_EQUAL(dst.events(0).name(), "inner_event_name");
    APSARA_TEST_EQUAL(dst.events(0).tags().at("inner_tag1"), "inner_value1");

    APSARA_TEST_EQUAL(dst.links_size(), 1);
    APSARA_TEST_EQUAL(dst.links(0).traceid(), "link_trace_id");
    APSARA_TEST_EQUAL(dst.links(0).spanid(), "link_span_id");
    APSARA_TEST_EQUAL(dst.links(0).tracestate(), "link_trace_state");
    APSARA_TEST_EQUAL(dst.links(0).tags().at("link_tag1"), "link_value1");

    APSARA_TEST_EQUAL(dst.scopetags().at("scope_tag1"), "scope_value1");
    APSARA_TEST_EQUAL(dst.scopetags().at("scope_tag2"), "scope_value2");
}

void ProtocolConversionUnittest::TestTransferPipelineEventToPBLog() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> src(new PipelineEventGroup(sourceBuffer));
    logtail::models::PipelineEventGroup dst;
    std::string errMsg;

    // Add log events
    size_t cnt = 10;
    for (size_t i = 0; i < cnt; i++) {
        src->AddLogEvent();
    }

    // Perform conversion
    bool result = TransferPipelineEventGroupToPB(*src, dst, errMsg);

    // Validate result
    ASSERT_TRUE(result);
    ASSERT_EQ(dst.logs().events_size(), cnt);
}

void ProtocolConversionUnittest::TestTransferPipelineEventToPBMetric() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> src(new PipelineEventGroup(sourceBuffer));
    logtail::models::PipelineEventGroup dst;
    std::string errMsg;

    // Add log events
    size_t cnt = 10;
    for (size_t i = 0; i < cnt; i++) {
        auto metricEvent = src->AddMetricEvent();
        metricEvent->SetValue(logtail::UntypedSingleValue{1});
    }

    // Perform conversion
    bool result = TransferPipelineEventGroupToPB(*src, dst, errMsg);

    // Validate result
    ASSERT_TRUE(result);
    ASSERT_EQ(dst.metrics().events_size(), cnt);

}

void ProtocolConversionUnittest::TestTransferPipelineEventToPBSpan() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> src(new PipelineEventGroup(sourceBuffer));
    logtail::models::PipelineEventGroup dst;
    std::string errMsg;

    // Add log events
    size_t cnt = 10;
    for (size_t i = 0; i < cnt; i++) {
        src->AddSpanEvent();
    }

    // Perform conversion
    bool result = TransferPipelineEventGroupToPB(*src, dst, errMsg);

    // Validate result
    ASSERT_TRUE(result);
    ASSERT_EQ(dst.spans().events_size(), cnt);
}

void ProtocolConversionUnittest::TestTransferPipelineEventToPBFailEmptyEvents() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> src(new PipelineEventGroup(sourceBuffer));
    logtail::models::PipelineEventGroup dst;
    std::string errMsg;

    // Perform conversion
    bool result = TransferPipelineEventGroupToPB(*src, dst, errMsg);

    // Validate result
    APSARA_TEST_FALSE(result);
    APSARA_TEST_EQUAL(errMsg, "error transfer PipelineEventGroup to PB: events empty");
}

void ProtocolConversionUnittest::TestTransferPipelineEventToPBFailDiffEvents() const {
    shared_ptr<SourceBuffer> sourceBuffer(new SourceBuffer);
    unique_ptr<PipelineEventGroup> src(new PipelineEventGroup(sourceBuffer));
    logtail::models::PipelineEventGroup dst;
    std::string errMsg;

    // Add events
    src->AddLogEvent();
    auto metricEvent = src->AddMetricEvent();
    metricEvent->SetValue(logtail::UntypedSingleValue{1});
    src->AddSpanEvent();

    // Perform conversion
    bool result = TransferPipelineEventGroupToPB(*src, dst, errMsg);

    // Validate result
    ASSERT_FALSE(result);
}

UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToLogEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToMetricEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToSpanEvent)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupLog)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupMetric)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPBToPipelineEventGroupSpan)

UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferLogEventToPB)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferMetricEventToPB)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferMetricEventToPBFailEmptyValue)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferSpanEventToPB)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPipelineEventToPBLog)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPipelineEventToPBMetric)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPipelineEventToPBSpan)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPipelineEventToPBFailEmptyEvents)
UNIT_TEST_CASE(ProtocolConversionUnittest, TestTransferPipelineEventToPBFailDiffEvents)

} // namespace logtail

UNIT_TEST_MAIN
