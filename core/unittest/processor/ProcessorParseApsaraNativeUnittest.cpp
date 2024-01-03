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
#include "processor/ProcessorSplitLogStringNative.h"
#include "processor/ProcessorSplitRegexNative.h"
#include "unittest/Unittest.h"

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
    void TestProcessWholeLinePart();
    void TestProcessKeyOverwritten();
    void TestUploadRawLog();
    void TestAddLog();
    void TestProcessEventKeepUnmatch();
    void TestProcessEventDiscardUnmatch();
    void TestMultipleLines();
    void TestProcessEventMicrosecondUnmatch();
    void TestApsaraEasyReadLogTimeParser();
    void TestApsaraLogLineParser();

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
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestMultipleLines);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestProcessEventMicrosecondUnmatch);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestApsaraEasyReadLogTimeParser);
UNIT_TEST_CASE(ProcessorParseApsaraNativeUnittest, TestApsaraLogLineParser);

void ProcessorParseApsaraNativeUnittest::TestMultipleLines() {
    // 第一个contents 测试多行下的解析，第二个contents测试多行下time的解析
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15:50.1]\t[ERROR]\t[1]\t/ilogtail/AppConfigBase.cpp:1\t\tAppConfigBase AppConfigBase:1
[2023-09-04 13:15:33.2]\t[INFO]\t[2]\t/ilogtail/AppConfigBase.cpp:2\t\tAppConfigBase AppConfigBase:2
[2023-09-04 13:15:22.3]\t[WARNING]\t[3]\t/ilogtail/AppConfigBase.cpp:3\t\tAppConfigBase AppConfigBase:3",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15
:50.1]\t[ERROR]\t[1]\t/ilogtail/AppConfigBase.cpp:1\t\tAppConfigBase AppConfigBase:1
[2023-09-04 13:15:22.3]\t[WARNING]\t[3]\t/ilogtail/AppConfigBase.cpp:3\t\tAppConfigBase AppConfigBase:3",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";


    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "1",
                    "AppConfigBase AppConfigBase": "1",
                    "__LEVEL__": "ERROR",
                    "__THREAD__": "1",
                    "log.file.offset": "0",
                    "microtime": "1693833350100000"
                },
                "timestamp": 1693833350,
                "timestampNanosecond": 100000000,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "2",
                    "AppConfigBase AppConfigBase": "2",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "2",
                    "log.file.offset": "0",
                    "microtime": "1693833333200000"
                },
                "timestamp": 1693833333,
                "timestampNanosecond": 200000000,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "3",
                    "AppConfigBase AppConfigBase": "3",
                    "__LEVEL__": "WARNING",
                    "__THREAD__": "3",
                    "log.file.offset": "0",
                    "microtime": "1693833322300000"
                },
                "timestamp": 1693833322,
                "timestampNanosecond": 300000000,
                "type": 1
            },
            {
                "contents": {
                    "__raw_log__": "[2023-09-04 13:15",
                    "log.file.offset": "0"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            },
            {
                "contents": {
                    "__raw_log__": ":50.1]\t[ERROR]\t[1]\t/ilogtail/AppConfigBase.cpp:1\t\tAppConfigBase AppConfigBase:1",
                    "log.file.offset": "0"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "3",
                    "AppConfigBase AppConfigBase": "3",
                    "__LEVEL__": "WARNING",
                    "__THREAD__": "3",
                    "log.file.offset": "0",
                    "microtime": "1693833322300000"
                },
                "timestamp": 1693833322,
                "timestampNanosecond": 300000000,
                "type": 1
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
        config.mUploadRawLog = false;
        config.mAdvancedConfig.mRawLogTag = "__raw__";
        config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();

        std::string pluginId = "testID";
        ComponentConfig componentConfig(pluginId, config);

        // run function ProcessorSplitLogStringNative
        ProcessorSplitLogStringNative processorSplitLogStringNative;
        processorSplitLogStringNative.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processorSplitLogStringNative.Init(componentConfig));
        processorSplitLogStringNative.Process(eventGroup);
        std::string outJson = eventGroup.ToJsonString();
        // run function ProcessorParseApsaraNative
        ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
        processor.Process(eventGroup);

        // judge result
        outJson = eventGroup.ToJsonString();
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    }
    // ProcessorSplitRegexNative
    {
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        eventGroup.FromJsonString(inJson);

        // make config
        Config config;
        config.mDiscardUnmatch = false;
        config.mUploadRawLog = false;
        config.mAdvancedConfig.mRawLogTag = "__raw__";
        config.mLogTimeZoneOffsetSecond = -GetLocalTimeZoneOffsetSecond();
        config.mLogBeginReg = ".*";

        std::string pluginId = "testID";
        ComponentConfig componentConfig(pluginId, config);
        // run function processorSplitRegexNative
        ProcessorSplitRegexNative processorSplitRegexNative;
        processorSplitRegexNative.SetContext(mContext);
        APSARA_TEST_TRUE_FATAL(processorSplitRegexNative.Init(componentConfig));
        processorSplitRegexNative.Process(eventGroup);
        // run function ProcessorParseApsaraNative
        ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
        processor.Process(eventGroup);
        // judge result
        std::string outJson = eventGroup.ToJsonString();
        std::string outJson2 = CompactJson(outJson);
        
        APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    }
}

