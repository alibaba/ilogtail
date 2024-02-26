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
#include "processor/ProcessorSplitLogStringNative.h"
#include "unittest/Unittest.h"

namespace logtail {

const std::string LOG_BEGIN_STRING = "Exception in thread 'main' java.lang.NullPointerException";
const std::string LOG_BEGIN_REGEX = R"(Exception.*)";
const std::string LOG_CONTINUE_STRING = "    at com.example.myproject.Book.getTitle(Book.java:16)";
const std::string LOG_CONTINUE_REGEX = R"(\s+at\s.*)";
const std::string LOG_END_STRING = "    ...23 more";
const std::string LOG_END_REGEX = R"(\s*\.\.\.\d+ more)";
const std::string LOG_UNMATCH = "unmatch log";

class ProcessorMergeMultilineLogNativeUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }
    void TestInit();
    void TestProcessEventSingleLine();
    void TestProcessEventMultiline();
    void TestProcessEventMultilineKeepUnmatch();
    void TestProcessEventMultilineDiscardUnmatch();
    void TestProcessEventMultilineAllNotMatchKeepUnmatch();
    void TestProcessEventMultilineAllNotMatchDiscardUnmatch();
    void TestProcess();
    void TestProcessEventsWithPartLog();
    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventSingleLine);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventMultiline);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventMultilineKeepUnmatch);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventMultilineDiscardUnmatch);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventMultilineAllNotMatchKeepUnmatch);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventMultilineAllNotMatchDiscardUnmatch);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcess);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventsWithPartLog);

void ProcessorMergeMultilineLogNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    config["StartPattern"] = ".*";
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    ProcessorMergeMultilineLogNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventSingleLine() {
    // make config
    Json::Value config;
    config["StartPattern"] = ".*";
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
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
                    "content" : "line1\nline2"
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
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "line1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "line2"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventMultiline() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : ")"
           << LOG_BEGIN_STRING << R"(first.\nmultiline1\nmultiline2\n)" << LOG_BEGIN_STRING
           << R"(second.\nmultiline1\nmultiline2)"
           << R"("
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());
    std::string logPath("/var/log/message");
    // run test function
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : ")"
               << LOG_BEGIN_STRING << R"(first.\nmultiline1\nmultiline2)"
               << R"("
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : ")"
               << LOG_BEGIN_STRING << R"(second.\nmultiline1\nmultiline2)"
               << R"("
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventMultilineKeepUnmatch() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : ")"
           << R"(first.\nmultiline1\nmultiline2\n)" << LOG_BEGIN_STRING << R"(second.\nmultiline1\nmultiline2)"
           << R"("
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());
    std::string logPath("/var/log/message");
    // run test function
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "first."
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "multiline1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "multiline2"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : ")"
               << LOG_BEGIN_STRING << R"(second.\nmultiline1\nmultiline2)"
               << R"("
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventMultilineDiscardUnmatch() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : ")"
           << R"(first.\nmultiline1\nmultiline2\n)" << LOG_BEGIN_STRING << R"(second.\nmultiline1\nmultiline2"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());
    std::string logPath("/var/log/message");
    // run test function
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : ")"
               << LOG_BEGIN_STRING << R"(second.\nmultiline1\nmultiline2"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventMultilineAllNotMatchKeepUnmatch() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "first.\nmultiline1\nsecond.\nmultiline1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());
    std::string logPath("/var/log/message");
    // run test function
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "first."
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "multiline1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "second."
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "multiline1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventMultilineAllNotMatchDiscardUnmatch() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "first.\nmultiline1\nsecond.\nmultiline1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());
    std::string logPath("/var/log/message");
    // run test function
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcess() {
    // make config
    Json::Value config;
    config["StartPattern"] = "line.*";
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = true;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
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
    processorSplitLogStringNative.Process(eventGroup);
    processorMergeMultilineLogNative.Process(eventGroup);
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "content" : "line1\ncontinue"
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
                    "content" : "line2\ncontinue"
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
    APSARA_TEST_EQUAL_FATAL(4, processorMergeMultilineLogNative.GetContext().GetProcessProfile().feedLines);
    APSARA_TEST_EQUAL_FATAL(2, processorMergeMultilineLogNative.GetContext().GetProcessProfile().splitLines);
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventsWithPartLog() {
    // make config
    Json::Value config;
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "flag";
    // make processor
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events": [
            {
                "contents" :
                {
                    "P": "true",
                    "content": "Exception"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": " in thread"
                },
                "timestamp" : 12345678902,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": "  'main'"
                },
                "timestamp" : 12345678903,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": " java.lang.NullPoinntterException"
                },
                "timestamp" : 12345678904,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": "Exception"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": " in thread"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": "  'main'"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": " java.lang.NullPoinntterException"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": "Exception"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": " in thread"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "P": "true",
                    "content": "  'main'"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": " java.lang.NullPoinntterException"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": "     at com.example.myproject.Book.getTitle"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": "     at com.example.myproject.Book.getTitle"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": "     at com.example.myproject.Book.getTitle"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content": "    ...23 more"
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
    processorMergeMultilineLogNative.Process(eventGroup);
    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events": [
            {
                "contents":{
                    "content":"Exception in thread  'main' java.lang.NullPoinntterException"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"Exception in thread  'main' java.lang.NullPoinntterException"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"Exception in thread  'main' java.lang.NullPoinntterException"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"     at com.example.myproject.Book.getTitle"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"     at com.example.myproject.Book.getTitle"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"     at com.example.myproject.Book.getTitle"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            },
            {
                "contents":{
                    "content":"    ...23 more"
                },
                "timestamp":12345678901,
                "timestampNanosecond":0,
                "type":1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());

    {
        // make config
        Json::Value config;
        config["StartPattern"] = LOG_BEGIN_REGEX;
        config["ContinuePattern"] = LOG_CONTINUE_REGEX;
        config["EndPattern"] = LOG_END_REGEX;
        config["MergeType"] = "regex";
        config["UnmatchedContentTreatment"] = "discard";
        config["AppendingLogPositionMeta"] = false;
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
        // run test function
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events": [
                {
                    "contents":{
                        "content":"Exception in thread  'main' java.lang.NullPoinntterException\nException in thread  'main' java.lang.NullPoinntterException\nException in thread  'main' java.lang.NullPoinntterException\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n    ...23 more"
                    },
                    "timestamp":12345678901,
                    "timestampNanosecond":0,
                    "type":1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }

}

class ProcessorMergeMultilineLogDisacardUnmatchUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithEnd();
    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorMergeMultilineLogDisacardUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(ProcessorMergeMultilineLogDisacardUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(ProcessorMergeMultilineLogDisacardUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(ProcessorMergeMultilineLogDisacardUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(ProcessorMergeMultilineLogDisacardUnmatchUnittest, TestLogSplitWithEnd);

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithBeginContinue() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)"
               << LOG_CONTINUE_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log (only begin)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithBeginEnd() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"(\n)" << LOG_END_STRING
               << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: incomplete log (begin)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithBegin() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithContinueEnd() {
    // make config
    Json::Value config;
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "regex";
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)"
               << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log (only end)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithEnd() {
    // make config
    Json::Value config;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "regex";
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    }
}

class ProcessorMergeMultilineLogKeepUnmatchUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }
    void TestLogSplitWithBeginContinue();
    void TestLogSplitWithBeginEnd();
    void TestLogSplitWithBegin();
    void TestLogSplitWithContinueEnd();
    void TestLogSplitWithEnd();
    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorMergeMultilineLogKeepUnmatchUnittest, TestLogSplitWithBeginContinue);
UNIT_TEST_CASE(ProcessorMergeMultilineLogKeepUnmatchUnittest, TestLogSplitWithBeginEnd);
UNIT_TEST_CASE(ProcessorMergeMultilineLogKeepUnmatchUnittest, TestLogSplitWithBegin);
UNIT_TEST_CASE(ProcessorMergeMultilineLogKeepUnmatchUnittest, TestLogSplitWithContinueEnd);
UNIT_TEST_CASE(ProcessorMergeMultilineLogKeepUnmatchUnittest, TestLogSplitWithEnd);

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithBeginContinue() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)"
               << LOG_CONTINUE_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log (only begin)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithBeginEnd() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"(\n)" << LOG_END_STRING
               << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: incomplete log (begin)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithBegin() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "regex";
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_BEGIN_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithContinueEnd() {
    // make config
    Json::Value config;
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "regex";
    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)"
               << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: complete log (only end)
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithEnd() {
    // make config
    Json::Value config;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
    config["AppendingLogPositionMeta"] = false;
    config["MergeType"] = "regex";
    // make processor

    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    { // case: complete log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    { // case: no match log
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::stringstream inJson;
        inJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
               << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson.str());
        std::string logPath("/var/log/message");
        // run test function
        processorSplitLogStringNative.Process(eventGroup);
        processorMergeMultilineLogNative.Process(eventGroup);
        // judge result
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}
} // namespace logtail

UNIT_TEST_MAIN