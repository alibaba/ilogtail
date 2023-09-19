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
    void TestOutPut();

};

APSARA_UNIT_TEST_CASE(SlsSplUnittest, TestOutPut, 0);



void SlsSplUnittest::TestOutPut() {
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
                    "c0" : "value_3_0",
                    "c1": "value_3_0",
                    "c2": "value_3_0",
                    "c3": "value_3_0",
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "c0" : "value_3_0",
                    "c1": "value_4_0",
                    "c2": "value_4_0",
                    "c3": "value_4_0",
                },
                "timestamp" : 12345678901,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ]
    })";
    eventGroup.FromJsonString(inJson);
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    ComponentConfig componentConfig(pluginId, config);
    APSARA_TEST_TRUE_FATAL(processorInstance.Init(componentConfig, mContext));
    processorInstance.Process(eventGroup);
    std::string outJson = eventGroup.ToJsonString();
    std::cout << "outJson: " << outJson << std::endl;
    return;
}


} // namespace logtail

int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}