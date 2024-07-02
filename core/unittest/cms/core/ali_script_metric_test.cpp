#include  <gtest/gtest.h>
#include "core/ali_script_metric.h"
#include "core/task_monitor.h"
#include "common/FileUtils.h"
#include "common/SystemUtil.h"
#include "common/Logger.h"

using namespace common;
using namespace argus;
using namespace std;

class CoreAliScriptMetricTest : public testing::Test {
};

TEST_F(CoreAliScriptMetricTest, AddConstructMetrics) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;

    AliScriptMetric pAliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, RAW_FORMAT);
    vector<pair<string, string> > stringLabels;
    vector<pair<string, double> > numberMetrics;
    map<string, double> metricMap;
    stringLabels.emplace_back("labelAKey", "LabelAValue");
    stringLabels.emplace_back("labelBKey", "LabelBValue");
    numberMetrics.emplace_back("MetricAKey", 11);
    metricMap["MetricAKey"] = 11;
    numberMetrics.emplace_back("MetricBKey", 22);
    metricMap["MetricBKey"] = 22;
    pAliScriptMetric.AddConstructMetrics(stringLabels, numberMetrics);
    vector<CommonMetric> commonMetrics;
    pAliScriptMetric.GetCommonMetrics(commonMetrics);
    EXPECT_EQ(commonMetrics.size(), size_t(2));
    for (auto commonMetric : commonMetrics) {
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
        EXPECT_EQ(commonMetric.tagMap["labelAKey"], "LabelAValue");
        EXPECT_EQ(commonMetric.tagMap["labelBKey"], "LabelBValue");
        auto it = metricMap.find(commonMetric.name);
        EXPECT_NE(it, metricMap.end());
        if (it != metricMap.end()) {
            EXPECT_EQ(commonMetric.name, it->first);
            EXPECT_EQ(commonMetric.value, it->second);
        }
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric1) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 1;
    meta1.name = "result";
    metricMetas.push_back(meta1);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": {"table": "t1", "code": 0, "result":"1"}, "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(2, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    if (commonMetrics.size() > 1) {
        CommonMetric commonMetric;
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        commonMetric = commonMetrics[1];
        EXPECT_EQ(commonMetric.name, "result");
        EXPECT_EQ(commonMetric.value, 1);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric2) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    LabelAddInfo labelAddInfo;
    labelAddInfo.name = "hostname";
    labelAddInfo.type = 0;
    labelAddInfos.push_back(labelAddInfo);
    labelAddInfo.name = "instance";
    labelAddInfo.type = 1;
    labelAddInfo.value = "INSTANCE";
    labelAddInfos.push_back(labelAddInfo);
    labelAddInfo.name = "test_add";
    labelAddInfo.type = 2;
    labelAddInfo.value = "test_add_label";
    labelAddInfos.push_back(labelAddInfo);
    labelAddInfo.name = "result";
    labelAddInfo.type = 3;
    labelAddInfo.value = "test_rename";
    labelAddInfos.push_back(labelAddInfo);
    SystemUtil::SetEnv("INSTANCE", "TEST_INSTANCE");
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 2;
    meta1.name = "table";
    meta1.isMetric = true;
    metricMetas.push_back(meta1);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": {"table": "t1","result":"success"}, "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(1, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    commonMetric = commonMetrics[0];
    if (!commonMetrics.empty()) {
        EXPECT_EQ(commonMetric.name, "table");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(5));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        EXPECT_EQ(commonMetric.tagMap["hostname"], SystemUtil::getHostname());
        EXPECT_EQ(commonMetric.tagMap["instance"], "TEST_INSTANCE");
        EXPECT_EQ(commonMetric.tagMap["test_add"], "test_add_label");
        EXPECT_EQ(commonMetric.tagMap["test_rename"], "success");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric3) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.name = "code";
    meta1.isMetric = true;
    metricMetas.push_back(meta1);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": {"table": "t1", "code": "110"}, "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(1, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (!commonMetrics.empty()) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 110);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric4) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.name = "code";
    meta1.type = 2;
    meta1.isMetric = true;
    metricMetas.push_back(meta1);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": {"table": "t1", "code": "110"}, "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(1, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (!commonMetrics.empty()) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        EXPECT_EQ(commonMetric.tagMap["code"], "110");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric5) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": {"table": "t1", "code": 0, "result":"1"}, "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(1, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (!commonMetrics.empty()) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        EXPECT_EQ(commonMetric.tagMap["result"], "1");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric6) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG": [{"table": "t1", "code": 0, "result":"1"},{"table": "t1", "code": 1, "result":"0"}], "collection_flag": 0, "error_info": ""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(2, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (commonMetrics.size() > 1) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        EXPECT_EQ(commonMetric.tagMap["result"], "1");
        commonMetric = commonMetrics[1];
        EXPECT_EQ(commonMetric.name, "code");
        EXPECT_EQ(commonMetric.value, 1);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(2));
        EXPECT_EQ(commonMetric.tagMap["table"], "t1");
        EXPECT_EQ(commonMetric.tagMap["result"], "0");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric7) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"collection_flag":0,"error_info":"","MSG":[{'cpu': 0.1,'mem': 8.5,'mem_free': 4737.11328125,'mem_used': 8172.51171875,'mem_buff': 5506.09375,'mem_cach': 78290.0546875,'mem_total': 96705.7734375,'sda': 0.0,'df_home': 35.3,'df_': 24.8,'df_boot': 4.6,'load1': 0.0,'load5': 0.0,'load15': 0.1,'ifin': 2.0927734375,'ifout': 1.208984375,'pktin': 32.0,'pktout': 10.0,'TCPretr': 0.4,}]})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(19, pAliScriptMetric->GetCommonMetrics(commonMetrics));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric8) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 2;
    meta1.name = "key";
    metricMetas.push_back(meta1);
    MetricMeta meta2;
    meta2.type = 1;
    meta2.name = "value";
    metricMetas.push_back(meta2);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG":[{"value":0,"key":"status"},{"value":"269","key":"jobs"},{"value":0.14848804473876953,"key":"elapsed_time"}],"collection_flag":0,"error_info":""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(3, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (commonMetrics.size() == size_t(3)) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "value");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "status");
        commonMetric = commonMetrics[1];
        EXPECT_EQ(commonMetric.name, "value");
        EXPECT_EQ(commonMetric.value, 269);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "jobs");
        commonMetric = commonMetrics[2];
        EXPECT_EQ(commonMetric.name, "value");
        EXPECT_EQ(commonMetric.value, 0.14848804473876953);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "elapsed_time");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric9) {
    vector<MetricFilterInfo> metricFilterInfos;
    MetricFilterInfo metricFilterInfo;
    metricFilterInfo.name = "value";
    metricFilterInfo.metricName = "new_name";
    metricFilterInfos.push_back(metricFilterInfo);
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 2;
    meta1.name = "key";
    metricMetas.push_back(meta1);
    MetricMeta meta2;
    meta2.type = 1;
    meta2.name = "value";
    metricMetas.push_back(meta2);
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string jsonStr = R"({"MSG":[{"value":0,"key":"status"},{"value":"269","key":"jobs"},{"value":0.14848804473876953,"key":"elapsed_time"}],"collection_flag":0,"error_info":""})";
    pAliScriptMetric->AddMetric(jsonStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(3, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    if (commonMetrics.size() == size_t(3)) {
        commonMetric = commonMetrics[0];
        EXPECT_EQ(commonMetric.name, "new_name");
        EXPECT_EQ(commonMetric.value, 0);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "status");
        commonMetric = commonMetrics[1];
        EXPECT_EQ(commonMetric.name, "new_name");
        EXPECT_EQ(commonMetric.value, 269);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "jobs");
        commonMetric = commonMetrics[2];
        EXPECT_EQ(commonMetric.name, "new_name");
        EXPECT_EQ(commonMetric.value, 0.14848804473876953);
        EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
        EXPECT_EQ(commonMetric.tagMap["key"], "elapsed_time");
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric10) {
    vector<MetricFilterInfo> metricFilterInfos;
    MetricFilterInfo metricFilterInfo;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    ScriptResultFormat resultFormat = ALIMONITOR_JSON_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    EXPECT_EQ(pAliScriptMetric->AddMetric(""), BaseParseMetric::EMPTY_STRING);
}

TEST_F(CoreAliScriptMetricTest, AddTextMetric1) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 1;
    meta1.name = "state";
    MetricMeta meta2;
    meta2.type = 2;
    meta2.name = "msg";
    metricMetas.push_back(meta1);
    ScriptResultFormat resultFormat = ALIMONITOR_TEXT_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string TextStr = "1621406395 state=1 msg=SSH-2.0-OpenSSH_5.3";
    pAliScriptMetric->AddMetric(TextStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(1, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    commonMetric = commonMetrics[0];
    EXPECT_EQ(commonMetric.name, "state");
    EXPECT_EQ(commonMetric.value, 1);
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["msg"], "SSH-2.0-OpenSSH_5.3");
}

TEST_F(CoreAliScriptMetricTest, AddTextMetric2) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    MetricMeta meta1;
    meta1.type = 1;
    meta1.name = "state";
    MetricMeta meta2;
    meta2.type = 2;
    meta2.name = "msg";
    meta2.isMetric = true;
    metricMetas.push_back(meta1);
    metricMetas.push_back(meta2);
    ScriptResultFormat resultFormat = ALIMONITOR_TEXT_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    string TextStr = "1621406395 state=1 msg=SSH-2.0-OpenSSH_5.3";
    pAliScriptMetric->AddMetric(TextStr);
    vector<CommonMetric> commonMetrics;
    EXPECT_EQ(2, pAliScriptMetric->GetCommonMetrics(commonMetrics));
    CommonMetric commonMetric;
    commonMetric = commonMetrics[0];
    EXPECT_EQ(commonMetric.name, "state");
    EXPECT_EQ(commonMetric.value, 1);
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["msg"], "SSH-2.0-OpenSSH_5.3");
    commonMetric = commonMetrics[1];
    EXPECT_EQ(commonMetric.name, "msg");
    EXPECT_EQ(commonMetric.value, 0);
    EXPECT_EQ(commonMetric.tagMap.size(), size_t(1));
    EXPECT_EQ(commonMetric.tagMap["msg"], "SSH-2.0-OpenSSH_5.3");
}