void ProcessorParseApsaraNativeUnittest::TestInit() {
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mLogTimeZoneOffsetSecond = 0;

    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
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
                    "content" : "[2023-09-04 13:15:04.862181]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:04.862181]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[1693833364862181]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
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
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "log.file.offset": "0",
                    "microtime": "1693833304862181"
                },
                "timestamp": 1693833304,
                "timestampNanosecond": 862181000,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "log.file.offset": "0",
                    "microtime": "1693833364862181"
                },
                "timestamp": 1693833364,
                "timestampNanosecond": 862181000,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "log.file.offset": "0",
                    "microtime": "1693833364862181"
                },
                "timestamp": 1693833364,
                "timestampNanosecond": 862181000,
                "type": 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestProcessWholeLinePart() {
    // make config
    Config config;
    config.mDiscardUnmatch = true;
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
                    "content" : "[2023-09-04 13:15:0]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:0[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[1234560\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
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
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "log.file.offset":"0",
                    "microtime": "1693833300000000"
                },
                "timestamp": 1693833300,
                "timestampNanosecond": 0,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__THREAD__": "385658",
                    "log.file.offset":"0",
                    "microtime": "1693833360000000"
                },
                "timestamp": 1693833360,
                "timestampNanosecond": 0,
                "type": 1
            }
        ]
    })";
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    // check observablity
    APSARA_TEST_EQUAL_FATAL(1, processor.GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(uint64_t(3), processorInstance.mProcInRecordsTotal->GetValue());
    // only timestamp failed, so output is 2
    APSARA_TEST_EQUAL_FATAL(uint64_t(2), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(1), processor.mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(1), processor.mProcParseErrorTotal->GetValue());
}

void ProcessorParseApsaraNativeUnittest::TestProcessKeyOverwritten() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
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
                    "content" : "[2023-09-04 13:15:04.862181]\t[INFO]\t[385658]\tcontent:100\t\t__raw__:success\t\t__raw_log__:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
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
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "__raw__": "success",
                    "__raw_log__": "success",
                    "content": "100",
                    "log.file.offset": "0",
                    "microtime": "1693833304862181"
                },
                "timestamp": 1693833304,
                "timestampNanosecond": 862181000,
                "type": 1
            },
            {
                "contents": {
                    "__raw__": "value1",
                    "__raw_log__": "value1",
                    "log.file.offset": "0"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestUploadRawLog() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
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
                    "content" : "[2023-09-04 13:15:04.862181]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
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
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "__raw__": "[2023-09-04 13:15:04.862181]\t[INFO]\t[385658]\t/ilogtail/AppConfigBase.cpp:100\t\tAppConfigBase AppConfigBase:success",
                    "log.file.offset": "0",
                    "microtime": "1693833304862181"
                },
                "timestamp": 1693833304,
                "timestampNanosecond": 862181000,
                "type": 1
            },
            {
                "contents": {
                    "__raw__": "value1",
                    "__raw_log__": "value1",
                    "log.file.offset": "0"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            }
        ]
    })";
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
}

