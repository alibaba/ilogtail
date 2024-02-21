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
#include "processor/ProcessorParseContainerLogNative.h"
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

class ProcessorParseContainerLogNativeUnittest : public ::testing::Test {
public:
    void SetUp() override { mContext.SetConfigName("project##config_0"); }

    void TestInit();
    void TestContainerdLog();
    void TestIgnoringStdoutStderr();
    void TestContainerdLogWithSplit();
    void TestDockerJsonLogLineParserWithSplit();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseContainerLogNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseContainerLogNativeUnittest, TestContainerdLog);
UNIT_TEST_CASE(ProcessorParseContainerLogNativeUnittest, TestIgnoringStdoutStderr);
UNIT_TEST_CASE(ProcessorParseContainerLogNativeUnittest, TestContainerdLogWithSplit);
UNIT_TEST_CASE(ProcessorParseContainerLogNativeUnittest, TestDockerJsonLogLineParserWithSplit);

void ProcessorParseContainerLogNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    ProcessorParseContainerLogNative processor;
    processor.SetContext(mContext);

    APSARA_TEST_TRUE_FATAL(processor.Init(config));
}

void ProcessorParseContainerLogNativeUnittest::TestIgnoringStdoutStderr() {
    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = true;
        config["IgnoringStderr"] = true;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        {
            PipelineEventGroup eventGroup(sourceBuffer);
            std::string containerType = "containerd_text";
            eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
            std::string inJson = R"({
                "events" :
                [
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:07.818486411+08:00 stdout P  in thread"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:08.818486411+08:00 stdout P   'main'"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  java.lang.NullPoinntterException"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc1"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc2"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc3"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  abc4"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    }
                ]
            })";
            eventGroup.FromJsonString(inJson);
            // run test function
            processor.Process(eventGroup);
            std::stringstream expectJson;
            expectJson << R"({
                "metadata": {
                    "container.type": "containerd_text"
                }
            })";
            std::string outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
        }
    }

    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = true;
        config["IgnoringStderr"] = false;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        {
            PipelineEventGroup eventGroup(sourceBuffer);
            std::string containerType = "containerd_text";
            eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
            std::string inJson = R"({
                "events" :
                [
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:07.818486411+08:00 stdout P  in thread"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:08.818486411+08:00 stdout P   'main'"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  java.lang.NullPoinntterException"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc1"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc2"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc3"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  abc4"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    }
                ]
            })";
            eventGroup.FromJsonString(inJson);
            // run test function
            processor.Process(eventGroup);
            std::stringstream expectJson;
            expectJson << R"({
                "events": [
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc1"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc2"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc3"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    }
                ],
                "metadata": {
                    "container.type": "containerd_text"
                }
            })";
            std::string outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
        }
    }
    
    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = false;
        config["IgnoringStderr"] = true;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        {
            PipelineEventGroup eventGroup(sourceBuffer);
            std::string containerType = "containerd_text";
            eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
            std::string inJson = R"({
                "events" :
                [
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:07.818486411+08:00 stdout P  in thread"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:08.818486411+08:00 stdout P   'main'"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  java.lang.NullPoinntterException"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc1"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc2"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc3"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  abc4"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    }
                ]
            })";
            eventGroup.FromJsonString(inJson);
            // run test function
            processor.Process(eventGroup);
            std::stringstream expectJson;
            expectJson << R"({
                "events": [
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:06.818486411+08:00",
                            "content": "Exception"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:07.818486411+08:00",
                            "content": " in thread"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:08.818486411+08:00",
                            "content": "  'main'"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " java.lang.NullPoinntterException"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc4"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    }
                ],
                "metadata": {
                    "container.type": "containerd_text"
                }
            })";
            std::string outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
        }
    }

    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = false;
        config["IgnoringStderr"] = false;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        {
            PipelineEventGroup eventGroup(sourceBuffer);
            std::string containerType = "containerd_text";
            eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
            std::string inJson = R"({
                "events" :
                [
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:07.818486411+08:00 stdout P  in thread"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:08.818486411+08:00 stdout P   'main'"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  java.lang.NullPoinntterException"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc1"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc2"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stderr F  abc3"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    },
                    {
                        "contents" :
                        {
                            "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  abc4"
                        },
                        "timestamp" : 12345678901,
                        "timestampNanosecond" : 0,
                        "type" : 1
                    }
                ]
            })";
            eventGroup.FromJsonString(inJson);
            // run test function
            processor.Process(eventGroup);
            std::stringstream expectJson;
            expectJson << R"({
                "events": [
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:06.818486411+08:00",
                            "content": "Exception"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:07.818486411+08:00",
                            "content": " in thread"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "PartLogFlag": "P",
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:08.818486411+08:00",
                            "content": "  'main'"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " java.lang.NullPoinntterException"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc1"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc2"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stderr",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc3"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    },
                    {
                        "contents": {
                            "_source_": "stdout",
                            "_time_": "2024-01-05T23:28:09.818486411+08:00",
                            "content": " abc4"
                        },
                        "timestamp": 12345678901,
                        "timestampNanosecond": 0,
                        "type": 1
                    }
                ],
                "metadata": {
                    "container.type": "containerd_text"
                }
            })";
            std::string outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
        }
    }
}

