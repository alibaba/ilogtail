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

#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"
#include "pipeline/plugin/PluginRegistry.h"
#include "plugin/input/InputNetworkSecurity.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class PipelineManagerUnittest : public testing::Test {
public:
    void TestPipelineManagement() const;
    void TestCheckIfGlobalSingletonInputLoaded() const;

protected:
    static void SetUpTestCase() { PluginRegistry::GetInstance()->LoadPlugins(); }
    static void TearDownTestCase() { PluginRegistry::GetInstance()->UnloadPlugins(); }
};

void PipelineManagerUnittest::TestPipelineManagement() const {
    PipelineManager::GetInstance()->mPipelineNameEntityMap["test1"] = make_shared<Pipeline>();
    PipelineManager::GetInstance()->mPipelineNameEntityMap["test2"] = make_shared<Pipeline>();

    APSARA_TEST_EQUAL(2U, PipelineManager::GetInstance()->GetAllConfigNames().size());
    APSARA_TEST_NOT_EQUAL(nullptr, PipelineManager::GetInstance()->FindConfigByName("test1"));
    APSARA_TEST_EQUAL(nullptr, PipelineManager::GetInstance()->FindConfigByName("test3"));
}

void PipelineManagerUnittest::TestCheckIfGlobalSingletonInputLoaded() const {
    { // test not singleton input
        auto inputConfig = Json::Value();
        inputConfig["Type"] = InputFile::sName;
        std::vector<const Json::Value*> inputConfigs = {&inputConfig};

        PipelineManager::GetInstance()->mPluginCntMap["inputs"][InputFile::sName] = 1;
        APSARA_TEST_EQUAL(false, PipelineManager::GetInstance()->CheckIfGlobalSingletonInputLoaded(inputConfigs));
    }
    { // test singleton input not loaded
        auto inputConfig = Json::Value();
        inputConfig["Type"] = InputNetworkSecurity::sName;
        std::vector<const Json::Value*> inputConfigs = {&inputConfig};

        PipelineManager::GetInstance()->mPluginCntMap["inputs"][InputNetworkSecurity::sName] = 0;
        APSARA_TEST_EQUAL(false, PipelineManager::GetInstance()->CheckIfGlobalSingletonInputLoaded(inputConfigs));
    }
    { // test singleton input loaded
        auto inputConfig = Json::Value();
        inputConfig["Type"] = InputNetworkSecurity::sName;
        std::vector<const Json::Value*> inputConfigs = {&inputConfig};

        PipelineManager::GetInstance()->mPluginCntMap["inputs"][InputNetworkSecurity::sName] = 1;
        APSARA_TEST_EQUAL(true, PipelineManager::GetInstance()->CheckIfGlobalSingletonInputLoaded(inputConfigs));
    }
}

UNIT_TEST_CASE(PipelineManagerUnittest, TestPipelineManagement)
UNIT_TEST_CASE(PipelineManagerUnittest, TestCheckIfGlobalSingletonInputLoaded)

} // namespace logtail

UNIT_TEST_MAIN
