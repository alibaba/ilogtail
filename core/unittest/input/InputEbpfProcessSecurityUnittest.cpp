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
#include "input/InputEbpfProcessSecurity.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineContext.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class InputEbpfProcessSecurityUnittest : public testing::Test {
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

void InputEbpfProcessSecurityUnittest::OnSuccessfulInit() {
    unique_ptr<InputEbpfProcessSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // only NamespaceFilter
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {
                    "Filter": 
                    {
                        "NamespaceFilter": [
                            {
                                "NamespaceType": "Pid",
                                "ValueList": [
                                    "4026531833"
                                ]
                            },
                            {
                                "NamespaceType": "Mnt",
                                "ValueList": [
                                    "4026531834"
                                ]
                            }
                        ]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    SecurityProcessFilter thisFilter1 = std::get<SecurityProcessFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, input->mSecurityOptions.mFilterType);
    APSARA_TEST_EQUAL("4026531833", thisFilter1.mNamespaceFilter[0].mValueList[0]);
    APSARA_TEST_EQUAL("Pid", thisFilter1.mNamespaceFilter[0].mNamespaceType);
    APSARA_TEST_EQUAL("4026531834", thisFilter1.mNamespaceFilter[1].mValueList[0]);
    APSARA_TEST_EQUAL("Mnt", thisFilter1.mNamespaceFilter[1].mNamespaceType);

    // only NamespaceBlackFilter
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {
                    "Filter": 
                    {
                        "NamespaceBlackFilter": [
                            {
                                "NamespaceType": "Pid",
                                "ValueList": [
                                    "4026531833"
                                ]
                            },
                            {
                                "NamespaceType": "Mnt",
                                "ValueList": [
                                    "4026531834"
                                ]
                            }
                        ]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    SecurityProcessFilter thisFilter2 = std::get<SecurityProcessFilter>(input->mSecurityOptions.mOptionList[0].mFilter);
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, input->mSecurityOptions.mFilterType);
    APSARA_TEST_EQUAL("4026531833", thisFilter2.mNamespaceBlackFilter[0].mValueList[0]);
    APSARA_TEST_EQUAL("Pid", thisFilter2.mNamespaceBlackFilter[0].mNamespaceType);
    APSARA_TEST_EQUAL("4026531834", thisFilter2.mNamespaceBlackFilter[1].mValueList[0]);
    APSARA_TEST_EQUAL("Mnt", thisFilter2.mNamespaceBlackFilter[1].mNamespaceType);

    // no NamespaceFilter and NamespaceBlackFilter
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {
                    "Filter": 
                    {
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_TRUE(input->Init(configJson, pluginIdx, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, input->mSecurityOptions.mFilterType);
}

void InputEbpfProcessSecurityUnittest::OnFailedInit() {
    unique_ptr<InputEbpfProcessSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;
    uint32_t pluginIdx = 0;

    // invalid param
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {
                    "Filter": 
                    {
                        "NamespaceBlackFilter": [
                            {
                                "NamespaceType": "Pid",
                                "ValueList": "4026531833"
                            }
                        ]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));

    // invalid param: 1 NamespaceFilter and 1 NamespaceBlackFilter
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {
                    "Filter": 
                    {
                        "NamespaceBlackFilter": [
                            {
                                "NamespaceType": "Pid",
                                "ValueList": ["4026531833"]
                            }
                        ],
                        "NamespaceFilter": [
                            {
                                "NamespaceType": "Pid",
                                "ValueList": ["4026531833"]
                            }
                        ]
                    }
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));

    // // invalid param: 2 NamespaceFilter
    // configStr = R"(
    //     {
    //         "Type": "input_ebpf_processprobe_security",
    //         "ConfigList": [
    //             {
    //                 "Filter":
    //                 {
    //                     "NamespaceFilter": [
    //                         {
    //                             "NamespaceType": "Pid",
    //                             "ValueList": ["4026531833"]
    //                         }
    //                     ],
    //                     "NamespaceFilter": [
    //                         {
    //                             "NamespaceType": "Pid",
    //                             "ValueList": ["4026531833"]
    //                         }
    //                     ]
    //                 }
    //             }
    //         ]
    //     }
    // )";
    // APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    // input.reset(new InputEbpfProcessSecurity());
    // input->SetContext(ctx);
    // APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {   
                    "NamespaceFilter": [
                        {
                            "NamespaceType": "Pid",
                            "ValueList": [
                                "4026531833"
                            ]
                        },
                        {
                            "NamespaceType": "Mnt",
                            "ValueList": [
                                "4026531834"
                            ]
                        }
                    ]
                }
            ]
        }
    )";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    input.reset(new InputEbpfProcessSecurity());
    input->SetContext(ctx);
    APSARA_TEST_FALSE(input->Init(configJson, pluginIdx, optionalGoPipeline));
}


// void InputEbpfProcessSecurityUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
