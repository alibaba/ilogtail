#include "unittest/Unittest.h"

#include <benchmark/benchmark.h>
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "sls_spl/ProcessorSPL.h"
#include "processor/ProcessorParseRegexNative.h"
#include "processor/ProcessorParseJsonNative.h"
#include "models/LogEvent.h"
#include "plugin/ProcessorInstance.h"
#include <iostream>
#include <sstream>

using namespace logtail;



template <int size>
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
    Json::Value root;
    Json::Value events;
    for (int i = 0; i < size; i ++) {
        Json::Value event;
        event["type"] = 1;
        event["timestamp"] = 1234567890;
        event["timestampNanosecond"] = 0;
        {
            Json::Value contents;
            contents["content"] = "10.0.0.0 GET /index.html 15824 0.043";
            
            event["contents"] = std::move(contents);
        }
        events.append(event);
    }

    root["events"] = events;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(root, &oss);
    std::string inJson = oss.str();
    //std::cout << "inJson: " << inJson << std::endl;

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

template <int size>
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
    Json::Value root;
    Json::Value events;
    for (int i = 0; i < size; i ++) {
        Json::Value event;
        event["type"] = 1;
        event["timestamp"] = 1234567890;
        event["timestampNanosecond"] = 0;
        {
            Json::Value contents;
            contents["content"] = "10.0.0.0 GET /index.html 15824 0.043";
            
            event["contents"] = std::move(contents);
        }
        events.append(event);
    }

    root["events"] = events;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(root, &oss);
    std::string inJson = oss.str();
    //std::cout << "inJson: " << inJson << std::endl;

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

template <int size>
static void BM_SplJson(benchmark::State& state) {
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
    config.mSpl = R"(* | parse-json content)";

    // make events
    Json::Value root;
    Json::Value events;
    for (int i = 0; i < size; i ++) {
        Json::Value event;
        event["type"] = 1;
        event["timestamp"] = 1234567890;
        event["timestampNanosecond"] = 0;
        {
            Json::Value contents;
            contents["content"] = "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}";
            event["contents"] = std::move(contents);
        }
        events.append(event);
    }

    root["events"] = events;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(root, &oss);
    std::string inJson = oss.str();
    //std::cout << "inJson: " << inJson << std::endl;

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

template <int size>
static void BM_RawJson(benchmark::State& state) {
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
    Json::Value root;
    Json::Value events;
    for (int i = 0; i < size; i ++) {
        Json::Value event;
        event["type"] = 1;
        event["timestamp"] = 1234567890;
        event["timestampNanosecond"] = 0;
        {
            Json::Value contents;
            contents["content"] = "{\"name\":\"Mike\",\"age\":25,\"is_student\":false,\"address\":{\"city\":\"Hangzhou\",\"postal_code\":\"100000\"},\"courses\":[\"Math\",\"English\",\"Science\"],\"scores\":{\"Math\":90,\"English\":85,\"Science\":95}}";
            event["contents"] = std::move(contents);
        }
        events.append(event);
    }

    root["events"] = events;

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(root, &oss);
    std::string inJson = oss.str();
    //std::cout << "inJson: " << inJson << std::endl;

    // run function
    ProcessorParseJsonNative& processor = *(new ProcessorParseJsonNative);
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


BENCHMARK_TEMPLATE(BM_SplRegex, 10)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawRegex, 10)->MinTime(5);

BENCHMARK_TEMPLATE(BM_SplRegex, 100)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawRegex, 100)->MinTime(5);

BENCHMARK_TEMPLATE(BM_SplRegex, 1000)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawRegex, 1000)->MinTime(5);


BENCHMARK_TEMPLATE(BM_SplJson, 10)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawJson, 10)->MinTime(5);

BENCHMARK_TEMPLATE(BM_SplJson, 100)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawJson, 100)->MinTime(5);

BENCHMARK_TEMPLATE(BM_SplJson, 1000)->MinTime(5);
BENCHMARK_TEMPLATE(BM_RawJson, 1000)->MinTime(5);

// Run the benchmark
BENCHMARK_MAIN();