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
    void TestProcess();
    void TestProcessEventsWithPartLog();
    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcess);
UNIT_TEST_CASE(ProcessorMergeMultilineLogNativeUnittest, TestProcessEventsWithPartLog);

void ProcessorMergeMultilineLogNativeUnittest::TestInit() {
    // 测试合法的正则配置
    {
        // start init通过，IsMultiline为true
        {
            Json::Value config;
            config["StartPattern"] = "123123123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
        // start + continue init通过，IsMultiline为true
        {
            Json::Value config;
            config["StartPattern"] = "123123123.*";
            config["ContinuePattern"] = "123123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
        // continue + end init通过，IsMultiline为true
        {
            Json::Value config;
            config["ContinuePattern"] = "123123.*";
            config["EndPattern"] = "123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
        // end init通过，IsMultiline为true
        {
            Json::Value config;
            config["EndPattern"] = "123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
        // start + end init通过，IsMultiline为true
        {
            Json::Value config;
            config["StartPattern"] = "123123123.*";
            config["EndPattern"] = "123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
    }
    // 测试非法的正则配置
    {
        // start + continue + end init通过，IsMultiline为true
        {
            Json::Value config;
            config["StartPattern"] = "123123123.*";
            config["ContinuePattern"] = "123123.*";
            config["EndPattern"] = "123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_TRUE(processor.mMultiline.IsMultiline());
        }
        // continue init通过，IsMultiline为false
        {
            Json::Value config;
            config["ContinuePattern"] = "123123.*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_FALSE(processor.mMultiline.IsMultiline());
        }
        // 正则格式非法 init通过，IsMultiline为false
        {
            Json::Value config;
            config["StartPattern"] = 1;
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_FALSE(processor.mMultiline.IsMultiline());
        }
        // 正则格式非法 init通过，IsMultiline为false
        {
            Json::Value config;
            config["StartPattern"] = "******";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_FALSE(processor.mMultiline.IsMultiline());
        }
        // 正则格式缺失 init通过，IsMultiline为false
        {
            Json::Value config;
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_FALSE(processor.mMultiline.IsMultiline());
        }
    }
    // 测试mergetype
    {
        // regex init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
            APSARA_TEST_FALSE(processor.mMultiline.IsMultiline());
        }
        // flag init通过
        {
            Json::Value config;
            config["MergeType"] = "flag";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
        // unknown init不通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "unknown";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_FALSE(processor.Init(config));
        }
        // 格式错误 init不通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = 1;
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_FALSE(processor.Init(config));
        }
        // 不存在 init不通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_FALSE(processor.Init(config));
        }
    }
    // 测试UnmatchedContentTreatment
    {
        // single_line init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            config["UnmatchedContentTreatment"] = "single_line";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
        // discard init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            config["UnmatchedContentTreatment"] = "discard";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
        // unknown init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            config["UnmatchedContentTreatment"] = "unknown";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
        // 格式错误 init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            config["UnmatchedContentTreatment"] = 1;
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
        // 不存在 init通过
        {
            Json::Value config;
            config["StartPattern"] = ".*";
            config["MergeType"] = "regex";
            ProcessorMergeMultilineLogNative processor;
            processor.SetContext(mContext);
            APSARA_TEST_TRUE(processor.Init(config));
        }
    }
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
    APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    // check observability
    APSARA_TEST_EQUAL_FATAL(2, processorMergeMultilineLogNative.GetContext().GetProcessProfile().splitLines);
}

void ProcessorMergeMultilineLogNativeUnittest::TestProcessEventsWithPartLog() {
    // case: P
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "Exception"
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
                        "content":"Exception"
                    },
                    "timestamp":12345678901,
                    "timestampNanosecond":0,
                    "type":1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: F
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "content": "Exception"
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
                        "content":"Exception"
                    },
                    "timestamp":12345678901,
                    "timestampNanosecond":0,
                    "type":1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: P+P
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "Except"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "ion"
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
                        "content":"Exception"
                    },
                    "timestamp":12345678901,
                    "timestampNanosecond":0,
                    "type":1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: P+F
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "Except"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "ion"
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
                        "content":"Exception"
                    },
                    "timestamp":12345678901,
                    "timestampNanosecond":0,
                    "type":1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: F+P
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "content": "Except"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "ion"
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
                    "contents" :
                    {
                        "content": "Except"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "ion"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: F+P+F
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "content": "Exc"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "ept"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "ion"
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
                    "contents" :
                    {
                        "content": "Exc"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "eption"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: P+F+P
    {
        // make config
        Json::Value config;
        config["MergeType"] = "flag";
        // make processor
        // ProcessorMergeMultilineLogNative
        ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
        processorMergeMultilineLogNative.SetContext(mContext);
        APSARA_TEST_TRUE(processorMergeMultilineLogNative.Init(config));
        // make eventGroup
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events": [
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "Exc"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "ept"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": "ion"
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
                    "contents" :
                    {
                        "content": "Except"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "content": "ion"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }

    // case part+regex
    {
        // make config
        Json::Value config;
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
                        "P": "",
                        "content": "Exception"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": " in thread"
                    },
                    "timestamp" : 12345678902,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
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
                        "P": "",
                        "content": "Exception"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": " in thread"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
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
                        "P": "",
                        "content": "Exception"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
                        "content": " in thread"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "P": "",
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());

        {
            // make config
            Json::Value config;
            config["StartPattern"] = LOG_BEGIN_REGEX;
            config["EndPattern"] = LOG_END_REGEX;
            config["MergeType"] = "regex";
            config["UnmatchedContentTreatment"] = "discard";

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
            APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
        }
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

    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // case: unmatch + unmatch
    {
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // start + unmatch
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // unmatch + start + continue + continue + unmatch
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch + start + start
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch + start + continue + continue
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithBeginEnd() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";

    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // case: unmatch + unmatch
    {
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: unmatch+start+unmatch
    {
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: unmatch+start+End+unmatch
    {
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
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"(\n)" << LOG_UNMATCH
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
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }

    // case: start+start
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: unmatch+start+End
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch+start+unmatch+End+unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithBegin() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["UnmatchedContentTreatment"] = "discard";

    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // case: unmatch + start
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch
    {
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
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: start + start
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: start + unmatch
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithContinueEnd() {
    // make config
    Json::Value config;
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
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
    // case: unmatch
    {
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: Continue + unmatch
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: Continue + Continue + end
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_END_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: continue
    {
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
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: end
    {
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
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogDisacardUnmatchUnittest::TestLogSplitWithEnd() {
    // make config
    Json::Value config;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "discard";
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
    // case: end
    {
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
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch
    {
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
        APSARA_TEST_STREQ("null", CompactJson(outJson).c_str());
    }
    // case: unmatch + end + unmatch
    {
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
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
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

    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // case: unmatch + unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // start + unmatch
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
                   << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // unmatch + start + continue + continue + unmatch
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch + start + start
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch + start + continue + continue
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithBeginEnd() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["MergeType"] = "regex";
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";

    // make processor
    // ProcessorSplitLogStringNative
    ProcessorSplitLogStringNative processorSplitLogStringNative;
    processorSplitLogStringNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(config));
    // ProcessorMergeMultilineLogNative
    ProcessorMergeMultilineLogNative processorMergeMultilineLogNative;
    processorMergeMultilineLogNative.SetContext(mContext);
    APSARA_TEST_TRUE_FATAL(processorMergeMultilineLogNative.Init(config));
    // case: unmatch + unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch+start+unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch+start+End+unmatch
    {
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
               << LOG_UNMATCH << R"(\n)" << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"(\n)" << LOG_UNMATCH
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
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_END_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }

    // case: start+start
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch+start+End
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch+start+unmatch+End+unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithBegin() {
    // make config
    Json::Value config;
    config["StartPattern"] = LOG_BEGIN_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
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
    // case: unmatch + start
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: start + start
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_BEGIN_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: start + unmatch
    {
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
               << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
                   << LOG_BEGIN_STRING << R"(\n)" << LOG_UNMATCH << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithContinueEnd() {
    // make config
    Json::Value config;
    config["ContinuePattern"] = LOG_CONTINUE_REGEX;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
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
    // case: unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: Continue + unmatch
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
                   << LOG_CONTINUE_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: Continue + Continue + end
    {
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
               << LOG_CONTINUE_STRING << R"(\n)" << LOG_CONTINUE_STRING << R"(\n)" << LOG_END_STRING << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: continue
    {
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
                   << LOG_CONTINUE_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: end
    {
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
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorMergeMultilineLogKeepUnmatchUnittest::TestLogSplitWithEnd() {
    // make config
    Json::Value config;
    config["EndPattern"] = LOG_END_REGEX;
    config["UnmatchedContentTreatment"] = "single_line";
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
    // case: end
    {
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
                   << LOG_END_STRING << R"("
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";
        std::string outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch
    {
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
    // case: unmatch + end + unmatch
    {
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
               << LOG_UNMATCH << R"(\n)" << LOG_END_STRING << R"(\n)" << LOG_UNMATCH << R"("
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
        APSARA_TEST_STREQ(CompactJson(expectJson.str()).c_str(), CompactJson(outJson).c_str());
    }
}
} // namespace logtail

UNIT_TEST_MAIN