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
#include "models/PipelineEventGroup.h"
#include "prometheus/labels/TextParser.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

bool IsDoubleEqual(double a, double b) {
    return fabs(a - b) < 0.000001;
}

class TextParserUnittest : public testing::Test {
public:
    void TestParseMultipleLines() const;
    void TestParseMetricWithTagsAndTimestamp() const;
    void TestParseMetricWithManyTags() const;

    void TestParseFaliure();
    void TestParseSuccess();
};

void TextParserUnittest::TestParseMultipleLines() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(R"""(
# begin

test_metric1{k1="v1", k2="v 1.0
  test_metric2{k1="v1", k2="v2"} 2.0 1234567890
test_metric3{k1="v1",k2="v2"} 9.9410452992e+10
  test_metric4{k1="v1",k2="v2"} 9.9410452992e+10 1715829785083
  test_metric5{k1="v1", k2="v2" } 9.9410452992e+10 1715829785083
test_metric6{k1="v1",k2="v2",} 9.9410452992e+10 1715829785083
test_metric7{k1="v1",k2="v2", } 9.9410452992e+10 1715829785083  
test_metric8{k1="v1", k2="v2", } 9.9410452992e+10 1715829785083

# end
    )""",
                                     0);
    const auto& events = &eGroup.GetEvents();
    APSARA_TEST_EQUAL(7UL, events->size());
}
UNIT_TEST_CASE(TextParserUnittest, TestParseMultipleLines)

void TextParserUnittest::TestParseMetricWithTagsAndTimestamp() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(R"""(
    test_metric{k1="v1", k2="v2"} 9.9410452992e+10 1715829785083
    test_metric2{k1="v1", k2="v2"} 2.0 1715829785083
    test_metric3{k1="v1",k2="v2"} 4.2 92233720368547758080000
    )""",
                                     0);

    // test_metric
    const auto& events = &eGroup.GetEvents();
    const auto& event = events->front();
    const auto& metric = event.Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric", metric->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric->GetTimestamp());
    APSARA_TEST_TRUE(IsDoubleEqual(9.9410452992e+10, metric->GetValue<UntypedSingleValue>()->mValue));
    APSARA_TEST_STREQ("v1", metric->GetTag("k1").data());
    APSARA_TEST_STREQ("v2", metric->GetTag("k2").data());

    // test_metric2
    const auto& event2 = events->at(1);
    const auto& metric2 = event2.Get<MetricEvent>();
    APSARA_TEST_STREQ("test_metric2", metric2->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric2->GetTimestamp());
    APSARA_TEST_TRUE(IsDoubleEqual(2.0, metric2->GetValue<UntypedSingleValue>()->mValue));
    APSARA_TEST_STREQ("v1", metric2->GetTag("k1").data());
    APSARA_TEST_STREQ("v2", metric2->GetTag("k2").data());

    // test_metric3 is not generated because of timestamp overflow
    APSARA_TEST_EQUAL(2UL, events->size());
}
UNIT_TEST_CASE(TextParserUnittest, TestParseMetricWithTagsAndTimestamp)

void TextParserUnittest::TestParseMetricWithManyTags() const {
    auto parser = TextParser();
    const auto eGroup = parser.Parse(
        R"""(container_blkio_device_usage_total{container="",device="/dev/nvme0n1",id="/",image="",major="259",minor="0",name="",namespace="",operation="Async",pod=""} 9.9410452992e+10 1715829785083)""",
        1715829785083);
    const auto& events = &eGroup.GetEvents();
    APSARA_TEST_EQUAL(1UL, events->size());
    const auto& event = events->front();
    const auto& metric = event.Get<MetricEvent>();
    APSARA_TEST_STREQ("container_blkio_device_usage_total", metric->GetName().data());
    APSARA_TEST_EQUAL(1715829785, metric->GetTimestamp());
    APSARA_TEST_TRUE(IsDoubleEqual(9.9410452992e+10, metric->GetValue<UntypedSingleValue>()->mValue));

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

void TextParserUnittest::TestParseFaliure() {
    auto f = [](const std::string& content) {
        TextParser parser;
        PipelineEventGroup eGroup = parser.Parse(content, 0);
        APSARA_TEST_EQUAL(0UL, eGroup.GetEvents().size());
    };

    // Empty lines and comments
    f("");
    f(" ");
    f("\t");
    f("\t  \r");
    f("\t\t  \n\n  # foobar");
    f("#foobar");
    f("#foobar\n");

    // invalid tags
    f("a{");
    f("a { ");
    f("a {foo");
    f("a {foo} 3");
    f("a {foo  =");
    f("a {foo  =\"bar");
    f("a {foo  =\"b\\ar");
    f("a {foo  = \"bar\"");
    f("a {foo  =\"bar\",");
    f("a {foo  =\"bar\" , ");
    f("a {foo  =\"bar\" , baz } 2");

    // Invalid tags - see https://github.com/VictoriaMetrics/VictoriaMetrics/issues/4284
    f(R"(a{"__name__":"upsd_time_left_ns","host":"myhost", "status_OB":"true"} 12)");
    f(R"(a{host:"myhost"} 12)");
    f(R"(a{host:"myhost",foo="bar"} 12)");

    // Empty metric name
    f(R"({foo="bar"})");

    // Invalid quotes for label value
    f(R"({foo='bar'} 23)");
    f(R"({foo=`bar`} 23");

    // Missing value
    f("aaa");   
    f(" aaa");
    f(" aaa ");
    f(" aaa   \n");
    f(R"( aa{foo="bar"}   )"
      + std::string("\n"));

    // Invalid value
    f("foo bar");
    f("foo bar 124");

    // Invalid timestamp
    f("foo 123 bar");
}
UNIT_TEST_CASE(TextParserUnittest, TestParseFaliure)

void TextParserUnittest::TestParseSuccess() {
    TextParser parser;
    string rawData;
    // single value
    rawData = "foobar 123";
    auto res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "foobar");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 123.0));

    rawData = "foobar 123.456 789\n";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "foobar");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 123.456));
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetTimestampNanosecond(), 789000000);

    rawData = R"(
    # TYPE cassandra_token_ownership_ratio gauge
