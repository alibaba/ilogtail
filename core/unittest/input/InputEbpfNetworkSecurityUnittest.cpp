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
#include "ebpf/security/SecurityOptions.h"
#include "ebpf/security/SecurityServer.h"
#include "input/InputEbpfNetworkSecurity.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEbpfNetworkSecurityUnittest : public testing::Test {
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

void InputEbpfNetworkSecurityUnittest::OnSuccessfulInit() {
    unique_ptr<InputEbpfNetworkSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;

    // valid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_sockettraceprobe_security",
            "ConfigList": [
                {
                    "CallName": ["tcp_connect", "tcp_close"],
                    "Filter": {
                        "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                        "DestPortList": [80],
                        "SourceAddrBlackList": ["127.0.0.1/8"],
                        "SourcePortBlackList": [9300]
                    }
                },
                {
                    "CallName": ["tcp_sendmsg"],
                    "Filter": {
                        "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                        "DestPortList": [80]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_sockettraceprobe_security");
    SecurityFilter* thisFilterBase1 = input->mSecurityOptions.mOptionList[0]->mFilter;
    SecurityNetworkFilter* thisFilter1 = dynamic_cast<SecurityNetworkFilter*>(thisFilterBase1);
    APSARA_TEST_EQUAL(SecurityFilterType::NETWORK, thisFilter1->mFilterType);
    APSARA_TEST_EQUAL("tcp_connect", input->mSecurityOptions.mOptionList[0]->mCallName[0]);
    APSARA_TEST_EQUAL("tcp_close", input->mSecurityOptions.mOptionList[0]->mCallName[1]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter1->mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter1->mDestAddrList[1]);
    APSARA_TEST_EQUAL(80, thisFilter1->mDestPortList[0]);
    APSARA_TEST_EQUAL("127.0.0.1/8", thisFilter1->mSourceAddrBlackList[0]);
    APSARA_TEST_EQUAL(9300, thisFilter1->mSourcePortBlackList[0]);

    SecurityFilter* thisFilterBase2 = input->mSecurityOptions.mOptionList[1]->mFilter;
    SecurityNetworkFilter* thisFilter2 = dynamic_cast<SecurityNetworkFilter*>(thisFilterBase2);
    APSARA_TEST_EQUAL("tcp_sendmsg", input->mSecurityOptions.mOptionList[1]->mCallName[0]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter2->mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter2->mDestAddrList[1]);
    APSARA_TEST_EQUAL(80, thisFilter2->mDestPortList[0]);
}

void InputEbpfNetworkSecurityUnittest::OnFailedInit() {
    unique_ptr<InputEbpfNetworkSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;

    // invalid optional param
    configStr = R"(
        {
            "Type": "input_ebpf_sockettraceprobe_security",
            "ConfigList": [
                {
                    "CallName": ["tcp_connect", "tcp_close"],
                    "Filter": {
                        "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                        "DestPortList": ["80"],
                        "SourceAddrBlackList": ["127.0.0.1/8"],
                        "SourcePortBlackList": [9300]
                    }
                },
                {
                    "CallName": ["tcp_sendmsg"],
                    "Filter": {
                        "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                        "DestPortList": [80]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_sockettraceprobe_security",
            "ConfigList": [
                {
                    "CallName": ["tcp_connect", "tcp_close"],
                    "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                    "DestPortList": ["80"],
                    "SourceAddrBlackList": ["127.0.0.1/8"],
                    "SourcePortBlackList": [9300]
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));
}


// void InputEbpfNetworkSecurityUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEbpfNetworkSecurityUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEbpfNetworkSecurityUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEbpfNetworkSecurityUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
