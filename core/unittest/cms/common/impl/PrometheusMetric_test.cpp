#include  <gtest/gtest.h>
#include "common/PrometheusMetric.h"
#include "common/Logger.h"
#include "common/StringUtils.h"

using namespace common;
using namespace std;

class CommonPrometheusMetricTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(CommonPrometheusMetricTest, ParseMetric_Empty) {
    // 匿名指标，常见于查询结果
    string str = "  ";
    CommonMetric commonMetric;
    EXPECT_EQ(-1, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "");
    EXPECT_TRUE(commonMetric.tagMap.empty());
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_000) {
    // 匿名指标，常见于查询结果
    string str = R"({} 1.0 1)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "");
    EXPECT_TRUE(commonMetric.tagMap.empty());
    EXPECT_EQ(commonMetric.timestamp, 1);
    EXPECT_EQ(commonMetric.value, 1.0);
}
TEST_F(CommonPrometheusMetricTest, ParseMetric_001) {
    string str = R"(container_cpu_load_average_10s 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(commonMetric.value, 1.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_002) {
    string str = R"(container_cpu_load_average_10s{} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(commonMetric.value, 1.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_003) {
    string str = R"(container_cpu_load_average_10s{id="xxx"} 1.0  )";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(commonMetric.value, 1.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_004) {
    string str = R"(container_cpu_load_average_10s{id="xxx",} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(commonMetric.value, 1.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_005) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , } 0.0000254389726)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(fmt::format("{}", commonMetric.value), "2.54389726e-05");
    EXPECT_EQ(StringUtils::DoubleToString(commonMetric.value), "0");
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_006) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , name = "hello"} 1.38526973   )";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.tagMap["name"], "hello");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(fmt::format("{}", commonMetric.value), "1.38526973");
    EXPECT_EQ(StringUtils::DoubleToString(commonMetric.value), "1.39");
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_007) {
    // container_cpu_load_average_10s { id="xxx"  , name = hello} 1.0
    // ....................................................^
    // format error: label value should start with '"'
    string str = R"(container_cpu_load_average_10s { id="xxx"  , name = hello} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(53, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_008) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , name = "hello"} NaN   )";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.tagMap["name"], "hello");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_TRUE(std::isnan(commonMetric.value));
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_009) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , name = "hello"} +Inf   )";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.tagMap["name"], "hello");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_TRUE(std::isinf(commonMetric.value));
    EXPECT_GT(commonMetric.value, 0.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_010) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , name = "hello"} -Inf   )";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.tagMap["name"], "hello");
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_TRUE(std::isinf(commonMetric.value));
    EXPECT_LT(commonMetric.value, 0.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_011_without_value) {
    string str = R"(container_cpu_load_average_10s{id="xxx"  , name = "hello"}   )";
    CommonMetric commonMetric;
    EXPECT_EQ(59, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_cpu_load_average_10s");
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxx");
    EXPECT_EQ(commonMetric.tagMap["name"], "hello");
    EXPECT_EQ(commonMetric.value, 0.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_012_invalid_metric_name_first_char) {
    string str = R"(3container_cpu_load_average_10s 1)";
    CommonMetric commonMetric;
    EXPECT_EQ(1, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_TRUE(commonMetric.name.empty());
    EXPECT_TRUE(commonMetric.tagMap.empty());
    EXPECT_EQ(commonMetric.value, 0.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_013) {
    string str = R"(cpu_load_average_10s{id="aaa" c="d"} 1)";
    CommonMetric commonMetric;
    EXPECT_EQ(31, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "cpu_load_average_10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
    EXPECT_EQ(commonMetric.value, 0.0);
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_050) {
    // container@10s { id="xxx"  , name = hello} 1.0
    // .........^
    // invalid metricName char: '@', validated regex: [a-zA-Z_:][a-zA-Z0-9_:]*
    const string str = R"(container@10s { id="xxx"  , name = hello} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(10, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_TRUE(commonMetric.name.empty());
    EXPECT_TRUE(commonMetric.tagMap.empty());
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_051) {
    // container:10s { 3id="xxx"  , name = hello} 1.0
    // ................^
    // the first char of label name don't satisfied: [a-zA-Z_]
    const string str = R"(container:10s { 3id="xxx"  , name = hello} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(17, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container:10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
}

TEST_F(CommonPrometheusMetricTest, ParseMetric_052) {
    // container_10s { id:"xxx"} 1.0
    // ..................^
    // format error: expected '='
    const string str = R"(container_10s { id:"xxx"} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(19, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.name, "container_10s");
    EXPECT_TRUE(commonMetric.tagMap.empty());
}

TEST_F(CommonPrometheusMetricTest,ParseMetric_100) {
    string str = R"(container_cpu_load_average_10s{id="xxxx",name="docker_ubuntu_16.10"} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.timestamp, 0);
    EXPECT_EQ(commonMetric.value, 1.0);
}
TEST_F(CommonPrometheusMetricTest,ParseMetric_101) {
    const std::string str = R"(container_cpu_load_average_10s{id="xxxx",name="docker_ubuntu_16.10"} -1.0 152222)";
    CommonMetric commonMetric;
    EXPECT_EQ(0,PrometheusMetric::ParseMetric(str,commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"],"xxxx");
    EXPECT_EQ(commonMetric.tagMap["name"],"docker_ubuntu_16.10");
    EXPECT_EQ(commonMetric.timestamp,152222);
    EXPECT_EQ(commonMetric.value,-1.0);
}
TEST_F(CommonPrometheusMetricTest,ParseMetric_102)
{
    const string str = R"(container_cpu_load_average_10s{id="xxxx",name="{docker_ubuntu_16.10}"} 1.0)";
    CommonMetric commonMetric;
    EXPECT_EQ(0,PrometheusMetric::ParseMetric(str,commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.timestamp,0);
    EXPECT_EQ(commonMetric.value,1.0);
    EXPECT_EQ(commonMetric.tagMap["name"],"{docker_ubuntu_16.10}");
}
TEST_F(CommonPrometheusMetricTest,ParseMetric_103) {
    CommonMetric commonMetric;
    const std::string str = R"(container_cpu_load_average_10s{id="xxxx",name="=docker_ubuntu_16.10"} -1.0 152222)";
    EXPECT_EQ(0,PrometheusMetric::ParseMetric(str,commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.tagMap["id"],"xxxx");
    EXPECT_EQ(commonMetric.tagMap["name"],"=docker_ubuntu_16.10");
    EXPECT_EQ(commonMetric.timestamp,152222);
    EXPECT_EQ(commonMetric.value,-1.0);
}
TEST_F(CommonPrometheusMetricTest,ParseMetric3) {
    string str = R"(ssl_cert_not_after{cn="*.4px.com",dnsnames=",*.4px.com,www.4px.com,4px.com,",emails="",ips="",issuer_cn="GlobalSign RSA OV SSL CA 2018",ou="",serial_no="29507780769329804368963828865"} 1.698131912e+09)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));

    LogDebug("{}", str);
    LogDebug("metricName: {}", commonMetric.name);
    LogDebug("metricLabels[{}]:", commonMetric.tagMap.size());
    int count = 0;
    for (const auto &it: commonMetric.tagMap) {
        LogDebug("[{}] <{}>: {}", ++count, it.first, it.second);
    }
    LogDebug("metricValue: {}", commonMetric.value);
    LogDebug("metricTimestamp: {}", commonMetric.timestamp);

    EXPECT_EQ(commonMetric.tagMap.size(), size_t(7));
    EXPECT_EQ(commonMetric.tagMap["dnsnames"], ",*.4px.com,www.4px.com,4px.com,");
    EXPECT_EQ(commonMetric.tagMap["issuer_cn"], "GlobalSign RSA OV SSL CA 2018");
    EXPECT_NE(commonMetric.tagMap.find("ips"), commonMetric.tagMap.end());
    EXPECT_TRUE(commonMetric.tagMap["ips"].empty());
    EXPECT_EQ(commonMetric.timestamp, 0);
}

TEST_F(CommonPrometheusMetricTest,ParseMetrics)
{
    string str = "container_cpu_load_average_10s{id=\"xxxx\",name=\"docker_ubuntu_16.10\"} 1.0\n";
    str += "container_cpu_load_average_11s{id=\"xxxx\",name=\"docker_ubuntu_16.10\"} -1.0 152222\n";
    str += R"(container_cpu_load_average_12s{id="xxxx",name="docker_ubuntu_16.10"} 10000222222 153322)";
    vector<CommonMetric> metrics;
    EXPECT_EQ(3,PrometheusMetric::ParseMetrics(str,metrics));
    for(auto it = metrics.begin();it!= metrics.end(); it++)
    {
        if(it->name == "container_cpu_load_average_10s")
        {
            EXPECT_EQ(it->value,1.0);
            EXPECT_EQ(it->timestamp,0);
        }else if (it->name == "container_cpu_load_average_11s")
        {
            EXPECT_EQ(it->value,-1.0);
            EXPECT_EQ(it->timestamp,152222);

        }else if(it->name == "container_cpu_load_average_12s")
        {
            EXPECT_EQ(it->value,10000222222);
            EXPECT_EQ(it->timestamp,153322);
        }else
        {
            EXPECT_EQ(0,1);
        }
    }
}

TEST_F(CommonPrometheusMetricTest,MetricToLine_001) {
    string str = R"(container_cpu_load_average_10s{id="xxxx",name="docker_ubuntu_16.10"} 1.01 1602232053000)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.timestamp, 1602232053000);
    EXPECT_EQ(commonMetric.value, 1.01);
    string lineStr = PrometheusMetric::MetricToLine(commonMetric);
    EXPECT_EQ(str, lineStr);
}
TEST_F(CommonPrometheusMetricTest,MetricToLine_002) {
    const string str = "container_cpu_load_average_10s{id=\"xxxx\"} 2.02 152222";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["id"], "xxxx");
    EXPECT_EQ(commonMetric.timestamp, 152222);
    string lineStr = PrometheusMetric::MetricToLine(commonMetric);
    EXPECT_EQ(str, lineStr);
}
TEST_F(CommonPrometheusMetricTest,MetricToLine_003) {
    const string str = "container_cpu_load_average_10s{id=\"xxxx\"} 2.0 152222";
    string str1      = "container_cpu_load_average_10s{id=\"xxxx\"} 2 152222";
    CommonMetric commonMetric;
    EXPECT_EQ(0,PrometheusMetric::ParseMetric(str,commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["id"],"xxxx");
    EXPECT_EQ(commonMetric.timestamp,152222);
    string lineStr = PrometheusMetric::MetricToLine(commonMetric);
    EXPECT_EQ(str1,lineStr);
}

TEST_F(CommonPrometheusMetricTest, MetricToLine_004) {
    string str = R"(container_cpu_load_average_10s{id="a\b\"c\n",name="docker_ubuntu_16.10"} 1.01 1602232053000)";
    CommonMetric commonMetric;
    EXPECT_EQ(0, PrometheusMetric::ParseMetric(str, commonMetric));
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
    EXPECT_EQ(commonMetric.timestamp, 1602232053000);
    EXPECT_EQ(commonMetric.value, 1.01);
    string lineStr = PrometheusMetric::MetricToLine(commonMetric);
    const string expect = R"(container_cpu_load_average_10s{id="a\\b\"c\n",name="docker_ubuntu_16.10"} 1.01 1602232053000)";
    cout << "expect: " << expect << endl
         << "actual: " << lineStr << endl;
    EXPECT_EQ(expect, lineStr);
}

TEST_F(CommonPrometheusMetricTest, MetricToLine_005) {
    CommonMetric metric;
    metric.value = 1.01;
    metric.timestamp = 2;

    std::string line = PrometheusMetric::MetricToLine(metric, false);
    EXPECT_EQ(line, "{} 1.01");

    line = PrometheusMetric::MetricToLine(metric, true);
    EXPECT_EQ(line, "{} 1.01 2");
}

TEST_F(CommonPrometheusMetricTest, MetricToLine_006_nan) {
    CommonMetric metric;
    metric.value = std::numeric_limits<double>::quiet_NaN();

    std::string line = PrometheusMetric::MetricToLine(metric, false);
    EXPECT_EQ(line, "{} NaN");
}

TEST_F(CommonPrometheusMetricTest, MetricToLine_007_inf) {
    CommonMetric metric;
    metric.value = std::numeric_limits<double>::infinity();

    std::string line = PrometheusMetric::MetricToLine(metric, false);
    EXPECT_EQ(line, "{} +Inf");
}

TEST_F(CommonPrometheusMetricTest, HandleEscape) {
    EXPECT_EQ(_PromEsc("hello"), "hello");
    EXPECT_EQ(_PromEsc("he\\l\"l\"o"), R"XX(he\\l\"l\"o)XX");
    EXPECT_EQ(_PromEsc("he\nllo"), R"XX(he\nllo)XX");
}
