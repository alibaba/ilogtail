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

#include "common/JsonUtil.h"
#include "config/Config.h"
#include "models/LogEvent.h"
#include "plugin/instance/ProcessorInstance.h"
#include "processor/ProcessorParseJsonNative.h"
#include "processor/ProcessorSplitLogStringNative.h"
#include "processor/ProcessorSplitRegexNative.h"
#include "unittest/Unittest.h"

namespace logtail {

class ProcessorParseJsonNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }

    void TestInit();
    void TestProcessJson();
    void TestProcessJsonEscapedNullByte();
    void TestAddLog();
    void TestProcessEventKeepUnmatch();
    void TestProcessEventDiscardUnmatch();
    void TestProcessJsonContent();
    void TestProcessJsonRaw();
    void TestMultipleLines();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestInit);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestAddLog);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessJson);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessJsonEscapedNullByte);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessEventKeepUnmatch);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessEventDiscardUnmatch);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessJsonContent);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestProcessJsonRaw);

UNIT_TEST_CASE(ProcessorParseJsonNativeUnittest, TestMultipleLines);


PluginInstance::PluginMeta getPluginMeta(){
    PluginInstance::PluginMeta pluginMeta{"testID", "testChildID"};
    return pluginMeta;
}


void ProcessorParseJsonNativeUnittest::TestMultipleLines() {
    // error json
    {
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}\u0000{\"name\":\"Mike\",\"age\":25,\"is_student\":asdfsadf,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                        "log.file.offset": "0"
                    },
                    "timestampNanosecond" : 0,
                    "timestamp" : 12345678901,
                    "type" : 1
                }
            ]
        })";

        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "__raw__" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}",
                        "log.file.offset": "0",
                        "time" : "07/Jul/2022:10:30:28",
                        "url" : "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "__raw__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":asdfsadf,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                        "__raw_log__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":asdfsadf,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                        "log.file.offset":"0"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";

        // ProcessorSplitLogStringNative
        {
            // make events
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            eventGroup.FromJsonString(inJson);

            // make config
            Config config;
            config.mDiscardUnmatch = false;
            config.mUploadRawLog = true;
            config.mAdvancedConfig.mRawLogTag = "__raw__";
            config.mLogType = JSON_LOG;

            std::string pluginId = "testID";
            ComponentConfig componentConfig(pluginId, config);

            // run function ProcessorSplitLogStringNative
            ProcessorSplitLogStringNative processorSplitLogStringNative;
            processorSplitLogStringNative.SetContext(mContext);
            APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(componentConfig));
            processorSplitLogStringNative.Process(eventGroup);
            std::string outJson = eventGroup.ToJsonString();
            // run function ProcessorParseJsonNative
            ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
            ProcessorInstance processorInstance(&processor, getPluginMeta());
            APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
            processor.Process(eventGroup);

            // judge result
            outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
        }
    }
    {
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}\u0000{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                        "log.file.offset": "0"
                    },
                    "timestampNanosecond" : 0,
                    "timestamp" : 12345678901,
                    "type" : 1
                }
            ]
        })";

        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "__raw__" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}",
                        "log.file.offset": "0",
                        "time" : "07/Jul/2022:10:30:28",
                        "url" : "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                },
                {
                    "contents" :
                    {
                        "__raw__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                        "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                        "age":"25",
                        "courses":"[\"Math\",\"English\",\"Science\"]",
                        "is_student":"false",
                        "log.file.offset":"0",
                        "name":"Mike",
                        "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
                    },
                    "timestamp" : 12345678901,
                    "timestampNanosecond" : 0,
                    "type" : 1
                }
            ]
        })";

        // ProcessorSplitLogStringNative
        {
            // make events
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            eventGroup.FromJsonString(inJson);

            // make config
            Config config;
            config.mDiscardUnmatch = false;
            config.mUploadRawLog = true;
            config.mAdvancedConfig.mRawLogTag = "__raw__";
            config.mLogType = JSON_LOG;

            std::string pluginId = "testID";
            ComponentConfig componentConfig(pluginId, config);

            // run function ProcessorSplitLogStringNative
            ProcessorSplitLogStringNative processorSplitLogStringNative;
            processorSplitLogStringNative.SetContext(mContext);
            APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(componentConfig));
            processorSplitLogStringNative.Process(eventGroup);
            std::string outJson = eventGroup.ToJsonString();
            // run function ProcessorParseJsonNative
            ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
            ProcessorInstance processorInstance(&processor, getPluginMeta());
            APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
            processor.Process(eventGroup);

            // judge result
            outJson = eventGroup.ToJsonString();
            APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
        }
    }
}

void ProcessorParseJsonNativeUnittest::TestInit() {
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";

    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
}

