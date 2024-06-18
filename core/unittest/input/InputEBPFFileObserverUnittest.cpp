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

#include <json/json.h>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "ebpf/observer/ObserverOptions.h"
#include "ebpf/observer/ObserverServer.h"
#include "input/InputEBPFFileObserver.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEBPFFileObserverUnittest : public testing::Test {
public:
    void OnSuccessfulInit();
    void OnFailedInit();
    // void OnPipelineUpdate();

protected:
    void SetUp() override {
        p.mName = "test_config";
        ctx.SetConfigName("test_config");
        ctx.SetPipeline(p);
    }

private:
    Pipeline p;
    PipelineContext ctx;
};

void InputEBPFFileObserverUnittest::OnSuccessfulInit() {
    unique_ptr<InputEBPFFileObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // valid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_profilingprobe_observer",
            "ProbeConfig": 
            {
                "ProfileRemoteServer": "",
                "CpuSkipUpload": false,
                "MemSkipUpload": false
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFFileObserver());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_profilingprobe_observer");
    ObserverFileOption thisObserver = std::get<ObserverFileOption>(input->mObserverOptions.mObserverOption);
    APSARA_TEST_EQUAL(ObserverType::FILE, input->mObserverOptions.mType);
    APSARA_TEST_EQUAL("", thisObserver.mProfileRemoteServer);
    APSARA_TEST_EQUAL(false, thisObserver.mCpuSkipUpload);
    APSARA_TEST_EQUAL(false, thisObserver.mMemSkipUpload);
}

void InputEBPFFileObserverUnittest::OnFailedInit() {
    unique_ptr<InputEBPFFileObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // invalid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_profilingprobe_observer",
            "ProbeConfig": 
            {
                "ProfileRemoteServer": 1,
                "CpuSkipUpload": false,
                "MemSkipUpload": false
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFFileObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_profilingprobe_observer",
            "ProfileRemoteServer": "",
            "CpuSkipUpload": false,
            "MemSkipUpload": false
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFFileObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));
}


// void InputEBPFFileObserverUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEBPFFileObserverUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEBPFFileObserverUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEBPFFileObserverUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
