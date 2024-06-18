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
#include "input/InputEBPFProcessObserver.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEBPFProcessObserverUnittest : public testing::Test {
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

void InputEBPFProcessObserverUnittest::OnSuccessfulInit() {
    unique_ptr<InputEBPFProcessObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // valid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_observer",
            "ProbeConfig": 
            {
                "IncludeCmdRegex": ["h"],
                "ExcludeCmdRegex": ["m","n"]
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_observer");
    ObserverProcessOption thisObserver = std::get<ObserverProcessOption>(input->mObserverOptions.mObserverOption);
    APSARA_TEST_EQUAL(ObserverType::PROCESS, input->mObserverOptions.mType);
    APSARA_TEST_EQUAL(thisObserver.mIncludeCmdRegex.size(), 1);
    APSARA_TEST_EQUAL("h", thisObserver.mIncludeCmdRegex[0]);
    APSARA_TEST_EQUAL(thisObserver.mExcludeCmdRegex.size(), 2);
    APSARA_TEST_EQUAL("m", thisObserver.mExcludeCmdRegex[0]);
    APSARA_TEST_EQUAL("n", thisObserver.mExcludeCmdRegex[1]);
}

void InputEBPFProcessObserverUnittest::OnFailedInit() {
    unique_ptr<InputEBPFProcessObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // invalid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_observer",
            "ProbeConfig": 
            {
                "IncludeCmdRegex": 1,
                "ExcludeCmdRegex": ["m","n"]
            }
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_observer",
            "IncludeCmdRegex": 1,
            "ExcludeCmdRegex": ["m","n"]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));
}


// void InputEBPFProcessObserverUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEBPFProcessObserverUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEBPFProcessObserverUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEBPFProcessObserverUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
