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
#include "processor/ProcessorParseRegexNative.h"
#include "models/LogEvent.h"

namespace logtail {

class ProcessorParseRegexNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }

    void TestInit();
    void TestProcessWholeLine();
    void TestProcessRegex();
    void TestAddLog();
    void TestProcessEventKeepUnmatch();
    void TestProcessEventDiscardUnmatch();
    void TestProcessEventKeyCountUnmatch();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestProcessWholeLine);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestProcessRegex);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestAddLog);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestProcessEventKeepUnmatch);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestProcessEventDiscardUnmatch);
UNIT_TEST_CASE(ProcessorParseRegexNativeUnittest, TestProcessEventKeyCountUnmatch);


void ProcessorParseRegexNativeUnittest::TestInit() {
    Config config;
    config.mLogBeginReg = ".*";
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back("(.*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("content");

    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorParseRegexNativeUnittest::TestProcessWholeLine() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back("(.*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("content");
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "line1\nline2",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "line3\nline4",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(inJson).c_str(), CompactJson(outJson).c_str());
    // metric
    APSARA_TEST_EQUAL_FATAL(2, processor.mProcInRecordsTotal->GetValue());
    std::string expectValue = "line1\nline2";
    APSARA_TEST_EQUAL_FATAL((expectValue.length())*2, processor.mProcInRecordsSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(2, processor.mProcOutRecordsTotal->GetValue());
    expectValue = "contentline1\nline2";
    APSARA_TEST_EQUAL_FATAL((expectValue.length() + 5)*2, processor.mProcOutRecordsSizeBytes->GetValue());

    APSARA_TEST_EQUAL_FATAL(0, processor.mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcParseErrorTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcKeyCountNotMatchErrorTotal->GetValue());
}

void ProcessorParseRegexNativeUnittest::TestProcessRegex() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back(R"((\w+)\t(\w+).*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("key1,key2");
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "value1\tvalue2",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value3\tvalue4",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    processor.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__raw__" : "value1\tvalue2",
                    "key1" : "value1",
                    "key2" : "value2",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw__" : "value3\tvalue4",
                    "key1" : "value3",
                    "key2" : "value4",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    APSARA_TEST_GT_FATAL(processor.mProcTimeMS->GetValue(), 0);
}

void ProcessorParseRegexNativeUnittest::TestAddLog() {
    Config config;
    ProcessorParseRegexNative processor;
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

void ProcessorParseRegexNativeUnittest::TestProcessEventKeepUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back(R"((\w+)\t(\w+).*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("key1,key2");
    
    // run function
    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    int count = 10;
    for (int i = 0; i < count; i ++) {
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
    }
    // check observablity
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().regexMatchFailures);
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length())*count, processor.mProcInRecordsSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcOutRecordsTotal->GetValue());
    expectValue = "__raw_log__value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length() + 5)*count, processor.mProcOutRecordsSizeBytes->GetValue());

    APSARA_TEST_EQUAL_FATAL(0, processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcParseErrorTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcKeyCountNotMatchErrorTotal->GetValue());
}

void ProcessorParseRegexNativeUnittest::TestProcessEventDiscardUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = true;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back(R"((\w+)\t(\w+).*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("key1,key2");
    
    // run function
    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    int count = 10;
    for (int i = 0; i < count; i ++) {
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
        processor.ProcessEvent("/var/log/message", logEvent);
    }
    // check observablity
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().regexMatchFailures);
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length())*count, processor.mProcInRecordsSizeBytes->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcOutRecordsSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcParseErrorTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcKeyCountNotMatchErrorTotal->GetValue());
}

void ProcessorParseRegexNativeUnittest::TestProcessEventKeyCountUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = true;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back(R"((\w+)\t(\w+).*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("key1,key2,key3");
    
    // run function
    ProcessorParseRegexNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    int count = 10;
    for (int i = 0; i < count; i ++) {
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        auto logEvent = PipelineEventPtr(LogEvent::CreateEvent(sourceBuffer));
        std::string inJson = R"({
            "contents" :
            {
                "content" : "value1\tvalue2",
                "log.file.offset": "0"
            },
            "timestamp" : 12345678901,
            "type" : 1
        })";
        logEvent->FromJsonString(inJson);
        processor.ProcessEvent("/var/log/message", logEvent);
    }
    // check observablity
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().regexMatchFailures);
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1\tvalue2";
    APSARA_TEST_EQUAL_FATAL((expectValue.length())*count, processor.mProcInRecordsSizeBytes->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcOutRecordsSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcDiscardRecordsTotal->GetValue());

    // mProcKeyCountNotMatchErrorTotal should equal count 
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcParseErrorTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcKeyCountNotMatchErrorTotal->GetValue());
}

} // namespace logtail

UNIT_TEST_MAIN
