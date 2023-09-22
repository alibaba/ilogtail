#include "unittest/Unittest.h"

#include "common/JsonUtil.h"
#include "config/Config.h"
#include "sls_spl/ProcessorSPL.h"
#include "models/LogEvent.h"
#include "plugin/ProcessorInstance.h"
#include <iostream>
#include <sstream>

namespace logtail {

static std::atomic_bool running(true);


class SlsSplUnittest : public ::testing::Test {
public:
    void SetUp() override {
        mContext.SetConfigName("project##config_0");
        mContext.SetLogstoreName("logstore");
        mContext.SetProjectName("project");
        mContext.SetRegion("cn-shanghai");
    }
    PipelineContext mContext;
    void TestSimple();
    void TestJsonParse();
    void TestRegexParse();
    void TestRegexKV();
    void TestRegexCSV();

    void TestTag();
    void TestMultiParse();
};

APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestSimple, 0);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestJsonParse, 1);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestRegexParse, 2);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestRegexCSV, 3);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestRegexKV, 4);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestTag, 5);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestMultiParse, 6);



void SlsSplUnittest::TestSimple() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = "* | where content = 'value_3_0'";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "value_3_0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "value_4_0"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL((u_int)1, logGroupList.size());

    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}


void SlsSplUnittest::TestJsonParse() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = "* | parse-json content ";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"a1\":\"bbbb\",\"c\":\"d\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"a1\":\"ccc\",\"c1\":\"d1\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 1);
    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}


void SlsSplUnittest::TestRegexParse() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(* | parse-regexp content, '(\S+)\s+(\w+)' as ip, method)";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "10.0.0.0 GET /index.html 15824 0.043"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "10.0.0.0 GET /index.html 15824 0.043"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 1);
    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}

void SlsSplUnittest::TestRegexCSV() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(* | parse-csv content as x, y, z)";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "a,b,c"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "a,b,c"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 1);
    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}



void SlsSplUnittest::TestRegexKV() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(* | parse-kv -delims='&?' content)";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "k1=v1&k2=v2?k3=v3"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "k1=v1&k2=v2?k3=v3"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 1);
    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}



void SlsSplUnittest::TestTag() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(* | parse-json content | project-rename __tag__:taiye2=a1)";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"a1\":\"bbbb\",\"c\":\"d\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"a1\":\"cccc\",\"c\":\"d\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__:taiye": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 2);
    if (logGroupList.size() > 0) {
        for (auto& logGroup : logGroupList) {
            std::string outJson = logGroup.ToJsonString();
            std::cout << "outJson: " << outJson << std::endl;
        }
    }
    
    return;
}


void SlsSplUnittest::TestMultiParse() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(.let src = * 
| parse-json content;
.let ds1 = $src
| where type = 'kv'
| parse-kv -delims='&?' message;
$ds1;
.let ds2 = $src
| where type = 'csv'
| parse-csv message as x, y, z;
$ds2;
)";

    // make events
    auto sourceBuffer = std::make_shared<SourceBuffer>();
    PipelineEventGroup eventGroup(sourceBuffer);
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "{\"type\":\"kv\",\"message\":\"k1=v1&k2=v2?k3=v3\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "{\"type\":\"csv\",\"message\":\"a,b,c\"}"
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__:taiye": "123"
        }
    })";
    eventGroup.FromJsonString(inJson);
    
    std::string pluginId = "testID";
    std::vector<PipelineEventGroup> logGroupList;
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);

    
    ComponentConfig componentConfig(pluginId, config);

    APSARA_TEST_TRUE_FATAL(processor.Init(componentConfig, mContext));
    processor.Process(eventGroup, logGroupList);

    APSARA_TEST_EQUAL(logGroupList.size(), 2);
    if (logGroupList.size() > 0) {
        for (auto& logGroup : logGroupList) {
            std::string outJson = logGroup.ToJsonString();
            std::cout << "outJson: " << outJson << std::endl;
        }
    }
    
    return;
}



} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}