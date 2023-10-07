#include "unittest/Unittest.h"

#include <benchmark/benchmark.h>
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "sls_spl/ProcessorSPL.h"
#include "processor/ProcessorParseRegexNative.h"
#include "models/LogEvent.h"
#include "plugin/ProcessorInstance.h"
#include <iostream>
#include <sstream>


using namespace logtail;

static void BM_SplRegex(benchmark::State& state) {
    logtail::Logger::Instance().InitGlobalLoggers();

    PipelineContext mContext;
    mContext.SetConfigName("project##config_0");
    mContext.SetLogstoreName("logstore");
    mContext.SetProjectName("project");
    mContext.SetRegion("cn-shanghai");
         // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = false;
    config.mSpl = R"(* | parse-regexp content, '(\S+)\s+(\w+)' as ip, method)";

    // make events
    
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "10.0.0.0 GET /index.html 15824 0.043"
                },
                "timestamp" : 1234567890,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "10.0.0.1 POST /index.html 15824 0.043"
                },
                "timestamp" : 1234567890,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";

    std::string pluginId = "testID";
    // run function
    ProcessorSPL& processor = *(new ProcessorSPL);
    ComponentConfig componentConfig(pluginId, config);
    bool init = processor.Init(componentConfig, mContext);
    if (init) {
        // Perform setup here
        for (auto _ : state) {
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);
            std::vector<PipelineEventGroup> logGroupList;
            processor.Process(eventGroup, logGroupList);
        }
    }
}


static void BM_RawRegex(benchmark::State& state) {
    logtail::Logger::Instance().InitGlobalLoggers();

    PipelineContext mContext;
    mContext.SetConfigName("project##config_0");
    mContext.SetLogstoreName("logstore");
    mContext.SetProjectName("project");
    mContext.SetRegion("cn-shanghai");

    // make config
    Config config;
    config.mDiscardUnmatch = false;
    config.mUploadRawLog = true;
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back(R"((\S+)\s+(\w+).*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("ip,method");

    // make events
    
    std::string inJson = R"({
        "events" :
        [
            {
                "contents" :
                {
                    "content" : "10.0.0.0 GET /index.html 15824 0.043"
                },
                "timestamp" : 1234567890,
                "timestampNanosecond" : 0,
                "type" : 1
            },
            {
                "contents" :
                {
                    "content" : "10.0.0.1 POST /index.html 15824 0.043"
                },
                "timestamp" : 1234567890,
                "timestampNanosecond" : 0,
                "type" : 1
            }
        ],
        "tags" : {
            "__tag__": "123"
        }
    })";


    // run function
    ProcessorParseRegexNative& processor = *(new ProcessorParseRegexNative);
    std::string pluginId = "testID";
    ProcessorInstance processorInstance(&processor, pluginId);
    ComponentConfig componentConfig(pluginId, config);

    bool init = processorInstance.Init(componentConfig, mContext);
    if (init) {
        // Perform setup here
        for (auto _ : state) {
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);
            processorInstance.Process(eventGroup);
        }
    }
}



// Register the function as a benchmark
BENCHMARK(BM_SplRegex)->MinTime(10);
BENCHMARK(BM_RawRegex)->MinTime(10);

// Run the benchmark
BENCHMARK_MAIN();