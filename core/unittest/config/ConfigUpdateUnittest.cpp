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

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "config/PipelineConfig.h"
#include "config/watcher/ConfigWatcher.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"
#include "pipeline/plugin/PluginRegistry.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class PipelineMock : public Pipeline {
public:
    bool Init(PipelineConfig&& config) {
        mConfig = std::move(config.mDetail);
        WriteMetrics::GetInstance()->PrepareMetricsRecordRef(
            mMetricsRecordRef,
            MetricCategory::METRIC_CATEGORY_PIPELINE,
            {{METRIC_LABEL_KEY_PROJECT, mContext.GetProjectName()}, {METRIC_LABEL_KEY_PIPELINE_NAME, mName}});
        mStartTime = mMetricsRecordRef.CreateIntGauge(METRIC_PIPELINE_START_TIME);
        return (*mConfig)["valid"].asBool();
    }
};

class PipelineManagerMock : public PipelineManager {
public:
    static PipelineManagerMock* GetInstance() {
        static PipelineManagerMock instance;
        return &instance;
    }

    void ClearEnvironment() {
        mPipelineNameEntityMap.clear();
        mPluginCntMap.clear();
    }

private:
    shared_ptr<Pipeline> BuildPipeline(PipelineConfig&& config) override {
        // this should be synchronized with PipelineManager::BuildPipeline, except for the pointer type.
        shared_ptr<PipelineMock> p = make_shared<PipelineMock>();
        if (!p->Init(std::move(config))) {
            return nullptr;
        }
        return p;
    }
};

class ConfigUpdateUnittest : public testing::Test {
public:
    void OnStartUp() const;
    void OnConfigDelete() const;
    void OnConfigToInvalidFormat() const;
    void OnConfigToInvalidDetail() const;
    void OnConfigToEnabledValid() const;
    void OnConfigToDisabledValid() const;
    void OnConfigUnchanged() const;
    void OnConfigAdded() const;

protected:
    static void SetUpTestCase() {
        PluginRegistry::GetInstance()->LoadPlugins();
        ConfigWatcher::GetInstance()->SetPipelineManager(PipelineManagerMock::GetInstance());
    }

    static void TearDownTestCase() { PluginRegistry::GetInstance()->UnloadPlugins(); }

    void SetUp() override {
        filesystem::create_directories(configDir);
        ConfigWatcher::GetInstance()->AddSource(configDir.string());
    }

    void TearDown() override {
        PipelineManagerMock::GetInstance()->ClearEnvironment();
        ConfigWatcher::GetInstance()->ClearEnvironment();
        filesystem::remove_all(configDir);
    }

private:
    void PrepareInitialSettings() const;
    void GenerateInitialConfigs() const;

