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

#include "common/Constants.h"
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "models/LogEvent.h"
#include "processor/ProcessorMergeMultilineLogNative.h"
#include "processor/ProcessorSplitNative.h"
#include "unittest/Unittest.h"

namespace logtail {


class ProcessorSplitNativeUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }

    void TestInit();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorSplitNativeUnittest, TestInit);

void ProcessorSplitNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    config["StartPattern"] = ".*";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    ProcessorMergeMultilineLogNative processor;
    processor.SetContext(mContext);

    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

} // namespace logtail

UNIT_TEST_MAIN