void ProcessorParseApsaraNativeUnittest::TestAddLog() {
    Config config;
    ProcessorParseApsaraNative& processor = *(new ProcessorParseApsaraNative);
    processor.SetContext(mContext);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
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

void ProcessorParseApsaraNativeUnittest::TestProcessEventKeepUnmatch() {
    // make config
    Config config;
    config.mSeparator = ",";
    config.mQuote = '\'';
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
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
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
                    "__raw_log__" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw_log__" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw_log__" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw_log__" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond": 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "__raw_log__" : "value1",
                    "log.file.offset": "0"
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
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL((std::string("__raw_log__").size() - std::string("content").size()) * count,
                            processor.mProcParseOutSizeBytes->GetValue());

    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processor.mProcDiscardRecordsTotal->GetValue());

    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcParseErrorTotal->GetValue());
}

void ProcessorParseApsaraNativeUnittest::TestProcessEventDiscardUnmatch() {
    // make config
    Config config;
    config.mSeparator = ",";
    config.mQuote = '\'';
    config.mColumnKeys = {"time", "method", "url", "request_time"};
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
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value1",
                    "log.file.offset": "0"
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
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    // judge result
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL("null", CompactJson(outJson).c_str());
    // check observablity
    int count = 5;
    APSARA_TEST_EQUAL_FATAL(count, processor.GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processorInstance.mProcInRecordsTotal->GetValue());
    std::string expectValue = "value1";
    APSARA_TEST_EQUAL_FATAL((expectValue.length()) * count, processor.mProcParseInSizeBytes->GetValue());
    // discard unmatch, so output is 0
    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processor.mProcParseOutSizeBytes->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(count), processor.mProcParseErrorTotal->GetValue());
}

void ProcessorParseApsaraNativeUnittest::TestProcessEventMicrosecondUnmatch() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:15:04,862181]	[INFO]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:16:04]	[INFO]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:17:04,1]	[INFO]	[385658]	/ilogtail/AppConfigBase.cpp:100		AppConfigBase AppConfigBase:success"
                },
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "[2023-09-04 13:18:04"
                },
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    std::string pluginId = "testID";
    ProcessorParseApsaraNative* processor = new ProcessorParseApsaraNative;
    ProcessorInstance processorInstance(processor, pluginId);
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);

    // judge result
    std::string expectJson = R"({
        "events": [
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "microtime": "1693833304862181"
                },
                "timestamp": 1693833304,
                "timestampNanosecond": 862181000,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "microtime": "1693833364000000"
                },
                "timestamp": 1693833364,
                "timestampNanosecond": 0,
                "type": 1
            },
            {
                "contents": {
                    "/ilogtail/AppConfigBase.cpp": "100",
                    "AppConfigBase AppConfigBase": "success",
                    "__LEVEL__": "INFO",
                    "__THREAD__": "385658",
                    "microtime": "1693833424100000"
                },
                "timestamp": 1693833424,
                "timestampNanosecond": 100000000,
                "type": 1
            },
            {
                "contents": {
                    "__raw_log__": "[2023-09-04 13:18:04"
                },
                "timestamp": 12345678901,
                "timestampNanosecond": 0,
                "type": 1
            }
        ]
    })";
    std::string outJson = eventGroup.ToJsonString();
    APSARA_TEST_STREQ_FATAL(CompactJson(expectJson).c_str(), CompactJson(outJson).c_str());
    // check observablity
    APSARA_TEST_EQUAL_FATAL(1, processor->GetContext().GetProcessProfile().parseFailures);
    APSARA_TEST_EQUAL_FATAL(uint64_t(4), processorInstance.mProcInRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(4), processorInstance.mProcOutRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(0), processor->mProcDiscardRecordsTotal->GetValue());
    APSARA_TEST_EQUAL_FATAL(uint64_t(1), processor->mProcParseErrorTotal->GetValue());
}

struct LogTimeCase {
    LogTimeCase(const char* buffer, int64_t secTime, int64_t microTime)
        : buffer(buffer), secTime(secTime), microTime(microTime) {}
    std::string buffer;
    int64_t secTime{};
    int64_t microTime{};
};

