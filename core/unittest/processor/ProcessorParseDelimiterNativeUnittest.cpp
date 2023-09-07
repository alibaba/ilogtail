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
#include "processor/ProcessorParseDelimiterNative.h"
#include "models/LogEvent.h"

namespace logtail {

class ProcessorParseDelimiterNativeUnittest : public ::testing::Test {
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

UNIT_TEST_CASE(ProcessorParseDelimiterNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseDelimiterNativeUnittest, TestProcessWholeLine);
UNIT_TEST_CASE(ProcessorParseDelimiterNativeUnittest, TestAddLog);
UNIT_TEST_CASE(ProcessorParseDelimiterNativeUnittest, TestProcessEventKeepUnmatch);

void ProcessorParseDelimiterNativeUnittest::TestInit() {
    Config config;
    config.mSeparator = ",";
    config.mColumnKeys = {"time", "method", "url", "request_time"};
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";

    ProcessorParseDelimiterNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorParseDelimiterNativeUnittest::TestProcessWholeLine() {
    // make config
    Config config;
    config.mSeparator = ",";
    config.mColumnKeys = {"time", "method", "url", "request_time"};
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "2013-10-31 21:03:49,POST,PutData?Category\n=YunOsAccountOpLog\n,0.024",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "2013-10-31 21:04:49,POST,PutData?Category\n=YunOsAccountOpLog\n,0.024",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseDelimiterNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.Process(eventGroup);
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "2013-10-31 21:03:49,POST,PutData?Category\n=YunOsAccountOpLog\n,0.024",
                    "log.file.offset": "0",
                    "method": "POST",
                    "request_time": "0.024",
                    "time": "2013-10-31 21:03:49",
                    "url": "PutData?Category\n=YunOsAccountOpLog\n"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "2013-10-31 21:04:49,POST,PutData?Category\n=YunOsAccountOpLog\n,0.024",
                    "log.file.offset": "0",
                    "method": "POST",
                    "request_time": "0.024",
                    "time": "2013-10-31 21:04:49",
                    "url": "PutData?Category\n=YunOsAccountOpLog\n"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseDelimiterNativeUnittest::TestAddLog() {
    Config config;
    ProcessorParseDelimiterNative processor;
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


void ProcessorParseDelimiterNativeUnittest::TestProcessEventKeepUnmatch() {
    // make config
    Config config;
    config.mSeparator = ",";
    config.mColumnKeys = {"time", "method", "url", "request_time"};
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
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
    ProcessorParseDelimiterNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.ProcessEvent("/var/log/message", logEvent);
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