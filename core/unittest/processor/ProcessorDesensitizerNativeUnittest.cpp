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
#include <cstdlib>
#include "unittest/Unittest.h"

#include "common/JsonUtil.h"
#include "config/Config.h"
#include "processor/ProcessorParseJsonNative.h"
#include "models/LogEvent.h"
#include "plugin/ProcessorInstance.h"

namespace logtail {

class ProcessorDesensitizerNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }


    void TestInit();

    Config* GetCastSensWordConfig(string , string , int , bool , string );


    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorDesensitizerNativeUnittest, TestInit);

Config* ProcessorDesensitizerNativeUnittest::GetCastSensWordConfig(string key = string("cast1"),
                                                                   string regex = string("(pwd=)[^,]+"),
                                                                   int type = SensitiveWordCastOption::CONST_OPTION,
                                                                   bool replaceAll = false,
                                                                   string constVal = string("\\1********")) {
    Config* oneConfig = new Config;
    vector<SensitiveWordCastOption>& optVec = oneConfig->mSensitiveWordCastOptions[key];

    optVec.resize(1);
    optVec[0].option = SensitiveWordCastOption::CONST_OPTION;
    optVec[0].key = key;
    optVec[0].constValue = constVal;
    optVec[0].replaceAll = replaceAll;
    optVec[0].mRegex.reset(new re2::RE2(regex));
    return oneConfig;
}

void ProcessorDesensitizerNativeUnittest::TestInit() {
    Config* config = GetCastSensWordConfig();
    // run function
    ProcessorDesensitizerNative& processor = *(new ProcessorDesensitizerNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    ComponentConfig componentConfig(pluginId, &config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
}


} // namespace logtail

UNIT_TEST_MAIN