cassandra_token_ownership_ratio 78.9)";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "cassandra_token_ownership_ratio");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 78.9));

    // `#` char in label value
    rawData = R"(foo{bar="#1 az"} 24)";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "foo");
    APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("bar").data(), "#1 az");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 24.0));

    // Incorrectly escaped backlash. This is real-world case, which must be supported.
    rawData = R"(mssql_sql_server_active_transactions_sec{loginname="domain\somelogin",env="develop"} 56)";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "mssql_sql_server_active_transactions_sec");
    APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("loginname").data(), "domain\\somelogin");
    APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("env").data(), "develop");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 56.0));

    rawData = R"(foo_bucket{le="10",a="#b"} 17)";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "foo_bucket");
    APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("le").data(), "10");
    APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("a").data(), "#b");
    APSARA_TEST_TRUE(
        IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, 17.0));

    // "Infinity" word - this has been added in OpenMetrics.
    // See https://github.com/OpenObservability/OpenMetrics/blob/master/OpenMetrics.md
    // Checks for https://github.com/VictoriaMetrics/VictoriaMetrics/issues/924
    rawData = R"(foo Infinity
		bar +Infinity
		baz -infinity
		aaa +inf
		bbb -INF
		ccc INF)";
    res = parser.Parse(rawData, 0);
    APSARA_TEST_EQUAL(res.GetEvents().size(), 6UL);
    APSARA_TEST_EQUAL(res.GetEvents()[0].Cast<MetricEvent>().GetName(), "foo");
    APSARA_TEST_EQUAL(res.GetEvents()[0].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      std::numeric_limits<double>::infinity());
    APSARA_TEST_EQUAL(res.GetEvents()[1].Cast<MetricEvent>().GetName(), "bar");
    APSARA_TEST_EQUAL(res.GetEvents()[1].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      std::numeric_limits<double>::infinity());
    APSARA_TEST_EQUAL(res.GetEvents()[2].Cast<MetricEvent>().GetName(), "baz");
    APSARA_TEST_EQUAL(res.GetEvents()[2].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      -std::numeric_limits<double>::infinity());
    APSARA_TEST_EQUAL(res.GetEvents()[3].Cast<MetricEvent>().GetName(), "aaa");
    APSARA_TEST_EQUAL(res.GetEvents()[3].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      std::numeric_limits<double>::infinity());
    APSARA_TEST_EQUAL(res.GetEvents()[4].Cast<MetricEvent>().GetName(), "bbb");
    APSARA_TEST_EQUAL(res.GetEvents()[4].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      -std::numeric_limits<double>::infinity());
    APSARA_TEST_EQUAL(res.GetEvents()[5].Cast<MetricEvent>().GetName(), "ccc");
    APSARA_TEST_EQUAL(res.GetEvents()[5].Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue,
                      std::numeric_limits<double>::infinity());

    // tags
    // rawData = R"(foo{bar="b\"a\\z"} -1.2)";
    // res = parser.Parse(rawData, 0);
    // APSARA_TEST_EQUAL(res.GetEvents().back().Cast<MetricEvent>().GetName(), "foo");
    // APSARA_TEST_STREQ(res.GetEvents().back().Cast<MetricEvent>().GetTag("bar").data(), "b\"a\\z");
    // APSARA_TEST_TRUE(
    //     IsDoubleEqual(res.GetEvents().back().Cast<MetricEvent>().GetValue<UntypedSingleValue>()->mValue, -1.2));
}
UNIT_TEST_CASE(TextParserUnittest, TestParseSuccess)


} // namespace logtail

UNIT_TEST_MAIN