void ProcessorParseApsaraNativeUnittest::TestApsaraEasyReadLogTimeParser() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mTimeZoneAdjust = true;
    config.mLogTimeZoneOffsetSecond = 8 * 3600;
    config.mAdvancedConfig.mAdjustApsaraMicroTimezone = true;

    StringView cacheStr;
    LogtailTime cacheTime = {0, 0};
    int64_t secTime{};
    int64_t microTime{};
    LogTimeCase cases[] = {
        {"[1378972170425093]\tA:B", 1378972170, 1378972170425093},
        {"[1378972170093]\tA:B", 1378972170, 1378972170093000},
        {"[1378972172]", 1378972172, 1378972172000000},
        {"[2013-09-12 22:18:28.819129]\tA:B", 1378995508, 1378995508819129},
        {"[2013-09-12 22:18:28.819139]\tA:B", 1378995508, 1378995508819139},
        {"[2013-09-12 22:18:29.819139]", 1378995509, 1378995509819139},
        {"[2013-09-12 22:18:29.819]\tA:B", 1378995509, 1378995509819000},
        {"[2013-09-12 22:18:30,666666]\tA:B", 1378995510, 1378995510666666},
        {"[2013-09-12 22:18:30,,]", 1378995510, 1378995510000000},
        {"[2013-09-12 22:18:31]\t", 1378995511, 1378995511000000},
    };

    // run function
    std::string pluginId = "testID";
    ProcessorParseApsaraNative* processor = new ProcessorParseApsaraNative;
    ProcessorInstance processorInstance(processor, pluginId);
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    for (auto& acase : cases) {
        auto buffer = StringView(acase.buffer);
        secTime = processor->ApsaraEasyReadLogTimeParser(buffer, cacheStr, cacheTime, microTime);
        APSARA_TEST_EQUAL_FATAL(acase.secTime, secTime);
        APSARA_TEST_EQUAL_FATAL(acase.microTime, microTime);
    }
}