    filesystem::path configDir = "./config";
    vector<filesystem::path> configPaths = {configDir / "invalid_format.json",
                                            configDir / "invalid_detail.json",
                                            configDir / "enabled_valid.json",
                                            configDir / "disabled_valid.json"};
    const string invalidConfigWithInvalidFormat = R"({"inputs":{}})";
    const string invalidConfigWithInvalidDetail = R"(
{
    "valid": false,
    "inputs": [
        {
            "Type": "input_file"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";
    const string enabledValidConfig = R"(
{
    "valid": true,
    "inputs": [
        {
            "Type": "input_file"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";
    const string disabledValidConfig = R"(
{
    "valid": true,
    "enable": false,
    "inputs": [
        {
            "Type": "input_file"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";

    const string newInvalidConfigWithInvalidFormat = R"({"flushers":{}})";
    const string newInvalidConfigWithInvalidDetail = R"(
{
    "valid": false,
    "inputs": [
        {
            "Type": "input_container_stdio"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";
    const string newEnabledValidConfig = R"(
{
    "valid": true,
    "inputs": [
        {
            "Type": "input_container_stdio"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";
    const string newDisabledValidConfig = R"(
{
    "valid": true,
    "enable": false,
    "inputs": [
        {
            "Type": "input_container_stdio"
        }
    ],
    "flushers": [
        {
            "Type": "flusher_sls"
        }
    ]
}
    )";
};

void ConfigUpdateUnittest::OnStartUp() const {
    PipelineConfigDiff diff;
    diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_TRUE(diff.IsEmpty());

    GenerateInitialConfigs();
    diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_EQUAL(2U, diff.mAdded.size());
    APSARA_TEST_TRUE(diff.mModified.empty());
    APSARA_TEST_TRUE(diff.mRemoved.empty());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
}

void ConfigUpdateUnittest::OnConfigDelete() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    filesystem::remove_all(configDir);
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_TRUE(diff.mAdded.empty());
    APSARA_TEST_TRUE(diff.mModified.empty());
    APSARA_TEST_EQUAL(1U, diff.mRemoved.size());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_TRUE(PipelineManagerMock::GetInstance()->GetAllConfigNames().empty());
}

void ConfigUpdateUnittest::OnConfigToInvalidFormat() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    for (const auto& path : configPaths) {
        ofstream fout(path, ios::trunc);
        fout << newInvalidConfigWithInvalidFormat;
    }
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_TRUE(diff.IsEmpty());
}

void ConfigUpdateUnittest::OnConfigToInvalidDetail() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    for (const auto& path : configPaths) {
        ofstream fout(path, ios::trunc);
        fout << newInvalidConfigWithInvalidDetail;
    }
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_EQUAL(3U, diff.mAdded.size());
    APSARA_TEST_EQUAL(1U, diff.mModified.size());
    APSARA_TEST_TRUE(diff.mRemoved.empty());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
}

void ConfigUpdateUnittest::OnConfigToEnabledValid() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    for (const auto& path : configPaths) {
        ofstream fout(path, ios::trunc);
        fout << newEnabledValidConfig;
    }
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_EQUAL(3U, diff.mAdded.size());
    APSARA_TEST_EQUAL(1U, diff.mModified.size());
    APSARA_TEST_TRUE(diff.mRemoved.empty());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_EQUAL(4U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
}

void ConfigUpdateUnittest::OnConfigToDisabledValid() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    for (const auto& path : configPaths) {
        ofstream fout(path, ios::trunc);
        fout << newDisabledValidConfig;
    }
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_TRUE(diff.mAdded.empty());
    APSARA_TEST_TRUE(diff.mModified.empty());
    APSARA_TEST_EQUAL(1U, diff.mRemoved.size());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_TRUE(PipelineManagerMock::GetInstance()->GetAllConfigNames().empty());
}

void ConfigUpdateUnittest::OnConfigUnchanged() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_TRUE(diff.IsEmpty());

    GenerateInitialConfigs();
    // mandatorily overwrite modify time in case of no update when file content remains the same.
    for (const auto& path : configPaths) {
        filesystem::file_time_type fTime = filesystem::last_write_time(path);
        filesystem::last_write_time(path, fTime + 1s);
    }
    diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_EQUAL(1U, diff.mAdded.size());
    APSARA_TEST_TRUE(diff.mModified.empty());
    APSARA_TEST_TRUE(diff.mRemoved.empty());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
}

void ConfigUpdateUnittest::OnConfigAdded() const {
    PrepareInitialSettings();
    APSARA_TEST_EQUAL(1U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());

    {
        ofstream fout(configDir / "add_invalid_format.json", ios::trunc);
        fout << invalidConfigWithInvalidFormat;
    }
    {
        ofstream fout(configDir / "add_invalid_detail.json", ios::trunc);
        fout << invalidConfigWithInvalidDetail;
    }
    {
        ofstream fout(configDir / "add_enabled_valid.json", ios::trunc);
        fout << enabledValidConfig;
    }
    {
        ofstream fout(configDir / "add_disabled_valid.json", ios::trunc);
        fout << disabledValidConfig;
    }
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    APSARA_TEST_FALSE(diff.IsEmpty());
    APSARA_TEST_EQUAL(2U, diff.mAdded.size());
    APSARA_TEST_TRUE(diff.mModified.empty());
    APSARA_TEST_TRUE(diff.mRemoved.empty());

    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
    APSARA_TEST_EQUAL(2U, PipelineManagerMock::GetInstance()->GetAllConfigNames().size());
}

void ConfigUpdateUnittest::PrepareInitialSettings() const {
    GenerateInitialConfigs();
    PipelineConfigDiff diff = ConfigWatcher::GetInstance()->CheckConfigDiff();
    PipelineManagerMock::GetInstance()->UpdatePipelines(diff);
}

void ConfigUpdateUnittest::GenerateInitialConfigs() const {
    {
        ofstream fout(configPaths[0], ios::trunc);
        fout << invalidConfigWithInvalidFormat;
    }
    {
        ofstream fout(configPaths[1], ios::trunc);
        fout << invalidConfigWithInvalidDetail;
    }
    {
        ofstream fout(configPaths[2], ios::trunc);
        fout << enabledValidConfig;
    }
    {
        ofstream fout(configPaths[3], ios::trunc);
        fout << disabledValidConfig;
    }
}

UNIT_TEST_CASE(ConfigUpdateUnittest, OnStartUp)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigDelete)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigToInvalidFormat)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigToInvalidDetail)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigToEnabledValid)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigToDisabledValid)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigUnchanged)
UNIT_TEST_CASE(ConfigUpdateUnittest, OnConfigAdded)

} // namespace logtail

UNIT_TEST_MAIN
