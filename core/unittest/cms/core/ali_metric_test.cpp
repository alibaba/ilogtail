#include <gtest/gtest.h>
#include "core/ali_metric.h"

#include "common/FileUtils.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/Config.h"

#include "core/task_monitor.h"

using namespace common;
using namespace argus;
using namespace std;

class AliMetricTest : public testing::Test {
};

bool GetMetric(const vector<CommonMetric> &metrics, const string &metricName, CommonMetric &metric) {
    for (const auto & it : metrics) {
        if (it.name == metricName) {
            metric = it;
            return true;
        }
    }
    return false;
}

TEST_F(AliMetricTest, AliMetric_buildMap) {
    std::vector<MetricFilterInfo> vec;
    vec.resize(1);

    std::unordered_map<std::string,std::set<std::string>> m;
    AliMetric::buildMap(vec, m);
    EXPECT_TRUE(m.empty());
}

TEST_F(AliMetricTest, AddMetric00) {
    AliMetric metric({}, {});
    EXPECT_EQ(BaseParseMetric::PARSE_FAILED, metric.AddMetric(""));
    EXPECT_EQ(BaseParseMetric::PARSE_FAILED, metric.AddMetric(R"({"success":false})"));
    EXPECT_EQ(BaseParseMetric::PARSE_FAILED, metric.AddMetric(R"({"success":true})"));  // no data

    metric.mMetricMap["a"].insert("xxx");
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":{}}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("no type"));
    }
    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":[{}]}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("invalid metric"));
    }
    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":[{
"metric":"xxx",
}]}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("invalid timestamp"));
    }
    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":[{
"metric":"xxx",
"timestamp":-2,
}]}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("invalid timestamp"));
    }
    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":[{
"metric":"xxx",
"timestamp":1,
}]}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("invalid interval"));
    }
    {
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, metric.AddMetric(R"({"success":true,"data":{"a":[{
"metric":"xxx",
"timestamp":1,
"interval":1,
"value":"hello",
}]}})"));
        EXPECT_TRUE(metric.mLastMetricMap.empty());
        auto cache = logger->getLogCache();
        std::cout << cache;
        EXPECT_NE(std::string::npos, cache.find("invalid value"));
    }
}

TEST_F(AliMetricTest, AddMetric) {
    string config;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/alimetric.json").string(), config);
    SingletonTaskMonitor::Instance()->ParseExporterTaskInfo(config, false);
    TaskManager *pTask = SingletonTaskManager::Instance();
    vector<ExporterItem> items = *pTask->ExporterItems();
    EXPECT_EQ(items.size(), size_t(1));
    if (items.size() == size_t(1)) {
        EXPECT_EQ(items[0].type, 1);
        unique_ptr<AliMetric> pAliMetricPtr(new AliMetric(items[0].metricFilterInfos, items[0].labelAddInfos));
        string result;
        FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/alimetric_result1.json").string(), result);
        pAliMetricPtr->AddMetric(result);
        vector<CommonMetric> commonMetrics;
        pAliMetricPtr->GetCommonMetrics(commonMetrics);
        constexpr const size_t metricNum = 2 + 19 + 2 + 9 + 3 + 3;
        cout << "size=" << commonMetrics.size() << endl;
        EXPECT_EQ(metricNum, commonMetrics.size());
        CommonMetric metric;
        EXPECT_TRUE(GetMetric(commonMetrics, "middleware.nginx.request.qps", metric));
        EXPECT_EQ(1605259460, metric.timestamp);
        // 避免精度误差
        EXPECT_EQ(convert(116.64177866268459).substr(0, 15), convert(metric.value).substr(0, 15));
        EXPECT_FALSE(GetMetric(commonMetrics, "jvm.gc.parnew.count.delta", metric));
        EXPECT_TRUE(GetMetric(commonMetrics, "middleware.hsf.provider.biz_pool.max_size", metric));
        EXPECT_EQ(1605259455, metric.timestamp);
        EXPECT_EQ(720, metric.value);
    }
    {
        unique_ptr<AliMetric> pAliMetricPtr(new AliMetric(items[0].metricFilterInfos, items[0].labelAddInfos));
        string result2, result3, result4;
        FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/alimetric_result2.json").string(), result2);
        FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/alimetric_result3.json").string(), result3);
        FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/exporter_task/alimetric_result4.json").string(), result4);
        pAliMetricPtr->AddMetric(result2);
        vector<CommonMetric> commonMetrics;
        pAliMetricPtr->GetCommonMetrics(commonMetrics);
        cout << "size=" << commonMetrics.size() << endl;
        // size_t number2 = commonMetrics.size();
        commonMetrics.clear();
        pAliMetricPtr->AddMetric(result3);
        pAliMetricPtr->GetCommonMetrics(commonMetrics);
        cout << "size=" << commonMetrics.size() << endl;
        for (const auto &it: commonMetrics) {
            cout << "metric=" << it.name << endl;
        }
        commonMetrics.clear();
        pAliMetricPtr->AddMetric(result4);
        pAliMetricPtr->GetCommonMetrics(commonMetrics);
        cout << "size=" << commonMetrics.size() << endl;
        commonMetrics.clear();
    }

}