void ProcessorParseApsaraNativeUnittest::TestApsaraLogLineParser() {
    const char* logLine[] = {
        "[2013-03-13 18:05:09.493309]\t[WARNING]\t[13000]\t[build/debug64/ilogtail/core/ilogtail.cpp:1753]", // 1
        "[2013-03-13 18:05:09.493309]\t[WARNING]\t[13000]\t[build/debug64/ilogtail/core/ilogtail.cpp:1753]\t", // 2
        "[2013-03-13 18:05:09.493309]\t[WARNING]\t[13000]\t[build/debug64/ilogtail/core/ilogtail.cpp:1754]\tsomestring", // 3
        "[2013-03-13 "
        "18:05:09.493309]\t[WARNING]\t[13000]\t[build/debug64/ilogtail/core/ilogtail.cpp:1755]\tRealRecycle#Command:rm "
        "-rf /apsara/tubo/.fuxi_tubo_trash/*", // 4
        "[2013-03-13 "
        "18:14:57.365716]\t[ERROR]\t[12835]\t[build/debug64/ilogtail/core/"
        "ilogtail.cpp:1945]\tParseWhiteListOK:{\n\\\"sys/"
        "pangu/ChunkServerRole\\\": \\\"\\\",\n\\\"sys/pangu/PanguMasterRole\\\": \\\"\\\"}", // 5
        "[2013-03-13 18:14:57.365716]\t[12835]\t[ERROR]\t[build/debug64/ilogtail/core/ilogtail.cpp:1945]", // 6
        "[2013-03-13 18:14:57.365716]\t[build/debug64/ilogtail/core/ilogtail.cpp:1945]\t[12835]\t[ERROR]", // 7
        "[2013-03-13 18:14:57.365716]\t[build/debug64/ilogtail/core/ilogtail.cpp:1945]\t[ERROR]", // 8
        "[2013-03-13 18:14:57.365716]\t[build/debug64/ilogtail/core/ilogtail.cpp:1945]\t[12835]\t[ERROR]\t[5432187]", // 9
        "[2013-03-13 "
        "18:14:57.365716]\t[build/debug64/ilogtail/core/ilogtail.cpp:1945]\t[12835]\t[ERROR]\t[5432187]\tcount:55", // 10
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]", // 11
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\t", // 12
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\n", // 13
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\tother\tcount:45", // 14
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\tother:\tcount:45", // 15
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\tcount:45", // 16
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\tcount:45\tnum:88\tjob:ss", // 17
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\t[corrupt\tcount:45\tnum:88\tjob:ss", // 18
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\t[corruptcount:45\tnum:88\tjob:ss", // 19
        "[2013-03-13 18:14:57.365716]\t[trace_id:787]\t[ERROR]\t[corrupt]count:45\tnum:88\tjob:ss", // 20
        "[2013-03-13 18:14:57.365716]\t[build/debug64]\t[ERROR]\tcount:45\tnum:88\tjob:ss", // 21
        "[2013-03-13 18:14:57.365716]\t[build/debug64:]\t[ERROR]\tcount:45\tnum:88\tjob:ss", // 22
        "[2013-03-13 18:14:57.365716]\t[build/debug64:]\t[ERROR]\tcount:45\t:88\tjob:ss", // 23
        "[2013-03-13 18:14:57.365716]", // 24
        "[2013-03-13 18:14:57.365716]\t", // 25
        "[2013-03-13 18:14:57.365716]\n", // 26
        "[2013-03-13 18:14:57.365716]\t\t\t", // 27
        "", // 28
        "[2013-03-13 "
        "18:05:09.493309]\t[WARNING]\t[13000]\t[13003]\t[ERROR]\t[build/debug64/ilogtail/core/ilogtail.cpp:1753]", // 29
        "[2013-03-13 18:05:09.493309]\t[WARNING]\t[13000]\t[13003]\t[ERROR]\t[tubo.cpp:1753]", // 30
        "[2013-03-13 18:05:09.493309" // 31
    };
    static const char* APSARA_FIELD_LEVEL = "__LEVEL__";
    static const char* APSARA_FIELD_THREAD = "__THREAD__";
    static const char* APSARA_FIELD_FILE = "__FILE__";
    static const char* APSARA_FIELD_LINE = "__LINE__";
    const char* logParseResult[][16] = {
        {"microtime",
         "1363169109493309",
         APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1753",
         NULL}, // 1
        {"microtime",
         "1363169109493309",
         APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1753",
         NULL}, // 2
        {"microtime",
         "1363169109493309",
         APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1754",
         NULL}, // 3
        {APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1755",
         "RealRecycle#Command",
         "rm -rf /apsara/tubo/.fuxi_tubo_trash/*",
         NULL}, // 4
        {APSARA_FIELD_LEVEL,
         "ERROR",
         APSARA_FIELD_THREAD,
         "12835",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         "ParseWhiteListOK",
         "{\n\"sys/pangu/ChunkServerRole\": \"\",\n\"sys/pangu/PanguMasterRole\": \"\"}",
         NULL}, // 5
        {APSARA_FIELD_THREAD,
         "12835",
         APSARA_FIELD_LEVEL,
         "ERROR",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         NULL}, // 6
        {APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         APSARA_FIELD_THREAD,
         "12835",
         APSARA_FIELD_LEVEL,
         "ERROR",
         NULL}, // 7
        {APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         APSARA_FIELD_LEVEL,
         "ERROR",
         NULL}, // 8
        {APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         APSARA_FIELD_THREAD,
         "12835",
         APSARA_FIELD_LEVEL,
         "ERROR",
         NULL}, // 9
        {APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1945",
         APSARA_FIELD_THREAD,
         "12835",
         APSARA_FIELD_LEVEL,
         "ERROR",
         "count",
         "55",
         NULL}, // 10
        {APSARA_FIELD_LEVEL, "ERROR", NULL}, // 11
        {APSARA_FIELD_LEVEL, "ERROR", NULL}, // 12
        {APSARA_FIELD_LEVEL, "ERROR", NULL}, // 13
        {APSARA_FIELD_LEVEL, "ERROR", "count", "45", NULL}, // 14
        {APSARA_FIELD_LEVEL, "ERROR", "other", "", "count", "45", NULL}, // 15
        {APSARA_FIELD_LEVEL, "ERROR", "count", "45", NULL}, // 16
        {APSARA_FIELD_LEVEL, "ERROR", "count", "45", "num", "88", "job", "ss", NULL}, // 17
        {APSARA_FIELD_LEVEL, "ERROR", "count", "45", "num", "88", "job", "ss", NULL}, // 18
        {APSARA_FIELD_LEVEL, "ERROR", "[corruptcount", "45", "num", "88", "job", "ss", NULL}, // 19
        {APSARA_FIELD_LEVEL, "ERROR", "[corrupt]count", "45", "num", "88", "job", "ss", NULL}, // 20
        {APSARA_FIELD_FILE,
         "build/debug64",
         APSARA_FIELD_LEVEL,
         "ERROR",
         "count",
         "45",
         "num",
         "88",
         "job",
         "ss",
         NULL}, // 21
        {APSARA_FIELD_FILE,
         "build/debug64",
         APSARA_FIELD_LINE,
         "",
         APSARA_FIELD_LEVEL,
         "ERROR",
         "count",
         "45",
         "num",
         "88",
         "job",
         "ss",
         NULL}, // 22
        {APSARA_FIELD_FILE,
         "build/debug64",
         APSARA_FIELD_LINE,
         "",
         APSARA_FIELD_LEVEL,
         "ERROR",
         "count",
         "45",
         "",
         "88",
         "job",
         "ss",
         NULL}, // 23
        {"microtime", "1363169697365716", NULL}, // 24
        {"microtime", "1363169697365716", NULL}, // 25
        {"microtime", "1363169697365716", NULL}, // 26
        {"microtime", "1363169697365716", NULL}, // 27
        {"content", "", NULL}, // 28
        {APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "build/debug64/ilogtail/core/ilogtail.cpp",
         APSARA_FIELD_LINE,
         "1753",
         NULL}, // 29
        {APSARA_FIELD_LEVEL,
         "WARNING",
         APSARA_FIELD_THREAD,
         "13000",
         APSARA_FIELD_FILE,
         "tubo.cpp",
         APSARA_FIELD_LINE,
         "1753",
         NULL}, // 30
        {NULL} // 31
    };

    // make config
    Config config;
    config.mDiscardUnmatch = true;
    config.mUploadRawLog = false;
    config.mTimeZoneAdjust = true;
    config.mLogTimeZoneOffsetSecond = 8 * 3600;
    config.mAdvancedConfig.mAdjustApsaraMicroTimezone = true;

    std::string pluginId = "testID";
    ProcessorInstance processorInstance(new ProcessorParseApsaraNative, pluginId);
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));

    for (uint32_t i = 0; i < sizeof(logLine) / sizeof(logLine[0]); i++) {
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "content" : ")"
            + std::string(logLine[i]) +
            R"("
                    },
                    "timestamp" : 12345678901,
                    "type" : 1
                }
            ]
        })";
        eventGroup.FromJsonString(inJson);
        processorInstance.Process(eventGroup);

        Json::Value outJson = eventGroup.ToJson();
        if (logParseResult[i][0] == NULL) {
            APSARA_TEST_EQUAL(eventGroup.ToJsonString(), "null");
            continue;
        }
        for (int j = 0; j < 10 && logParseResult[i][j] != NULL; j++) {
            if (j % 2 == 0) {
                APSARA_TEST_TRUE(outJson.isMember("events"));
                APSARA_TEST_TRUE(outJson["events"].isArray());
                APSARA_TEST_TRUE(outJson["events"][0].isObject());
                APSARA_TEST_TRUE(outJson["events"][0].isMember("contents"));
                APSARA_TEST_TRUE(outJson["events"][0]["contents"].isMember(logParseResult[i][j]));
                APSARA_TEST_EQUAL(outJson["events"][0]["contents"][logParseResult[i][j]],
                                  std::string(logParseResult[i][j + 1]));
            } else {
                continue;
            }
        }
    }
}
} // namespace logtail

UNIT_TEST_MAIN