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

};

APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestSimple, 0);
APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestJsonParse, 1);



void SlsSplUnittest::TestSimple() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
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

    APSARA_TEST_EQUAL(logGroupList.size(), 1);

    std::string outJson = logGroupList[0].ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}


void SlsSplUnittest::TestJsonParse() {
    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mSpl = "* | parse-json content | project a1 ";

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



} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}