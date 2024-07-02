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
#include "input/InputEBPFNetworkSecurity.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEBPFNetworkSecurityUnittest : public testing::Test {
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

void InputEBPFNetworkSecurityUnittest::OnSuccessfulInit() {
    unique_ptr<InputEBPFNetworkSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

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
    input.reset(new InputEBPFNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_sockettraceprobe_security");
    SecurityNetworkFilter thisFilter1 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(SecurityFilterType::NETWORK, input->mSecurityOptions.mFilterType);
    APSARA_TEST_EQUAL("tcp_connect", input->mSecurityOptions.mOptionList[0].mCallName[0]);
    APSARA_TEST_EQUAL("tcp_close", input->mSecurityOptions.mOptionList[0].mCallName[1]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter1.mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter1.mDestAddrList[1]);
    APSARA_TEST_EQUAL(80, thisFilter1.mDestPortList[0]);
    APSARA_TEST_EQUAL("127.0.0.1/8", thisFilter1.mSourceAddrBlackList[0]);
    APSARA_TEST_EQUAL(9300, thisFilter1.mSourcePortBlackList[0]);

    SecurityNetworkFilter thisFilter2 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL("tcp_sendmsg", input->mSecurityOptions.mOptionList[1].mCallName[0]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter2.mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter2.mDestAddrList[1]);
    APSARA_TEST_EQUAL(80, thisFilter2.mDestPortList[0]);
}

void InputEBPFNetworkSecurityUnittest::OnFailedInit() {
    unique_ptr<InputEBPFNetworkSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

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
    input.reset(new InputEBPFNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_sockettraceprobe_security");
    SecurityNetworkFilter thisFilter1 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(SecurityFilterType::NETWORK, input->mSecurityOptions.mFilterType);
    APSARA_TEST_EQUAL("tcp_connect", input->mSecurityOptions.mOptionList[0].mCallName[0]);
    APSARA_TEST_EQUAL("tcp_close", input->mSecurityOptions.mOptionList[0].mCallName[1]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter1.mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter1.mDestAddrList[1]);
    APSARA_TEST_EQUAL(0, thisFilter1.mDestPortList.size());
    APSARA_TEST_EQUAL("127.0.0.1/8", thisFilter1.mSourceAddrBlackList[0]);
    APSARA_TEST_EQUAL(9300, thisFilter1.mSourcePortBlackList[0]);

    SecurityNetworkFilter thisFilter2 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[1].mFilter);
    APSARA_TEST_EQUAL("tcp_sendmsg", input->mSecurityOptions.mOptionList[1].mCallName[0]);
    APSARA_TEST_EQUAL("10.0.0.0/8", thisFilter2.mDestAddrList[0]);
    APSARA_TEST_EQUAL("92.168.0.0/16", thisFilter2.mDestAddrList[1]);
    APSARA_TEST_EQUAL(80, thisFilter2.mDestPortList[0]);

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
    input.reset(new InputEBPFNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    SecurityNetworkFilter thisFilter3 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(0, thisFilter3.mDestAddrList.size());
    APSARA_TEST_EQUAL(0, thisFilter3.mDestPortList.size());

    // valid and invalid optional param
    // if the optional param in a list is invalid, the valid param will be ignored only when after it
    configStr = R"(
        {
            "Type": "input_ebpf_sockettraceprobe_security",
            "ConfigList": [
                {
                    "CallName": ["tcp_connect", "tcp_close"],
                    "Filter": {
                        "DestAddrList": ["10.0.0.0/8","92.168.0.0/16"],
                        "DestPortList": [40, "80", 160],
                        "SourceAddrBlackList": ["127.0.0.1/8"],
                        "SourcePortBlackList": [9300]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEBPFNetworkSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    SecurityNetworkFilter thisFilter4 = std::get<SecurityNetworkFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(2, thisFilter4.mDestAddrList.size());
    APSARA_TEST_EQUAL(1, thisFilter4.mDestPortList.size());
}


// void InputEBPFNetworkSecurityUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEBPFNetworkSecurityUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEBPFNetworkSecurityUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEBPFNetworkSecurityUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