TEST_F(CoreAliScriptMetricTest, AddTextMetric3) {
    vector<MetricFilterInfo> metricFilterInfos;
    vector<LabelAddInfo> labelAddInfos;
    vector<MetricMeta> metricMetas;
    ScriptResultFormat resultFormat = ALIMONITOR_TEXT_FORMAT;
    unique_ptr<AliScriptMetric> pAliScriptMetric(
            new AliScriptMetric(metricFilterInfos, labelAddInfos, metricMetas, resultFormat));
    EXPECT_EQ(pAliScriptMetric->AddMetric(""), BaseParseMetric::EMPTY_STRING);
}

TEST_F(CoreAliScriptMetricTest, GetMetricKey) {
    CommonMetric metric;
    metric.name = "a";
    metric.tagMap["b"] = "c";
    EXPECT_EQ("abc", AliScriptMetric::GetMetricKey(metric));
}

TEST_F(CoreAliScriptMetricTest, GetJsonLabelAddInfos) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mJsonlabelAddInfos.resize(1);

    std::vector<LabelAddInfo> labels;
    scriptMetric.GetJsonLabelAddInfos(labels);
    EXPECT_EQ(labels.size(), scriptMetric.mJsonlabelAddInfos.size());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric_InvalidStandarizePythonJson) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_RESULT_NOT_JSON, scriptMetric.AddJsonMetric("[)"));
    auto cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("neither json nor StandardPythonJson"));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric_NotObject) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_RESULT_NOT_OBJECT, scriptMetric.AddJsonMetric("[]"));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric_NoMsgKey) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_INVALID, scriptMetric.AddJsonMetric("{}"));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMetric_ArrayMsgKey_InvalidElement) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    const char *szJson = R"({"MSG": [null]})";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_ARRAY_ELEMENT_INVALID, scriptMetric.AddJsonMetric(szJson));
}

