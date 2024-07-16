/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include "MetricEvent.h"
#include "unittest/Unittest.h"
#include "models/PipelineEventGroup.h"
#include "prometheus/TextParser.h"

using namespace std;

namespace logtail {

bool isDoubleEqual(double a, double b) {
    return fabs(a - b) < 0.000001;
}

class TextParserUnittest : public testing::Test {
public:
    void TestMetricEvent() const;
    void TestParseMultipleLines() const;
    void TestSampleRegex() const;
    void TestParseMetricWithTagsAndTimestamp() const;
    void TestParseMetricWithManyTags() const;
};

void TextParserUnittest::TestMetricEvent() const {
    const auto &srcBuf = make_shared<SourceBuffer>();
    auto eGroup = PipelineEventGroup(srcBuf);
    auto event = eGroup.AddMetricEvent();
    event->SetName("test_metric");
    event->SetValue(MetricValue(UntypedSingleValue{ 1.0 }));
    event->SetTimestamp(1234567890);
    event->SetTag(StringView("test_key"), StringView("test_value"));

    const auto &events = eGroup.GetEvents();
    APSARA_TEST_EQUAL(1UL, events.size());
    const auto &firstEvent = &events.front();
    const auto &firstMetric = firstEvent->Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric", firstMetric->GetName().data());
    const auto &metricValue = firstMetric->GetValue<UntypedSingleValue>();
    APSARA_TEST_EQUAL(1.0, metricValue->mValue);
    APSARA_TEST_EQUAL(1234567890, firstMetric->GetTimestamp());
    APSARA_TEST_STREQ("test_value", firstMetric->GetTag(logtail::StringView("test_key")).data());
}
UNIT_TEST_CASE(TextParserUnittest, TestMetricEvent)

void TextParserUnittest::TestSampleRegex() const {
    const auto &parser = TextParser();
    string name, labels, unwrapped_labels, value, suffix, timestamp;
    APSARA_TEST_TRUE(RE2::FullMatch(R"""(go_gc_duration_seconds{quantile="0"} 0)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("go_gc_duration_seconds", name.data());
    APSARA_TEST_STREQ("{quantile=\"0\"}", labels.data());
    APSARA_TEST_STREQ("quantile=\"0\"", unwrapped_labels.data());
    APSARA_TEST_STREQ("0", value.data());
    APSARA_TEST_STREQ("", suffix.data());
    APSARA_TEST_STREQ("", timestamp.data());

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(go_memstats_mallocs_total 4859)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("go_memstats_mallocs_total", name.data());
    APSARA_TEST_STREQ("", labels.data());
    APSARA_TEST_STREQ("", unwrapped_labels.data());
    APSARA_TEST_STREQ("4859", value.data());
    APSARA_TEST_STREQ("", suffix.data());
    APSARA_TEST_STREQ("", timestamp.data());

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(go_memstats_other_sys_bytes 1.030122e+06)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("go_memstats_other_sys_bytes", name.data());
    APSARA_TEST_STREQ("", labels.data());
    APSARA_TEST_STREQ("", unwrapped_labels.data());
    APSARA_TEST_STREQ("1.030122e+06", value.data());
    APSARA_TEST_STREQ("", suffix.data());
    APSARA_TEST_STREQ("", timestamp.data());

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(node_cpu_seconds_total{cpu="1",mode="idle"} 166606.17)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("node_cpu_seconds_total", name.data());
    APSARA_TEST_STREQ("{cpu=\"1\",mode=\"idle\"}", labels.data());
    APSARA_TEST_STREQ("cpu=\"1\",mode=\"idle\"", unwrapped_labels.data());
    APSARA_TEST_STREQ("166606.17", value.data());
    APSARA_TEST_STREQ("", suffix.data());
    APSARA_TEST_STREQ("", timestamp.data());

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(node_cpu_seconds_total{cpu="1",  mode="idle"} 166606.17)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("node_cpu_seconds_total", name.data());
    APSARA_TEST_STREQ("{cpu=\"1\",  mode=\"idle\"}", labels.data());
    APSARA_TEST_STREQ("cpu=\"1\",  mode=\"idle\"", unwrapped_labels.data());
    APSARA_TEST_STREQ("166606.17", value.data());
    APSARA_TEST_STREQ("", suffix.data());
    APSARA_TEST_STREQ("", timestamp.data());

    APSARA_TEST_FALSE(RE2::FullMatch(R"""(node_cpu_seconds_total{cpu="1",mode="idle"} 166606.17        )""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(container_blkio_device_usage_total{container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""} 9.9410452992e+10 1715829785083)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("container_blkio_device_usage_total", name.data());
    APSARA_TEST_STREQ(R"""({container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""})""", labels.data());
    APSARA_TEST_STREQ(R"""(container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod="")""", unwrapped_labels.data());
    APSARA_TEST_STREQ("9.9410452992e+10", value.data());
    APSARA_TEST_STREQ(" 1715829785083", suffix.data());
    APSARA_TEST_STREQ("1715829785083", timestamp.data());

    APSARA_TEST_TRUE(RE2::FullMatch(R"""(container_blkio_device_usage_total{container="", device="/dev/nvme0n1",  id="/"  ,image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""} 9.9410452992e+10    1715829785083)""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
    APSARA_TEST_STREQ("container_blkio_device_usage_total", name.data());
    APSARA_TEST_STREQ(R"""({container="", device="/dev/nvme0n1",  id="/"  ,image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""})""", labels.data());
    APSARA_TEST_STREQ(R"""(container="", device="/dev/nvme0n1",  id="/"  ,image="",major="259",minor="0",name="",namespace="",operation="Async",pod="")""", unwrapped_labels.data());
    APSARA_TEST_STREQ("9.9410452992e+10", value.data());
    APSARA_TEST_STREQ("    1715829785083", suffix.data());
    APSARA_TEST_STREQ("1715829785083", timestamp.data());

    APSARA_TEST_FALSE(RE2::FullMatch(R"""(container_blkio_device_usage_total{container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""} 9.9410452992e+10 1715829785083       )""", parser.mSampleRegex, RE2::Arg(&name), RE2::Arg(&labels), RE2::Arg(&unwrapped_labels), RE2::Arg(&value), RE2::Arg(&suffix), RE2::Arg(&timestamp)));
}
UNIT_TEST_CASE(TextParserUnittest, TestSampleRegex)

void TextParserUnittest::TestParseMultipleLines() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(R"""(
# begin

test_metric1{k1="v1", k2="v2"} 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k2="v2", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k2="v2", } 9.9410452992e+10 1715829785083

# end
    )""");
    const auto &events = &eGroup.GetEvents();
    APSARA_TEST_EQUAL(8UL, events->size());
}
UNIT_TEST_CASE(TextParserUnittest, TestParseMultipleLines)

void TextParserUnittest::TestParseMetricWithTagsAndTimestamp() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(R"""(
    test_metric{k1="v1", k2="v2"} 9.9410452992e+10 1715829785083
    test_metric2{k1="v1", k2="v2"} 2.0 1715829785083
    test_metric3{k1="v1",k2="v2"} 4.2 1715829785083
    )""");

    // test_metric
    const auto &events = &eGroup.GetEvents();
    APSARA_TEST_EQUAL(3UL, events->size());
    const auto &event = events->front();
    const auto &metric = event.Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric", metric->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric->GetTimestamp());
    APSARA_TEST_TRUE(isDoubleEqual(9.9410452992e+10, metric->GetValue<UntypedSingleValue>()->mValue));
    APSARA_TEST_STREQ("v1", metric->GetTag("k1").data());
    APSARA_TEST_STREQ("v2", metric->GetTag("k2").data());

    // test_metric2
    auto &event2 = events->at(1);
    const auto &metric2 = event2.Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric2", metric2->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric2->GetTimestamp());
    APSARA_TEST_TRUE(isDoubleEqual(2.0, metric2->GetValue<UntypedSingleValue>()->mValue));
    APSARA_TEST_STREQ("v1", metric2->GetTag("k1").data());
    APSARA_TEST_STREQ("v2", metric2->GetTag("k2").data());

    // test_metric3
    auto &event3 = events->at(2);
    const auto &metric3 = event3.Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric3", metric3->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric3->GetTimestamp());
    APSARA_TEST_TRUE(isDoubleEqual(4.2, metric3->GetValue<UntypedSingleValue>()->mValue));
    APSARA_TEST_STREQ("v1", metric3->GetTag("k1").data());
    APSARA_TEST_STREQ("v2", metric3->GetTag("k2").data());
}
UNIT_TEST_CASE(TextParserUnittest, TestParseMetricWithTagsAndTimestamp)

void TextParserUnittest::TestParseMetricWithManyTags() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(R"""(container_blkio_device_usage_total{container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""} 9.9410452992e+10 1715829785083)""");
    const auto &events = &eGroup.GetEvents();
    APSARA_TEST_EQUAL(1UL, events->size());
    const auto &event = events->front();
    const auto &metric = event.Get<MetricEvent>();
    APSARA_TEST_STREQ("container_blkio_device_usage_total", metric->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric->GetTimestamp());
    APSARA_TEST_TRUE(isDoubleEqual(9.9410452992e+10, metric->GetValue<UntypedSingleValue>()->mValue));

    // TODO: assert number of tags
    APSARA_TEST_STREQ("", metric->GetTag("container").data());
    APSARA_TEST_STREQ("/dev/nvme0n1", metric->GetTag("device").data());
    APSARA_TEST_STREQ("/", metric->GetTag("id").data());
    APSARA_TEST_STREQ("", metric->GetTag("image").data());
    APSARA_TEST_STREQ("259", metric->GetTag("major").data());
    APSARA_TEST_STREQ("0", metric->GetTag("minor").data());
    APSARA_TEST_STREQ("", metric->GetTag("name").data());
    APSARA_TEST_STREQ("", metric->GetTag("namespace").data());
    APSARA_TEST_STREQ("Async", metric->GetTag("operation").data());
    APSARA_TEST_STREQ("", metric->GetTag("pod").data());
}
UNIT_TEST_CASE(TextParserUnittest, TestParseMetricWithManyTags)

} // namespace logtail

UNIT_TEST_MAIN
