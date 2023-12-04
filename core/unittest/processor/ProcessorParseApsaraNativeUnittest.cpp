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
#include "processor/ProcessorParseApsaraNative.h"
#include "unittest/Unittest.h"

namespace logtail {

class ProcessorParseApsaraNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        BOOL_FLAG(ilogtail_discard_old_data) = false;
    }

    void TestInit();
    void TestProcessWholeLine();
    void TestProcessWholeLinePart();
    void TestProcessKeyOverwritten();
    void TestUploadRawLog();
    void TestAddLog();
    void TestProcessEventKeepUnmatch();
    void TestProcessEventDiscardUnmatch();

    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestInit);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessWholeLine);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessWholeLinePart);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessKeyOverwritten);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestUploadRawLog);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestAddLog);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessEventKeepUnmatch);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessEventDiscardUnmatch);

void ProcessorParseApsaraNativeUnittest::TestInit() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = false;
    config["CopingRawLog"] = false;
    config["RenamedSourceKey"] = "rawLog";
    config["Timezone"] = "";

    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
}

void ProcessorParseApsaraNativeUnittest::TestProcessWholeLine() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = false;
    config["CopingRawLog"] = false;
    config["RenamedSourceKey"] = "rawLog";
    config["Timezone"] = "";
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
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:04.862181]	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[1693833364862181]	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "__file_offset__": "0",
                    "microtime": "1693833304862181"
                },
                "timestamp" : 1693833304,
                "timestampNanosecond": 862181000,
                "type" : 1
            },
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "__file_offset__": "0",
                    "microtime": "1693833364862181"
                },
                "timestamp" : 1693833364,
                "timestampNanosecond": 862181000,
                "type" : 1
            },
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "__file_offset__": "0",
                    "microtime": "1693833364862181"
                },
                "timestamp" : 1693833364,
                "timestampNanosecond": 862181000,
                "type" : 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestProcessWholeLinePart() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = false;
    config["KeepingSourceWhenParseSucceed"] = false;
    config["CopingRawLog"] = false;
    config["RenamedSourceKey"] = "rawLog";
    config["Timezone"] = "";
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15:0]	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:0[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[1234560	[info]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    // check observablity
    int count = 3;
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(count, processorInstance.mProcInRecordsTotal->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(0, processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcParseOutSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcParseErrorTotal->GetValue());
}

void ProcessorParseApsaraNativeUnittest::TestProcessKeyOverwritten() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = true;
    config["CopingRawLog"] = true;
    config["RenamedSourceKey"] = "rawLog";
    config["Timezone"] = "";
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15:04.862181]	[info]	[385658]	content:100		rawLog:success		__raw_log__:success",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__THREAD__": "385658",
                    "__file_offset__": "0",
                    "__raw_log__": "success",
                    "content": "100",
                    "microtime": "1693833304862181",
                    "rawLog": "success"
                },
                "timestamp" : 1693833304,
                "timestampNanosecond": 862181000,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "__raw_log__": "value1",
                    "rawLog": "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestUploadRawLog() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = true;
    config["CopingRawLog"] = true;
    config["RenamedSourceKey"] = "rawLog";
    config["Timezone"] = "";
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
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "__file_offset__": "0",
                    "microtime": "1693833304862181",
                    "rawLog" : "[2023-09-04 13:15:04.862181]\t[info]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success"
                },
                "timestamp" : 1693833304,
                "timestampNanosecond": 862181000,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "__raw_log__": "value1",
                    "rawLog": "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestAddLog() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = true;
    config["KeepingSourceWhenParseSucceed"] = false;
    config["CopingRawLog"] = false;
    config["RenamedSourceKey"] = "rawLog";

    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));

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
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "rawLog" : "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "rawLog" : "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "rawLog" : "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "rawLog" : "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__file_offset__": "0",
                    "rawLog" : "value1"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    // check observablity
    int count = 5;
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(count, processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processorInstance.mProcOutRecordsTotal->GetValue());
    expectValue = "rawLogvalue1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseOutSizeBytes->GetValue());

    APSARA_TEST_EQUAL_FATAL(0, processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(count, processor.mProcParseErrorTotal->GetValue());
}

void ProcessorParseApsaraNativeUnittest::TestProcessEventDiscardUnmatch() {
    // make config
    Json::Value config;
    config["SourceKey"] = "content";
    config["KeepingSourceWhenParseFail"] = false;
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
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "__file_offset__": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    // check observablity
    int count = 5;
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(count, processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(0, processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(0, processor.mProcParseOutSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(count, processor.mProcParseErrorTotal->GetValue());
}

} // namespace logtail

UNIT_TEST_MAIN