TEST_F(CoreAliScriptMetricTest, AddTextMetric_InvalidTimestamp) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    const char *szText = "abc";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_TEXT_TIMESTAMP_INVALID, scriptMetric.AddTextMetric(szText));
}

TEST_F(CoreAliScriptMetricTest, AddTextMetric) {
    {
        AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
        scriptMetric.mMetricMetaMap["num"].type = 1; // number
        scriptMetric.mMetricMetaMap["num"].isMetric = true;

        scriptMetric.mMetricFilterInfoMap["num"].metricName = "numX";

        // 1706716800 2024-02-01 00:00:00
        const char *szText = "1706716800 a=b=c num=s";
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddTextMetric(szText));

        EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

        EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "numX");
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 0.0);
        std::map<std::string, std::string> expect{
                {"num", "s"},
        };
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].tagMap, expect);
    }
    {
        AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
        scriptMetric.mMetricMetaMap["class"].type = 3; // number
        scriptMetric.mMetricMetaMap["class"].isMetric = true;
        scriptMetric.mMetricFilterInfoMap["class"].metricName = "clazz";

        // 1706716800 2024-02-01 00:00:00
        const char *szText = "1706716800 class=3";
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddTextMetric(szText));

        EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

        EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "clazz");
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 3.0);
        EXPECT_TRUE(scriptMetric.mCommonMetrics[0].tagMap.empty());
    }
    {
        AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
        scriptMetric.mMetricMetaMap["folder"].type = 3; // number
        scriptMetric.mMetricMetaMap["folder"].isMetric = true;
        scriptMetric.mMetricFilterInfoMap["folder"].metricName = "Folder";

        // 1706716800 2024-02-01 00:00:00
        const char *szText = "1706716800 folder=/tmp";
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddTextMetric(szText));

        EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "Folder");
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 0.0);
        std::map<std::string, std::string> expect{
                {"folder", "/tmp"},
        };
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].tagMap, expect);
    }
    {
        AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
        scriptMetric.mMetricFilterInfoMap["other"].metricName = "Other";

        // 1706716800 2024-02-01 00:00:00
        const char *szText = "1706716800 other=5";
        EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddTextMetric(szText));

        EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "Other");
        EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 5.0);
        EXPECT_TRUE(scriptMetric.mCommonMetrics[0].tagMap.empty());
    }
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject000) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    const char *szJson = R"XX({"A":null})XX";
    json::Object root = json::parseObject(szJson);
    ASSERT_FALSE(root.isNull());
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_MEMBER_INVALID, scriptMetric.AddJsonMsgObject(root));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject001) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["num"].type = 1; // number
    scriptMetric.mMetricMetaMap["num"].isMetric = true;

    scriptMetric.mMetricFilterInfoMap["num"].metricName = "numX";

    const char *szJson = R"({"num":"s"})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "numX");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 0.0);
    std::map<std::string, std::string> expect{
            {"num", "s"},
    };
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].tagMap, expect);
}
TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0011) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["num"].type = 1; // number
    scriptMetric.mMetricMetaMap["num"].isMetric = true;

    scriptMetric.mMetricFilterInfoMap["num"].metricName = "numX";

    const char *szJson = R"({"num":[]})";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_MEMBER_INVALID,
              scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_TRUE(scriptMetric.mCommonMetrics.empty());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject002) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["num"].type = 2; // number
    scriptMetric.mMetricMetaMap["num"].isMetric = true;

    scriptMetric.mMetricFilterInfoMap["num"].metricName = "numX";

    const char *szJson = R"({"num":2})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "numX");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 0.0);
    std::map<std::string, std::string> expect{
            {"num", "2"}
    };
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].tagMap, expect);
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0021) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["num"].type = 2; // number
    scriptMetric.mMetricMetaMap["num"].isMetric = true;

    scriptMetric.mMetricFilterInfoMap["num"].metricName = "numX";

    const char *szJson = R"({"num":[]})";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_MEMBER_INVALID,
              scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_TRUE(scriptMetric.mCommonMetrics.empty());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0030) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["class"].type = 3; // number
    scriptMetric.mMetricMetaMap["class"].isMetric = true;
    scriptMetric.mMetricFilterInfoMap["class"].metricName = "clazz";

    const char *szJson = R"({"class":3})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "clazz");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 3.0);
    EXPECT_TRUE(scriptMetric.mCommonMetrics[0].tagMap.empty());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0031) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["class"].type = 3; // number
    scriptMetric.mMetricMetaMap["class"].isMetric = true;
    scriptMetric.mMetricFilterInfoMap["class"].metricName = "clazz";

    const char *szJson = R"({"class":"3"})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "clazz");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 3.0);
    EXPECT_TRUE(scriptMetric.mCommonMetrics[0].tagMap.empty());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0032) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["class"].type = 3; // number
    scriptMetric.mMetricMetaMap["class"].isMetric = true;
    scriptMetric.mMetricFilterInfoMap["class"].metricName = "clazz";

    const char *szJson = R"({"class":"abcd"})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));

    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "clazz");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 0.0);
    std::map<std::string, std::string> expect{{"class", "abcd"}};
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].tagMap, expect);
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0033) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricMetaMap["class"].type = 3; // number
    scriptMetric.mMetricMetaMap["class"].isMetric = true;
    scriptMetric.mMetricFilterInfoMap["class"].metricName = "clazz";

    const char *szJson = R"({"class":[]})";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_MEMBER_INVALID,
              scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(0));
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject004) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricFilterInfoMap["other"].metricName = "Other";

    const char *szJson = R"({"other":5})";
    EXPECT_EQ(BaseParseMetric::PARSE_SUCCESS, scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(1));
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].name, "Other");
    EXPECT_EQ(scriptMetric.mCommonMetrics[0].value, 5.0);
    EXPECT_TRUE(scriptMetric.mCommonMetrics[0].tagMap.empty());
}

TEST_F(CoreAliScriptMetricTest, AddJsonMsgObject0041) {
    AliScriptMetric scriptMetric({}, {}, {}, ScriptResultFormat::ALIMONITOR_JSON_FORMAT);
    scriptMetric.mMetricFilterInfoMap["other"].metricName = "Other";

    const char *szJson = R"({"other":[]})";
    EXPECT_EQ(BaseParseMetric::ALIMONITOR_JSON_MSG_MEMBER_INVALID,
              scriptMetric.AddJsonMsgObject(json::parseObject(szJson)));

    EXPECT_EQ(scriptMetric.mCommonMetrics.size(), size_t(0));
}
