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

#include <json/json.h>

#include <memory>
#include <string>

#include "config/ConfigDiff.h"
#include "config/PipelineConfig.h"
#include "config/watcher/PipelineConfigWatcher.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class PipelineConfigWatcherUnittest : public testing::Test {
public:
    void TestSortPipelineConfigDiff() const;

private:
    const string configName = "test";
};

void PipelineConfigWatcherUnittest::TestSortPipelineConfigDiff() const {
    PipelineConfigDiff diff;
    auto configName1 = "test1";
    auto configJson1 = std::make_unique<Json::Value>();
    (*configJson1)["name"] = configName1;
    auto config1 = PipelineConfig(configName1, std::move(configJson1));
    config1.Parse();

    auto configName2 = "test2";
    auto configJson2 = std::make_unique<Json::Value>();
    (*configJson2)["name"] = configName2;
    (*configJson2)["createTime"] = 1;
    auto config2 = PipelineConfig(configName2, std::move(configJson2));
    config2.Parse();

    auto configName3 = "test3";
    auto configJson3 = std::make_unique<Json::Value>();
    (*configJson3)["name"] = configName3;
    (*configJson3)["createTime"] = 2;
    auto config3 = PipelineConfig(configName3, std::move(configJson3));
    config3.Parse();

    auto configName4 = "test4";
    auto configJson4 = std::make_unique<Json::Value>();
    (*configJson4)["name"] = configName4;
    auto config4 = PipelineConfig(configName4, std::move(configJson4));
    config4.Parse();

    auto configName5 = "test5";
    auto configJson5 = std::make_unique<Json::Value>();
    (*configJson5)["name"] = configName5;
    (*configJson5)["createTime"] = 1;
    auto config5 = PipelineConfig(configName5, std::move(configJson5));
    config5.Parse();

    diff.mAdded.push_back(std::move(config1));
    diff.mAdded.push_back(std::move(config2));
    diff.mAdded.push_back(std::move(config3));
    diff.mAdded.push_back(std::move(config4));
    diff.mAdded.push_back(std::move(config5));

    auto configName6 = "test6";
    auto configJson6 = std::make_unique<Json::Value>();
    (*configJson6)["name"] = configName6;
    auto config6 = PipelineConfig(configName6, std::move(configJson6));
    config6.Parse();

    auto configName7 = "test7";
    auto configJson7 = std::make_unique<Json::Value>();
    (*configJson7)["name"] = configName7;
    (*configJson7)["createTime"] = 1;
    auto config7 = PipelineConfig(configName7, std::move(configJson7));
    config7.Parse();

    auto configName8 = "test8";
    auto configJson8 = std::make_unique<Json::Value>();
    (*configJson8)["name"] = configName8;
    (*configJson8)["createTime"] = 2;
    auto config8 = PipelineConfig(configName8, std::move(configJson8));
    config8.Parse();

    auto configName9 = "test9";
    auto configJson9 = std::make_unique<Json::Value>();
    (*configJson9)["name"] = configName9;
    auto config9 = PipelineConfig(configName9, std::move(configJson9));
    config9.Parse();

    auto configName10 = "test10";
    auto configJson10 = std::make_unique<Json::Value>();
    (*configJson10)["name"] = configName10;
    (*configJson10)["createTime"] = 1;
    auto config10 = PipelineConfig(configName10, std::move(configJson10));
    config10.Parse();

    diff.mModified.push_back(std::move(config6));
    diff.mModified.push_back(std::move(config7));
    diff.mModified.push_back(std::move(config8));
    diff.mModified.push_back(std::move(config9));
    diff.mModified.push_back(std::move(config10));

    PipelineConfigWatcher::GetInstance()->SortPipelineConfigDiff(diff);

    APSARA_TEST_EQUAL(diff.mAdded[0].mName, configName2);
    APSARA_TEST_EQUAL(diff.mAdded[1].mName, configName5);
    APSARA_TEST_EQUAL(diff.mAdded[2].mName, configName3);
    APSARA_TEST_EQUAL(diff.mAdded[3].mName, configName1);
    APSARA_TEST_EQUAL(diff.mAdded[4].mName, configName4);

    APSARA_TEST_EQUAL(diff.mModified[0].mName, configName10);
    APSARA_TEST_EQUAL(diff.mModified[1].mName, configName7);
    APSARA_TEST_EQUAL(diff.mModified[2].mName, configName8);
    APSARA_TEST_EQUAL(diff.mModified[3].mName, configName6);
    APSARA_TEST_EQUAL(diff.mModified[4].mName, configName9);
}

UNIT_TEST_CASE(PipelineConfigWatcherUnittest, TestSortPipelineConfigDiff)

} // namespace logtail

UNIT_TEST_MAIN
