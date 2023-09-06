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
#include "processor/ProcessorParseApsaraNative.h"
#include "models/LogEvent.h"

namespace logtail {

class ProcessorParseApsaraNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
        BOOL_FLAG(ilogtail_discard_old_data) = false;
    }

    void TestInit();
    void TestProcessWholeLine();
    void TestAddLog();
    void TestProcessEventKeepUnmatch();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessWholeLine);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestAddLog);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessEventKeepUnmatch);

void ProcessorParseApsaraNativeUnittest::TestInit() {
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mLogTimeZoneOffsetSecond = 0;

    ProcessorParseApsaraNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorParseApsaraNativeUnittest::TestProcessWholeLine() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15:04.862181]	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:04.862181]	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.Process(eventGroup);
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "content" : "[2023-09-04 13:15:04.862181]\t[info]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0",
                    "microtime": "1693833304862181"
                },
                "timestamp" : 1693833304,
                "type" : 1
            },
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "content" : "[2023-09-04 13:16:04.862181]\t[info]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0",
                    "microtime": "1693833364862181"
                },
                "timestamp" : 1693833364,
                "type" : 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestAddLog() {
    Config config;
    ProcessorParseApsaraNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));

    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = LogEvent::CreateEvent(sourceBuffer);
    char key[] = "key";
    char value[] = "value";
    processor.AddLog(key, value, *logEvent);
    // check observability
    APSARA_TEST_EQUAL_FATAL(strlen(key) + strlen(value) + 5, processor.GetContext().GetProcessProfile().logGroupSize);
}

void ProcessorParseApsaraNativeUnittest::TestProcessEventKeepUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mLogTimeZoneOffsetSecond = 0;
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = PipelineEventPtr(LogEvent::CreateEvent(sourceBuffer));
    std::string inJson = R"({
        "contents" :
        {
            "content" : "value1",
            "log.file.offset": "0"
        },
        "timestamp" : 12345678901,
        "type" : 1
    })";
    logEvent->FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    time_t logTime;
    logtail::StringView timeStrCache;
    processor.ProcessEvent("/var/log/message", logEvent, logTime, timeStrCache);
    // judge result
    std::string expectJson = R"({
        "contents" :
        {
            "__raw_log__" : "value1",
            "content" : "value1",
            "log.file.offset": "0"
        },
        "timestamp" : 12345678901,
        "type" : 1
    })";
    std::string outJson = logEvent->ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    // check observablity
    APSARA_TEST_EQUAL_FATAL(1, processor.GetContext().GetProcessProfile().parseFailures);
}

} // namespace logtail

UNIT_TEST_MAIN