void ProcessorParseJsonNativeUnittest::TestAddLog() {
    Config config;
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));

    auto sourceBuffer = std::make_shared<SourceBuffer>();
    auto logEvent = LogEvent::CreateEvent(sourceBuffer);
    char key[] = "key";
    char value[] = "value";
    processor.AddLog(key, value, *logEvent);
    // check observability
    APSARA_TEST_EQUAL_FATAL(int(strlen(key) + strlen(value) + 5), processor.GetContext().GetProcessProfile().logGroupSize);
}

void ProcessorParseJsonNativeUnittest::TestProcessJsonEscapedNullByte() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = false;
    config["CopingRawLog"] = false;
    config["RenamedSourceKey"] = "rawLog";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"level\":\"ERROR\",\"time\":\"2024-07-04T06:59:23.078Z\",\"msg\":\"expect { or n, but found \\u0000, error found in #0 byte of ...\"}"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    std::vector<PipelineEventGroup> eventGroupList;
    eventGroupList.emplace_back(std::move(eventGroup));
    processorInstance.Process(eventGroupList);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "level": "ERROR",
	                "msg": "expect { or n, but found \u0000, error found in #0 byte of ...",
	                "time": "2024-07-04T06:59:23.078Z"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroupList[0].ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    APSARA_TEST_GT_FATAL(processorInstance.mProcTimeMS->GetValue(), uint64_t(0));
}

void ProcessorParseJsonNativeUnittest::TestProcessJson() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
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
                    "content" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__raw__" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"}",
                    "log.file.offset": "0",
                    "time" : "07/Jul/2022:10:30:28",
                    "url" : "POST /PutData?Category=YunOsAccountOpLog HTTP/1.1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                    "age":"25",
                    "courses":"[\"Math\",\"English\",\"Science\"]",
                    "is_student":"false",
                    "log.file.offset":"0",
                    "name":"Mike",
                    "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
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

void ProcessorParseJsonNativeUnittest::TestProcessJsonContent() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
    config.mAdvancedConfig.mRawLogTag = "__raw__";

    // make events
    // the first event has key "content" in json key with overwrites sourceKey "content"
    // the second event doesn't have key "content" in json
    // after parsing, the first event's content should be the value in json, the original content should be the value of
    // "__raw__" the second event's content should be dropped, the original content should be the value of "__raw__"
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"content\":\"content_test\",\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678902,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__raw__" : "{\"content\":\"content_test\",\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                    "age":"25",
                    "content":"content_test",
                    "courses":"[\"Math\",\"English\",\"Science\"]",
                    "is_student":"false",
                    "log.file.offset":"0",
                    "name":"Mike",
                    "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                    "age":"25",
                    "courses":"[\"Math\",\"English\",\"Science\"]",
                    "is_student":"false",
                    "name":"Mike",
                    "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
                },
                "timestamp" : 12345678902,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseJsonNativeUnittest::TestProcessJsonRaw() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
    config.mAdvancedConfig.mRawLogTag = "__raw__";

    // make events
    // the first event has key "__raw__" in json key with overwrites rawLogTag "__raw__"
    // the second event doesn't have key "__raw__" in json
    // after parsing, the first event's __raw__ should be the value in json, the original content should be discarded
    // the second event's __raw__ should be the original content
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"__raw__\":\"content_test\",\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678902,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__raw__" : "content_test",
                    "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                    "age":"25",
                    "courses":"[\"Math\",\"English\",\"Science\"]",
                    "is_student":"false",
                    "log.file.offset":"0",
                    "name":"Mike",
                    "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw__" : "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}",
                    "address" : "{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"}",
                    "age":"25",
                    "courses":"[\"Math\",\"English\",\"Science\"]",
                    "is_student":"false",
                    "log.file.offset":"0",
                    "name":"Mike",
                    "scores":"{\"Math\":90,\"English\":85,\"Science\":95}"
                },
                "timestamp" : 12345678902,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseJsonNativeUnittest::TestProcessEventKeepUnmatch() {
    // make config
    Config config;
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
                    "content" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);

    int count = 1;

    // check observablity
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);

    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue
        = "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL((std::string("__raw_log__").size() - std::string("content").size()) * count,
                            processor.mProcParseOutSizeBytes->GetValue());

    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcParseErrorTotal->GetValue());

    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__raw_log__" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"",
                    "log.file.offset":"0"
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

void ProcessorParseJsonNativeUnittest::TestProcessEventDiscardUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = true;
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
                    "content" : "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, getPluginMeta());
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);

    int count = 1;

    // check observablity
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);

    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue
        = "{\"url\": \"POST /PutData?Category=YunOsAccountOpLog HTTP/1.1\",\"time\": \"07/Jul/2022:10:30:28\"";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processor.mProcParseOutSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcParseErrorTotal->GetValue());

    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
}

} // namespace logtail

UNIT_TEST_MAIN
