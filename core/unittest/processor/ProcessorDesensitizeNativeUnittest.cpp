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
#include "unittest/Unittest.h"
#include "common/JsonUtil.h"
#include "processor/ProcessorDesensitizeNative.h"
#include "models/LogEvent.h"
#include "plugin/instance/ProcessorInstance.h"

using namespace std;

namespace logtail {

class ProcessorDesensitizeNativeUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
    }
    Json::Value GetCastSensWordConfig(string, string, string, string, string, bool);
    void TestInit();
    void TestCastSensWordConst();
    void TestCastSensWordMD5();
    void TestCastSensWordFail();
    void TestCastSensWordLoggroup();
    void TestCastSensWordMulti();
    PipelineContext mContext;
};

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestInit);

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestCastSensWordConst);

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestCastSensWordMD5);

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestCastSensWordFail);

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestCastSensWordLoggroup);

UNIT_TEST_CASE(ProcessorDesensitizeNativeUnittest, TestCastSensWordMulti);

Json::Value ProcessorDesensitizeNativeUnittest::GetCastSensWordConfig(string sourceKey = string("cast1"),
                                                                      string method = "const",
                                                                      string replacingString = string("********"),
                                                                      string contentPatternBeforeReplacedString
                                                                      = "pwd=",
                                                                      string replacedContentPattern = "[^,]+",
                                                                      bool replaceAll = false) {
    Json::Value config;
    config["SourceKey"] = Json::Value(sourceKey);
    config["Method"] = Json::Value(method);
    if (replacingString != "") {
        config["ReplacingString"] = Json::Value(replacingString);
    }
    config["ContentPatternBeforeReplacedString"] = Json::Value(contentPatternBeforeReplacedString);
    config["ReplacedContentPattern"] = Json::Value(replacedContentPattern);
    config["ReplacingAll"] = Json::Value(replaceAll);
    return config;
}

void ProcessorDesensitizeNativeUnittest::TestInit() {
    Json::Value config = GetCastSensWordConfig();
    // run function
    ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
}

