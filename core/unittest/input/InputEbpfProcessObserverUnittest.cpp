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
#include "input/InputEbpfProcessObserver.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEbpfProcessObserverUnittest : public testing::Test {
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

void InputEbpfProcessObserverUnittest::OnSuccessfulInit() {
    unique_ptr<InputEbpfProcessObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;

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
    input.reset(new InputEbpfProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_observer");
    APSARA_TEST_EQUAL(ObserverType::PROCESS, input->mObserverOption.mObserver->mType);
    ObserverProcess* thisObserver = dynamic_cast<ObserverProcess*>(input->mObserverOption.mObserver);
    APSARA_TEST_EQUAL(thisObserver->mIncludeCmdRegex.size(), 1);
    APSARA_TEST_EQUAL("h", thisObserver->mIncludeCmdRegex[0]);
    APSARA_TEST_EQUAL(thisObserver->mExcludeCmdRegex.size(), 2);
    APSARA_TEST_EQUAL("m", thisObserver->mExcludeCmdRegex[0]);
    APSARA_TEST_EQUAL("n", thisObserver->mExcludeCmdRegex[1]);
}

void InputEbpfProcessObserverUnittest::OnFailedInit() {
    unique_ptr<InputEbpfProcessObserver> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;

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
    input.reset(new InputEbpfProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_observer",
            "IncludeCmdRegex": 1,
            "ExcludeCmdRegex": ["m","n"]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessObserver());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));
}


// void InputEbpfProcessObserverUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEbpfProcessObserverUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEbpfProcessObserverUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEbpfProcessObserverUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