void ProcessorParseContainerLogNativeUnittest::TestContainerdLog() {
    // make config
    Json::Value config;
    // make ProcessorParseContainerLogNative
    ProcessorParseContainerLogNative processor;
    processor.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processor.Init(config));
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    {
        // case1: PartLogFlag存在，第三个空格存在但空格后无内容
        // case2: PartLogFlag存在，第三个空格不存在
        // case3: PartLogFlag不存在，第二个空格存在
        // case4: 第二个空格不存在
        // case5: 第一个空格不存在
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string containerType = "containerd_text";
        eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P "
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout "
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00stdout"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson);
        // run test function
        processor.Process(eventGroup);
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "PartLogFlag": "P",
                        "_source_": "stdout",
                        "_time_": "2024-01-05T23:28:06.818486411+08:00",
                        "content": ""
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "2024-01-05T23:28:06.818486411+08:00 stdout P"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "_source_": "stdout",
                        "_time_": "2024-01-05T23:28:06.818486411+08:00",
                        "content": ""
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00stdout"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ],
            "metadata": {
                "container.type": "containerd_text"
            }
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    {
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string containerType = "containerd_text";
        eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:07.818486411+08:00 stdout P  in thread"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:08.818486411+08:00 stdout P   'main'"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content" : "2024-01-05T23:28:09.818486411+08:00 stdout F  java.lang.NullPoinntterException"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson);
        // run test function
        processor.Process(eventGroup);
        std::stringstream expectJson;
        expectJson << R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "PartLogFlag": "P",
                        "_source_": "stdout",
                        "_time_" : "2024-01-05T23:28:06.818486411+08:00",
                        "content" : "Exception"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "PartLogFlag": "P",
                        "_source_": "stdout",
                        "_time_" : "2024-01-05T23:28:07.818486411+08:00",
                        "content" : " in thread"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "PartLogFlag": "P",
                        "_source_": "stdout",
                        "_time_" : "2024-01-05T23:28:08.818486411+08:00",
                        "content" : "  'main'"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "_source_": "stdout",
                        "_time_" : "2024-01-05T23:28:09.818486411+08:00",
                        "content" : " java.lang.NullPoinntterException"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ],
            "metadata": {
                "container.type": "containerd_text"
            }
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorParseContainerLogNativeUnittest::TestContainerdLogWithSplit(){
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    std::string containerType = "containerd_text";
    eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
    inJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "2024-01-05T23:28:06.818486411+08:00 stdout P Exception i\n2024-01-05T23:28:06.818486411+08:00 stdout P n thread  'main' java.lang.Null\n2024-01-05T23:28:06.818486411+08:00 stdout P PoinntterExcept\n2024-01-05T23:28:06.818486411+08:00 stdout F ion\n2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitle\n2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitle\n2024-01-05T23:28:06.818486411+08:00 stdout P      at com.exa\n2024-01-05T23:28:06.818486411+08:00 stdout P mple.myproject.Book.g\n2024-01-05T23:28:06.818486411+08:00 stdout P etTit\n2024-01-05T23:28:06.818486411+08:00 stdout F le\n2024-01-05T23:28:06.818486411+08:00 stdout F     ...23 more\n2024-01-05T23:31:06.818486411+08:00 stdout P Exception i\n2024-01-05T23:28:06.818486411+08:00 stdout P n thread  'main' java.lang.Null\n2024-01-05T23:28:06.818486411+08:00 stdout P PoinntterExcept\n2024-01-05T23:28:06.818486411+08:00 stdout F ion\n2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitle\n2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitle\n2024-01-05T23:28:06.818486411+08:00 stdout P      at com.exa\n2024-01-05T23:28:06.818486411+08:00 stdout P mple.myproject.Book.g\n2024-01-05T23:28:06.818486411+08:00 stdout P etTit\n2024-01-05T23:28:06.818486411+08:00 stdout F le\n2024-01-05T23:28:06.818486411+08:00 stdout F     ...23 more"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());

    // ProcessorSplitLogStringNative
    {
        // make config
        Json::Value config;
        config["AppendingLogPositionMeta"] = true;
        // make ProcessorSplitLogStringNative
        ProcessorSplitLogStringNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorParseContainerLogNative
    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = false;
        config["IgnoringStderr"] = true;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorMergeMultilineLogNative
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        config["UnmatchedContentTreatment"] = "single_line";
        // make ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorMergeMultilineLogNative
    {
        // make config
        Json::Value config;
        config["StartPattern"] = LOG_BEGIN_REGEX;
        config["MergeBehavior"] = "regex";
        config["UnmatchedContentTreatment"] = "single_line";
        // make ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }

    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "_source_": "stdout",
                    "_time_": "2024-01-05T23:28:06.818486411+08:00",
                    "content" : "Exception in thread  'main' java.lang.NullPoinntterException\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n    ...23 more"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": ")"
               << strlen(R"(2024-01-05T23:28:06.818486411+08:00 stdout P Exception in2024-01-05T23:28:06.818486411+08:00 stdout P n thread  'main' java.lang.Nulln2024-01-05T23:28:06.818486411+08:00 stdout P PoinntterExceptn2024-01-05T23:28:06.818486411+08:00 stdout P ionn2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitlen2024-01-05T23:28:06.818486411+08:00 stdout F      at com.example.myproject.Book.getTitlen2024-01-05T23:28:06.818486411+08:00 stdout P      at com.exan2024-01-05T23:28:06.818486411+08:00 stdout P mple.myproject.Book.gn2024-01-05T23:28:06.818486411+08:00 stdout P etTitn2024-01-05T23:28:06.818486411+08:00 stdout F len2024-01-05T23:28:06.818486411+08:00 stdout F     ...23 moren)") << R"(",
                    "_source_": "stdout",
                    "_time_": "2024-01-05T23:31:06.818486411+08:00",
                    "content" : "Exception in thread  'main' java.lang.NullPoinntterException\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n    ...23 more"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "metadata":{
            "container.type":"containerd_text"
        }
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseContainerLogNativeUnittest::TestDockerJsonLogLineParserWithSplit() {
    // make eventGroup
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::stringstream inJson;
    std::string containerType = "docker_json-file";
    eventGroup.SetMetadata(EventGroupMetaKey::FILE_ENCODING, containerType);
    inJson << R"({
        "events": [
            {
                "contents": {
                    "content": "{\"log\":\"Exception in thread  \\\"main\\\" java.lang.NullPoinntterException\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:49:37.793533014Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:49:37.793559367Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:49:37.793563414Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:49:37.793566551Z\"}\n{\"log\":\"    ...23 more\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:49:37.793569514Z\"}\n{\"log\":\"Exception in thread  \\\"main\\\" java.lang.NullPoinntterException\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:55:17.514807564Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:55:17.514841003Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:55:17.514853553Z\"}\n{\"log\":\"     at com.example.myproject.Book.getTitle\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:55:17.514856538Z\"}\n{\"log\":\"    ...23 more\\n\",\"stream\":\"stdout\",\"time\":\"2024-02-19T03:55:17.514858843Z\"}"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson.str());

    // ProcessorSplitLogStringNative
    {
        // make config
        Json::Value config;
        config["AppendingLogPositionMeta"] = true;
        // make ProcessorSplitLogStringNative
        ProcessorSplitLogStringNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorParseContainerLogNative
    {
        // make config
        Json::Value config;
        config["IgnoringStdout"] = false;
        config["IgnoringStderr"] = true;
        // make ProcessorParseContainerLogNative
        ProcessorParseContainerLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorMergeMultilineLogNative
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        config["UnmatchedContentTreatment"] = "single_line";
        // make ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }
    // ProcessorMergeMultilineLogNative
    {
        // make config
        Json::Value config;
        config["StartPattern"] = LOG_BEGIN_REGEX;
        config["MergeBehavior"] = "regex";
        config["UnmatchedContentTreatment"] = "single_line";
        // make ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processor;
        processor.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processor.Init(config));
        // run test function
        processor.Process(eventGroup);
    }

    // judge result
    std::stringstream expectJson;
    expectJson << R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "_source_": "stdout",
                    "_time_": "2024-02-19T03:49:37.793533014Z",
                    "content" : "Exception in thread  \"main\" java.lang.NullPoinntterException\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n    ...23 more"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": ")"
               << strlen(R"({"log":"Exception in thread  \"main\" java.lang.NullPoinntterException\n","stream":"stdout","time":"2024-02-19T03:49:37.793533014Z"}n{"log":"     at com.example.myproject.Book.getTitle\n","stream":"stdout","time":"2024-02-19T03:49:37.793559367Z"}n{"log":"     at com.example.myproject.Book.getTitle\n","stream":"stdout","time":"2024-02-19T03:49:37.793563414Z"}n{"log":"     at com.example.myproject.Book.getTitle\n","stream":"stdout","time":"2024-02-19T03:49:37.793566551Z"}n{"log":"    ...23 more\n","stream":"stdout","time":"2024-02-19T03:49:37.793569514Z"}n)") << R"(",
                    "_source_": "stdout",
                    "_time_": "2024-02-19T03:55:17.514807564Z",
                    "content" : "Exception in thread  \"main\" java.lang.NullPoinntterException\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n     at com.example.myproject.Book.getTitle\n    ...23 more"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "metadata":{
            "container.type":"docker_json-file"
        }
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
}

} // namespace logtail

UNIT_TEST_MAIN