void ProcessorDesensitizeNativeUnittest::TestCastSensWordConst() {
    // case1
    {
        Json::Value config = GetCastSensWordConfig();
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,",
                        "log.file.offset": "0"
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
    // case2
    {
        Json::Value config = GetCastSensWordConfig();
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "\r\n\r\nasf@@\n\n@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "\r\n\r\nasf@@\n\n@324 FS2$%pwd,pwd=********,,",
                        "log.file.offset": "0"
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
    // case3
    {
        Json::Value config = GetCastSensWordConfig();
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=12341,df",
                        "log.file.offset": "0"
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
    // case4
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingAll"] = true;
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=********,df",
                        "log.file.offset": "0"
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
    // case5
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingAll"] = true;
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=sdfpsw=543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=********,df",
                        "log.file.offset": "0"
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
}

void ProcessorDesensitizeNativeUnittest::TestCastSensWordMD5() {
    // case1
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=91F6CFCF46787E8A02082B58F7117AFA,,",
                        "log.file.offset": "0"
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
    // case2
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=91F6CFCF46787E8A02082B58F7117AFA,,pwd=12341,df",
                        "log.file.offset": "0"
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
    // case 3
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwdsaf543#$@,,pwd=12341",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwdsaf543#$@,,pwd=F190CE9AC8445D249747CAB7BE43F7D5",
                        "log.file.offset": "0"
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
    // case 4
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=12341",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=F190CE9AC8445D249747CAB7BE43F7D5",
                        "log.file.offset": "0"
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
    // case 5
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=91F6CFCF46787E8A02082B58F7117AFA,,pwd=F190CE9AC8445D249747CAB7BE43F7D5,df",
                        "log.file.offset": "0"
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
    // case 6
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=saf543#$@,,pwd=12341f",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=91F6CFCF46787E8A02082B58F7117AFA,,pwd=2369B00C6DB80BF0794658225730FF0B",
                        "log.file.offset": "0"
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
    // case 7
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=saf543#$@,,pwd=12341f,asfasf",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "pwd=91F6CFCF46787E8A02082B58F7117AFA,,pwd=2369B00C6DB80BF0794658225730FF0B,asfasf",
                        "log.file.offset": "0"
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
    // case 8
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=91F6CFCF46787E8A02082B58F7117AFA,,",
                        "log.file.offset": "0"
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
    // case 9
    {
        Json::Value config = GetCastSensWordConfig();
        config["Method"] = "md5";
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,\npwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,\npwd=91F6CFCF46787E8A02082B58F7117AFA,,",
                        "log.file.offset": "0"
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
}

void ProcessorDesensitizeNativeUnittest::TestCastSensWordFail() {
    // case 1
    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast0" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast0" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
                        "log.file.offset": "0"
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
    // case 2
    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "123214" : "asf@@@324 FS2$%psw,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "123214" : "asf@@@324 FS2$%psw,pwd=saf543#$@,,pwd=12341,df",
                        "log.file.offset": "0"
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
    // case 3
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingString"] = "********";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=12341,df",
                        "log.file.offset": "0"
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
    // case 4
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingString"] = "********";
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=12341,df",
                        "log.file.offset": "0"
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
}

void ProcessorDesensitizeNativeUnittest::TestCastSensWordLoggroup() {
    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "user" : "ali-sls-logtail",
                    "cast1" : "pwd=donottellanyone!,",
                    "id" : "33032119850506123X",
                    "content" : "{'account':'18122100036969','password':'04adf38'};akProxy=null;",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            },
            {
                "contents" :
                {
                    "user" : "ali-sls-logtail1",
                    "cast1" : "pwd=dafddasf@@!123!,",
                    "id" : "33032119891206123X",
                    "log.file.offset": "0"
                },
                "timestampNanosecond" : 0,
                "timestamp" : 12345678901,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);

    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // run function
        processorInstance.Process(eventGroup);
    }
    {
        Json::Value config = GetCastSensWordConfig("id", "const", "********", "\\d{6}", "\\d{8}", true);
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // run function
        processorInstance.Process(eventGroup);
    }
    {
        Json::Value config = GetCastSensWordConfig("content", "const", "********", "'password':'", "[^']*", true);
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // run function
        processorInstance.Process(eventGroup);
    }

    // judge result
    std::string expectJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "cast1" : "pwd=********,",
                    "content" : "{'account':'18122100036969','password':'********'};akProxy=null;",
                    "id" : "330321********123X",
                    "log.file.offset": "0",
                    "user" : "ali-sls-logtail"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "cast1" : "pwd=********,",
                    "id" : "330321********123X",
                    "log.file.offset": "0",
                    "user" : "ali-sls-logtail1"
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

void ProcessorDesensitizeNativeUnittest::TestCastSensWordMulti() {
    // case 1
    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,",
                        "log.file.offset": "0"
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
    // case 2
    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "\r\n\r\nasf@@\n\n@324 FS2$%pwd,pwd=saf543#$@,,",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "\r\n\r\nasf@@\n\n@324 FS2$%pwd,pwd=********,,",
                        "log.file.offset": "0"
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
    // case 3
    {
        Json::Value config = GetCastSensWordConfig();
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=12341,df",
                        "log.file.offset": "0"
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
    // case 4
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=saf543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=********,df",
                        "log.file.offset": "0"
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
    // case 5
    {
        Json::Value config = GetCastSensWordConfig();
        config["ReplacingAll"] = true;
        // init
        ProcessorDesensitizeNative& processor = *(new ProcessorDesensitizeNative);
        std::string pluginId = "testID";
        ProcessorInstance processorInstance(&processor, pluginId);
        APSARA_TEST_TRUE_FATAL(processorInstance.Init(config, mContext));
        // make events
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        std::string inJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=sdfpsw=543#$@,,pwd=12341,df",
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
        processorInstance.Process(eventGroup);
        // judge result
        std::string expectJson = R"({
            "events" :
            [
                {
                    "contents" :
                    {
                        "cast1" : "asf@@@324 FS2$%pwd,pwd=********,,pwd=********,df",
                        "log.file.offset": "0"
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
}
} // namespace logtail

UNIT_TEST_MAIN
