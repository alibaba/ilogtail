// Copyright 2023 iLogtail Authors
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

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "config/ProcessConfig.h"
#include "pipeline/ProcessConfigManager.h"
#include "unittest/Unittest.h"

using namespace std;

DECLARE_FLAG_BOOL(enable_send_tps_smoothing);
DECLARE_FLAG_BOOL(enable_flow_control);

namespace logtail {

class ProcessConfigManagerUnittest : public testing::Test {
public:
    void TestUpdateProcessConfigs();
    void GetProcessConfig();
};

void ProcessConfigManagerUnittest::GetProcessConfig() {
    // Exist
    {
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigBoolValue("bool", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, false);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigIntValue("int", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, -1);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigInt64Value("int64", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, -1000000);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigUIntValue("uint", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, 10000);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigUInt64Value("uint64", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, 100000000000);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigRealValue("double", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, 123123.1);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigStringValue("string", isExist);
            APSARA_TEST_TRUE(isExist);
            APSARA_TEST_EQUAL(res, "string");
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigArrayValue("array", isExist);
            APSARA_TEST_TRUE(isExist);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigObjectValue("object", isExist);
            APSARA_TEST_TRUE(isExist);
        }
    }
    // not Exist
    {
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigBoolValue("bool1", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, false);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigIntValue("int1", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, 0);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigInt64Value("int641", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, 0);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigUIntValue("uint1", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, 0);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigUInt64Value("uint641", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, 0);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigRealValue("double1", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, 0);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigStringValue("string1", isExist);
            APSARA_TEST_TRUE(!isExist);
            APSARA_TEST_EQUAL(res, "");
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigArrayValue("array1", isExist);
            APSARA_TEST_TRUE(!isExist);
        }
        {
            bool isExist = false;
            auto res = ProcessConfigManager::GetInstance()->GetProcessConfigObjectValue("object1", isExist);
            APSARA_TEST_TRUE(!isExist);
        }
    }
}

void ProcessConfigManagerUnittest::TestUpdateProcessConfigs() {
    AppConfig::GetInstance();
    ProcessConfigManager::GetInstance()->RegisterCallback(
        std::bind(&ProcessConfigManagerUnittest::GetProcessConfig, this));

    // Added
    {
        ProcessConfigDiff configDiff;
        std::string content
            = R"({"enable":true,"max_bytes_per_sec":1234,"mem_usage_limit":456,"cpu_usage_limit":2,"bool":false,"int":-1,"int64":-1000000,"uint":10000,"uint64":100000000000,"double":123123.1,"string":"string","array":[1,2,3],"object":{"a":1}})";
        std::string errorMsg;
        unique_ptr<Json::Value> detail = unique_ptr<Json::Value>(new Json::Value());
        APSARA_TEST_TRUE(ParseJsonTable(content, *detail, errorMsg));
        APSARA_TEST_TRUE(errorMsg.empty());
        ProcessConfig config("test1", std::move(detail));
        configDiff.mAdded.emplace_back(config);
        ProcessConfigManager::GetInstance()->UpdateProcessConfigs(configDiff);

        APSARA_TEST_EQUAL(1U, ProcessConfigManager::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_NOT_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test1"));
        APSARA_TEST_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test3"));

        APSARA_TEST_EQUAL(1234U, AppConfig::GetInstance()->GetMaxBytePerSec());
        APSARA_TEST_EQUAL(456U, AppConfig::GetInstance()->GetMemUsageUpLimit());
        APSARA_TEST_EQUAL(2U, AppConfig::GetInstance()->GetCpuUsageUpLimit());
        APSARA_TEST_EQUAL(AppConfig::GetInstance()->IsSendRandomSleep(), BOOL_FLAG(enable_send_tps_smoothing));
        APSARA_TEST_EQUAL(AppConfig::GetInstance()->IsSendFlowControl(), BOOL_FLAG(enable_flow_control));
    }

    // Modified
    {
        ProcessConfigDiff configDiff;
        std::string content
            = R"({"enable": true,"max_bytes_per_sec": 209715200, "mem_usage_limit":123, "cpu_usage_limit":4,"bool":false,"int":-1,"int64":-1000000,"uint":10000,"uint64":100000000000,"double":123123.1,"string":"string","array":[1,2,3],"object":{"a":1}})";
        std::string errorMsg;
        unique_ptr<Json::Value> detail = unique_ptr<Json::Value>(new Json::Value());
        APSARA_TEST_TRUE(ParseJsonTable(content, *detail, errorMsg));
        APSARA_TEST_TRUE(errorMsg.empty());
        ProcessConfig config("test1", std::move(detail));
        configDiff.mModified.emplace_back(config);
        ProcessConfigManager::GetInstance()->UpdateProcessConfigs(configDiff);

        APSARA_TEST_EQUAL(1U, ProcessConfigManager::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_NOT_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test1"));
        APSARA_TEST_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test3"));

        APSARA_TEST_EQUAL(209715200U, AppConfig::GetInstance()->GetMaxBytePerSec());
        APSARA_TEST_EQUAL(123U, AppConfig::GetInstance()->GetMemUsageUpLimit());
        APSARA_TEST_EQUAL(4U, AppConfig::GetInstance()->GetCpuUsageUpLimit());
        APSARA_TEST_EQUAL(AppConfig::GetInstance()->IsSendRandomSleep(), false);
        APSARA_TEST_EQUAL(AppConfig::GetInstance()->IsSendFlowControl(), false);
        APSARA_TEST_NOT_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test1"));
    }

    // mRemoved
    {
        ProcessConfigDiff configDiff;
        configDiff.mRemoved.emplace_back("test1");
        ProcessConfigManager::GetInstance()->UpdateProcessConfigs(configDiff);

        APSARA_TEST_EQUAL(0U, ProcessConfigManager::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test1"));
        APSARA_TEST_EQUAL(nullptr, ProcessConfigManager::GetInstance()->FindConfigByName("test3"));
    }
}

UNIT_TEST_CASE(ProcessConfigManagerUnittest, TestUpdateProcessConfigs)

} // namespace logtail

UNIT_TEST_MAIN
