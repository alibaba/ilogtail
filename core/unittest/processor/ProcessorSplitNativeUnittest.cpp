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
#include "processor/ProcessorSplitNative.h"
#include "unittest/Unittest.h"

namespace logtail {


class ProcessorSplitNativeUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }

    void TestInit();
    void TestAppendingLogPositionMeta();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorSplitNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorSplitNativeUnittest, TestAppendingLogPositionMeta);

void ProcessorSplitNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    config["AppendingLogPositionMeta"] = false;
    ProcessorSplitNative processor;
    processor.SetContext(mContext);

    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorSplitNativeUnittest::TestAppendingLogPositionMeta() {
    // make config
    Json::Value config;
    config["AppendingLogPositionMeta"] = true;
    // make processor
    // ProcessorSplitNative
    ProcessorSplitNative processorSplitNative;
    processorSplitNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "content" : "line1\ncontinue\nline2\ncontinue"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    std::string logPath("/var/log/message");
    // run test function
    processorSplitNative.Process(eventGroup);
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "content" : "line1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": ")"
               << strlen(R"(line1n)") << R"(",
                    "content" : "continue"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": ")"
               << strlen(R"(line1ncontinuen)") << R"(",
                    "content" : "line2"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": ")"
               << strlen(R"(line1ncontinuenline2n)") << R"(",
                    "content" : "continue"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    // check observability
    APSARA_TEST_EQUAL_FATAL(4, processorSplitNative.GetContext().GetProcessProfile().feedLines);
    APSARA_TEST_EQUAL_FATAL(4, processorSplitNative.GetContext().GetProcessProfile().splitLines);
}

} // namespace logtail

UNIT_TEST_MAIN