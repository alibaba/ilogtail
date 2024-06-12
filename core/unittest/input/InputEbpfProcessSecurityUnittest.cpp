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
                                "Type": "Pid",
                                "ValueList": [
                                    "4026531833"
                                ]
                            },
                            {
                                "Type": "Mnt",
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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    SecurityFilter* thisFilterBase1 = input->mSecurityOptions.mOptionList[0]->mFilter;
    SecurityProcessFilter* thisFilter1 = dynamic_cast<SecurityProcessFilter*>(thisFilterBase1);
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, thisFilter1->mFilterType);
    APSARA_TEST_EQUAL("4026531833", thisFilter1->mNamespaceFilter[0]->mValueList[0]);
    APSARA_TEST_EQUAL("Pid", thisFilter1->mNamespaceFilter[0]->mType);
    APSARA_TEST_EQUAL("4026531834", thisFilter1->mNamespaceFilter[1]->mValueList[0]);
    APSARA_TEST_EQUAL("Mnt", thisFilter1->mNamespaceFilter[1]->mType);

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
                                "Type": "Pid",
                                "ValueList": [
                                    "4026531833"
                                ]
                            },
                            {
                                "Type": "Mnt",
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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    SecurityFilter* thisFilterBase2 = input->mSecurityOptions.mOptionList[0]->mFilter;
    SecurityProcessFilter* thisFilter2 = dynamic_cast<SecurityProcessFilter*>(thisFilterBase2);
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, thisFilter2->mFilterType);
    APSARA_TEST_EQUAL("4026531833", thisFilter2->mNamespaceBlackFilter[0]->mValueList[0]);
    APSARA_TEST_EQUAL("Pid", thisFilter2->mNamespaceBlackFilter[0]->mType);
    APSARA_TEST_EQUAL("4026531834", thisFilter2->mNamespaceBlackFilter[1]->mValueList[0]);
    APSARA_TEST_EQUAL("Mnt", thisFilter2->mNamespaceBlackFilter[1]->mType);

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
    APSARA_TEST_TRUE(input->Init(configJson, optionalGoPipeline));
    APSARA_TEST_EQUAL(input->sName, "input_ebpf_processprobe_security");
    SecurityFilter* thisFilterBase3 = input->mSecurityOptions.mOptionList[0]->mFilter;
    SecurityProcessFilter* thisFilter3 = dynamic_cast<SecurityProcessFilter*>(thisFilterBase3);
    APSARA_TEST_EQUAL(SecurityFilterType::PROCESS, thisFilter3->mFilterType);
}

void InputEbpfProcessSecurityUnittest::OnFailedInit() {
    unique_ptr<InputEbpfProcessSecurity> input;
    Json::Value configJson, optionalGoPipeline;
    string configStr, errorMsg;

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
                                "Type": "Pid",
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
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

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
                                "Type": "Pid",
                                "ValueList": ["4026531833"]
                            }
                        ],
                        "NamespaceFilter": [
                            {
                                "Type": "Pid",
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
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

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
    //                             "Type": "Pid",
    //                             "ValueList": ["4026531833"]
    //                         }
    //                     ],
    //                     "NamespaceFilter": [
    //                         {
    //                             "Type": "Pid",
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
    // APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));

    // error param level
    configStr = R"(
        {
            "Type": "input_ebpf_processprobe_security",
            "ConfigList": [
                {   
                    "NamespaceFilter": [
                        {
                            "Type": "Pid",
                            "ValueList": [
                                "4026531833"
                            ]
                        },
                        {
                            "Type": "Mnt",
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
    APSARA_TEST_FALSE(input->Init(configJson, optionalGoPipeline));
}


// void InputEbpfProcessSecurityUnittest::OnPipelineUpdate() {
// }


UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnSuccessfulInit)
UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnFailedInit)
// UNIT_TEST_CASE(InputEbpfProcessSecurityUnittest, OnPipelineUpdate)

} // namespace logtail

UNIT_TEST_MAIN
