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

#include "common/JsonUtil.h"
#include "config/watcher/PipelineConfigWatcher.h"
#include "plugin/PluginRegistry.h"
#include "unittest/Unittest.h"
#include "unittest/config/PipelineManagerMock.h"

using namespace std;

namespace logtail {

class PipelineConfigWatcherUnittest : public testing::Test {
public:
    void TestLoadSingletonConfig();

protected:
    static void SetUpTestCase() {
        PluginRegistry::GetInstance()->LoadPlugins();
        PipelineConfigWatcher::GetInstance()->SetPipelineManager(PipelineManagerMock::GetInstance());
    }
    static void TearDownTestCase() { PluginRegistry::GetInstance()->UnloadPlugins(); }

private:
    void PrepareConfig() {
        filesystem::create_directories(configDir1);
        PipelineConfigWatcher::GetInstance()->AddSource(configDir1.string());
        filesystem::create_directories(configDir2);
        PipelineConfigWatcher::GetInstance()->AddSource(configDir2.string());
    }

    void ClearConfig() {
        PipelineManagerMock::GetInstance()->ClearEnvironment();
        PipelineConfigWatcher::GetInstance()->ClearEnvironment();
        filesystem::remove_all(configDir1);
        filesystem::remove_all(configDir2);
    }

    filesystem::path configDir1 = "./continuous_pipeline_config1";
    filesystem::path configDir2 = "./continuous_pipeline_config2";

    const std::string greaterPriorityConfig = R"(
        {
            "createTime": 1,
            "valid": true,
            "inputs": [
                {
                    "Type": "input_network_observer"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";

    const std::string lessPriorityConfig = R"(
        {
            "createTime": 2,
            "valid": true,
            "inputs": [
                {
                    "Type": "input_network_observer"
                }
            ],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";

    const std::string modifiedGreaterPriorityConfig = R"(
        {
            "createTime": 1,
            "valid": true,
            "inputs": [
                {
                    "Type": "input_network_observer"
                }
            ],
            "processors": [],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";

    const std::string modifiedLessPriorityConfig = R"(
        {
            "createTime": 2,
            "valid": true,
            "inputs": [
                {
                    "Type": "input_network_observer"
                }
            ],
            "processors": [],
            "flushers": [
                {
                    "Type": "flusher_sls"
                }
            ]
        }
    )";
};

void PipelineConfigWatcherUnittest::TestLoadSingletonConfig() {
    // there are 4 kinds of a config: added, modified, removed, unchanged
    // there are 4 kinds of priority relationship: first > second, first < second,
    // first > second -> first < second, first < second -> first > second
    // total case:  4 (first kind) * 4(second kind) * 4(priority) = 64
    { // case: added -> added, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> added, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> added, first > second -> first < second
      // should not happen
    }
    { // case: added -> added, first < second -> first > second
      // should not happen
    }
    { // case: added -> modified, first > second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> modified, first < second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> modified, first > second -> first < second
      // should not happen
    }
    { // case: added -> modified, first < second -> first > second
      // should not happen
    }
    { // case: added -> removed, first > second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> removed, first < second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> removed, first > second -> first < second
      // should not happen
    }
    { // case: added -> removed, first < second -> first > second
      // should not happen
    }
    { // case: added -> unchanged, first > second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> unchanged, first < second
        PrepareConfig();
        ofstream fout(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: added -> unchanged, first > second -> first < second
      // should not happen
    }
    { // case: added -> unchanged, first < second -> first > second
      // should not happen
    }
    { // case: modified -> added, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> added, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> added, first > second -> first < second
      // should not happen
    }
    { // case: modified -> added, first < second -> first > second
      // should not happen
    }
    { // case: modified -> modified, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> modified, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> modified, first > second -> first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> modified, first < second -> first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> removed, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> removed, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> removed, first > second -> first < second
      // should not happen
    }
    { // case: modified -> removed, first < second -> first > second
      // should not happen
    }
    { // case: modified -> unchanged, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> unchanged, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> unchanged, first > second -> first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        fout.open(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: modified -> unchanged, first < second -> first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test3.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir1 / "test3.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test3", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> added, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> added, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> added, first > second -> first < second
      // should not happen
    }
    { // case: removed -> added, first < second -> first > second
      // should not happen
    }
    { // case: removed -> modified, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        this_thread::sleep_for(chrono::milliseconds(1));

        filesystem::remove(configDir1 / "test1.json");
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> modified, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        this_thread::sleep_for(chrono::milliseconds(1));

        filesystem::remove(configDir1 / "test1.json");
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> modified, first > second -> first < second
      // should not happen
    }
    { // case: removed -> modified, first < second -> first > second
      // should not happen
    }
    { // case: removed -> removed, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(0U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        ClearConfig();
    }
    { // case: removed -> removed, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(0U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        ClearConfig();
    }
    { // case: removed -> removed, first > second -> first < second
      // should not happen
    }
    { // case: removed -> removed, first < second -> first > second
      // should not happen
    }
    { // case: removed -> unchanged, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> unchanged, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir1 / "test1.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: removed -> unchanged, first > second -> first < second
      // should not happen
    }
    { // case: removed -> unchanged, first < second -> first > second
      // should not happen
    }
    { // case: unchanged -> added, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> added, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> added, first > second -> first < second
      // should not happen
    }
    { // case: unchanged -> added, first < second -> first > second
      // should not happen
    }
    { // case: unchanged -> modified, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> modified, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedGreaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> modified, first > second -> first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> modified, first < second -> first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test3.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << modifiedLessPriorityConfig;
        fout.close();
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test3", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> removed, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> removed, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        filesystem::remove(configDir2 / "test2.json");
        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(1, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> removed, first > second -> first < second
      // should not happen
    }
    { // case: unchanged -> removed, first < second -> first > second
      // should not happen
    }
    { // case: unchanged -> unchanged, first > second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test1", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> unchanged, first < second
        PrepareConfig();
        ofstream fout(configDir1 / "test1.json", ios::trunc);
        fout << lessPriorityConfig;
        fout.close();
        fout.open(configDir2 / "test2.json", ios::trunc);
        fout << greaterPriorityConfig;
        fout.close();
        auto diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

        diff = PipelineConfigWatcher::GetInstance()->CheckConfigDiff();
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mAdded.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mModified.size());
        APSARA_TEST_EQUAL_FATAL(0, diff.first.mRemoved.size());

        PipelineManagerMock::GetInstance()->UpdatePipelines(diff.first);
        APSARA_TEST_EQUAL_FATAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
        APSARA_TEST_EQUAL_FATAL("test2", PipelineManagerMock::GetInstance()->GetAllConfigNames()[0]);
        ClearConfig();
    }
    { // case: unchanged -> unchanged, first > second -> first < second
      // should not happen
    }
    { // case: unchanged -> unchanged, first < second -> first > second
      // should not happen
    }
}

UNIT_TEST_CASE(PipelineConfigWatcherUnittest, TestLoadSingletonConfig)

} // namespace logtail

UNIT_TEST_MAIN
