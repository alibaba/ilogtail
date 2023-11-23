#include <benchmark/benchmark.h>
#include "common/JsonUtil.h"
#include "config/Config.h"
#include "sls_spl/ProcessorSPL.h"
#include "processor/ProcessorParseRegexNative.h"
#include "processor/ProcessorParseJsonNative.h"
#include "models/LogEvent.h"
#include "plugin/instance/ProcessorInstance.h"
#include <iostream>
#include <sstream>

using namespace logtail;


std::string formatSize(long long size) {
    static const char* units[] = {" B", "KB", "MB", "GB", "TB"};
    int index = 0;
    double doubleSize = static_cast<double>(size);
    while (doubleSize >= 1024.0 && index < 4) {
        doubleSize /= 1024.0;
        index++;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << std::setw(6) << std::setfill(' ') << doubleSize << " " << units[index];
    return ss.str();
}

template <int size>
static void BM_SplNoProcess(benchmark::State& state) {
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
    config.mSpl = R"(* | where true)";

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
        //std::cout << "SplNoProcess init: " << init << std::endl;
        // Perform setup here
        int count = 0;
        for (auto _ : state) {
            count++;
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);
            std::vector<PipelineEventGroup> logGroupList;
            processor.Process(eventGroup, logGroupList);
        }
        //std::cout << "SplNoProcess count: " << count << std::endl;
    }
}

template <int size>
static void BM_RawNoProcess(benchmark::State& state) {
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
    config.mAdvancedConfig.mRawLogTag = "__raw__";
    config.mRegs = std::make_shared<std::list<std::string> >();
    config.mRegs->emplace_back("(.*)");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("content");

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
        //std::cout << "RawNoProcess init: " << init << std::endl;
        int count = 0;
        // Perform setup here
        for (auto _ : state) {
            count ++;
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);
            processorInstance.Process(eventGroup);
        }
        //std::cout << "RawNoProcess count: " << count << std::endl;
    }
}


static void BM_SplRegex(int size, int batchSize) {
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
    config.mSpl = "* |parse-regexp content, '^(\\S+)\\s-\\s\\[([^]]+)]\\s-\\s(\\S+)\\s\\[(\\S+)\\s\\S+\\s\"(\\w+)\\s(\\S+)\\s([^\"]+)\"\\s(\\d+)\\s(\\d+)\\s\"([^\"]*)\"\\s\"([^\"]*)\"\\s(\\S+)\\s(\\S+)+\\s\\[([^]]*)]\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s*(\\S*).*' as client_ip,x_forward_for,remote_user,time,method,url,version,status,body_bytes_sent,http_referer,http_user_agent,request_length,request_time,proxy_upstream_name,upstream_addr,upstream_response_length,upstream_response_time,upstream_status,req_id,host";

    std::string data = "106.14.76.139 - [106.14.76.139] - - [08/Nov/2023:13:12:52 +0800] \"POST /api/v1/trade/queryLast HTTP/1.1\" 200 34 \"-\" \"okhttp/3.14.9\" 1313 0.003 [sas-devops-202210191027-svc-80] 10.33.95.216:7001 34 0.003 200 d82accba8c35ad7de27a3a64926a03d0 stosas-test.sto.cn";
            
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
            contents["content"] = data;
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
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        // This code gets timed
        eventGroup.FromJsonString(inJson);
        // Perform setup here
        
        int count = 0;
        uint64_t durationTime = 0;
        for (int i = 0; i < batchSize; i ++) {
            count++;
            std::vector<PipelineEventGroup> logGroupList;

            uint64_t startTime = GetCurrentTimeInMicroSeconds();
            processor.Process(eventGroup, logGroupList);
            durationTime += GetCurrentTimeInMicroSeconds() - startTime;
            //for (auto& logGroup : logGroupList) {
            //    std::string outJson = logGroup.ToJsonString();
            //    std::cout << "outJson: " << outJson << std::endl;
            //}
        }
        std::cout << "spl regex count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;

        //uint64_t total = (data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;

        std::cout << "stage1TimeTotal: " << processor.stage1TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage2TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage3TimeTotal << std::endl;
        
    }
}

static void BM_RawRegex(int size, int batchSize) {
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
    config.mRegs->emplace_back("^(\\S+)\\s-\\s\\[([^]]+)]\\s-\\s(\\S+)\\s\\[(\\S+)\\s\\S+\\s\"(\\w+)\\s(\\S+)\\s([^\"]+)\"\\s(\\d+)\\s(\\d+)\\s\"([^\"]*)\"\\s\"([^\"]*)\"\\s(\\S+)\\s(\\S+)+\\s\\[([^]]*)]\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s(\\S+)\\s*(\\S*).*");
    config.mKeys = std::make_shared<std::list<std::string> >();
    config.mKeys->emplace_back("client_ip,x_forward_for,remote_user,time,method,url,version,status,body_bytes_sent,http_referer,http_user_agent,request_length,request_time,proxy_upstream_name,upstream_addr,upstream_response_length,upstream_response_time,upstream_status,req_id,host");

    std::string data = "106.14.76.139 - [106.14.76.139] - - [08/Nov/2023:13:12:52 +0800] \"POST /api/v1/trade/queryLast HTTP/1.1\" 200 34 \"-\" \"okhttp/3.14.9\" 1313 0.003 [sas-devops-202210191027-svc-80] 10.33.95.216:7001 34 0.003 200 d82accba8c35ad7de27a3a64926a03d0 stosas-test.sto.cn";

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
            contents["content"] = data;
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
        int count = 0;
        // Perform setup here
        uint64_t durationTime = 0;
        for (int i = 0; i < batchSize; i ++) {
            count ++;
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);
            uint64_t startTime = GetCurrentTimeInMicroSeconds();
            processorInstance.Process(eventGroup);
            durationTime += GetCurrentTimeInMicroSeconds() - startTime;
            //std::string outJson = eventGroup.ToJsonString();
            //std::cout << "outJson: " << outJson << std::endl;
        }
        
        std::cout << "raw regex count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;
        
        //std::string outJson = eventGroup.ToJsonString();
        //std::cout << "outJson: " << outJson << std::endl;
    }
}

static void BM_SplJson(int size, int batchSize) {
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
    std::string data = "{\"_time_\":\"2023-11-15T01:04:21.80553511Z\",\"_source_\":\"stdout\",\"_pod_name_\":\"gpassport-37games-deployment-6d68b45779-rgfcz\",\"_namespace_\":\"go-app\",\"_pod_uid_\":\"22d6acfa-d55e-4be0-bb3f-ca91584a4f49\",\"_container_ip_\":\"10.101.31.136\",\"_image_name_\":\"686337631058.dkr.ecr.ap-southeast-1.amazonaws.com/gpassport-37games:master-ceb4bb745aa101731616baad3c2920a3a0b11dbf\",\"_container_name_\":\"gpassport-37games\",\"traceId\":\"44507629d8ebd96a6ff7810618d020ee\",\"logType\":\"http_access_log\",\"level\":\"INFO\",\"request\":\"/direct_login\",\"clientip\":\"218.225.227.156\",\"x_true_client_ip\":\"218.225.227.156\",\"real_ip_remote\":\"10.101.128.113\",\"xforward\":\"218.225.227.156, 70.132.19.70\",\"xforwardProto\":\"https\",\"method\":\"POST\",\"status\":\"200\",\"agent\":\"okhttp/3.12.13\",\"cost\":\"0.020\",\"bytes\":\"1409\",\"host\":\"http://gpassport.superfastgame.com\",\"remove_host\":\"http://gpassport.superfastgame.com\",\"referer\":\"-\",\"httpversion\":\"HTTP/1.1\",\"postData\":\"gpid=393ed90f-9de0-4343-80bc-a61881cfbde7&language=ja-JP&gaid=393ed90f-9de0-4343-80bc-a61881cfbde7&country=JP&userAgent=Dalvik%2F2.1.0+%28Linux%3B+U%3B+Android+9%3B+TONE+e20+Build%2FPPR1.180610.011%29&advertiser=global&channelId=googlePlay&installTime=1694994381280&jgPid=&phoneModel=TONE+e20&Isdblink=0&ratio=720x1520&gameId=191&netType=MOBILE&phoneTablet=Phone&deepLinkURL=&timeStamp=1700010260521&phoneBrand=TONE&apps=1694994408269-2661115393006544017&packageVersion=146&androidid=81444cf49a3f0f014d30b3e0571d894e&userMode=2&sdkVersionName=3.2.6_beta_1b09b7&isTrackEnabled=1&devicePlate=android&timeZone=JST&mac=&isVpnOn=0&appLanguage=ja-JP&imei=&ueAndroidId=e3010c3cc52667ae&isFirst=0&sign=5fd790e62c8e791388d913e808504c03&thirdPlatForm=mac&packageName=com.global.ztmslg&publishPlatForm=googlePlay&osVersion=9&customUserId=b7c47cec-2c1f-4b5f-8a86-1f27884da5f0&loginId=393ed90f-9de0-4343-80bc-a61881cfbde7&sdkVersion=326&ptCode=global&gameCode=ztmslg&att=1&battery=68\",\"cookieData\":\"-\",\"content_length\":\"986\",\"@timestamp\":\"2023-11-15T09:04:21+08:00\",\"__pack_meta__\":\"1|MTY5MzU5Njg0MTIwODU1NjgwOQ==|437|426\",\"__topic__\":\"\",\"__source__\":\"10.101.29.105\",\"__tag__:__pack_id__\":\"5BCAE694BB74A062-38D81B\",\"__tag__:_node_name_\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:_node_ip_\":\"10.101.29.105\",\"__tag__:__hostname__\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:__client_ip__\":\"54.251.11.83\",\"__tag__:__receive_time__\":\"1700010262\"}";

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
            contents["content"] = data;
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
        int count = 0;
        // Perform setup here
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        // This code gets timed
        eventGroup.FromJsonString(inJson);
        uint64_t durationTime = 0;
        for (int i = 0; i < batchSize; i ++) {
            count ++;
            std::vector<PipelineEventGroup> logGroupList;

            uint64_t startTime = GetCurrentTimeInMicroSeconds();
            processor.Process(eventGroup, logGroupList);
            durationTime += GetCurrentTimeInMicroSeconds() - startTime;
            //for (auto& logGroup : logGroupList) {
            //    std::string outJson = logGroup.ToJsonString();
            //    std::cout << "outJson: " << outJson << std::endl;
            //}
        }
        //uint64_t durationTime = GetCurrentTimeInMicroSeconds() - startTime;
        std::cout << "spl json count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;
        
        std::cout << "stage1TimeTotal: " << processor.stage1TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage2TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage3TimeTotal << std::endl;


    }
}

static void BM_RawJson(int size, int batchSize) {
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

    std::string data = "{\"_time_\":\"2023-11-15T01:04:21.80553511Z\",\"_source_\":\"stdout\",\"_pod_name_\":\"gpassport-37games-deployment-6d68b45779-rgfcz\",\"_namespace_\":\"go-app\",\"_pod_uid_\":\"22d6acfa-d55e-4be0-bb3f-ca91584a4f49\",\"_container_ip_\":\"10.101.31.136\",\"_image_name_\":\"686337631058.dkr.ecr.ap-southeast-1.amazonaws.com/gpassport-37games:master-ceb4bb745aa101731616baad3c2920a3a0b11dbf\",\"_container_name_\":\"gpassport-37games\",\"traceId\":\"44507629d8ebd96a6ff7810618d020ee\",\"logType\":\"http_access_log\",\"level\":\"INFO\",\"request\":\"/direct_login\",\"clientip\":\"218.225.227.156\",\"x_true_client_ip\":\"218.225.227.156\",\"real_ip_remote\":\"10.101.128.113\",\"xforward\":\"218.225.227.156, 70.132.19.70\",\"xforwardProto\":\"https\",\"method\":\"POST\",\"status\":\"200\",\"agent\":\"okhttp/3.12.13\",\"cost\":\"0.020\",\"bytes\":\"1409\",\"host\":\"http://gpassport.superfastgame.com\",\"remove_host\":\"http://gpassport.superfastgame.com\",\"referer\":\"-\",\"httpversion\":\"HTTP/1.1\",\"postData\":\"gpid=393ed90f-9de0-4343-80bc-a61881cfbde7&language=ja-JP&gaid=393ed90f-9de0-4343-80bc-a61881cfbde7&country=JP&userAgent=Dalvik%2F2.1.0+%28Linux%3B+U%3B+Android+9%3B+TONE+e20+Build%2FPPR1.180610.011%29&advertiser=global&channelId=googlePlay&installTime=1694994381280&jgPid=&phoneModel=TONE+e20&Isdblink=0&ratio=720x1520&gameId=191&netType=MOBILE&phoneTablet=Phone&deepLinkURL=&timeStamp=1700010260521&phoneBrand=TONE&apps=1694994408269-2661115393006544017&packageVersion=146&androidid=81444cf49a3f0f014d30b3e0571d894e&userMode=2&sdkVersionName=3.2.6_beta_1b09b7&isTrackEnabled=1&devicePlate=android&timeZone=JST&mac=&isVpnOn=0&appLanguage=ja-JP&imei=&ueAndroidId=e3010c3cc52667ae&isFirst=0&sign=5fd790e62c8e791388d913e808504c03&thirdPlatForm=mac&packageName=com.global.ztmslg&publishPlatForm=googlePlay&osVersion=9&customUserId=b7c47cec-2c1f-4b5f-8a86-1f27884da5f0&loginId=393ed90f-9de0-4343-80bc-a61881cfbde7&sdkVersion=326&ptCode=global&gameCode=ztmslg&att=1&battery=68\",\"cookieData\":\"-\",\"content_length\":\"986\",\"@timestamp\":\"2023-11-15T09:04:21+08:00\",\"__pack_meta__\":\"1|MTY5MzU5Njg0MTIwODU1NjgwOQ==|437|426\",\"__topic__\":\"\",\"__source__\":\"10.101.29.105\",\"__tag__:__pack_id__\":\"5BCAE694BB74A062-38D81B\",\"__tag__:_node_name_\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:_node_ip_\":\"10.101.29.105\",\"__tag__:__hostname__\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:__client_ip__\":\"54.251.11.83\",\"__tag__:__receive_time__\":\"1700010262\"}";
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
            contents["content"] = data;
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
        int count = 0;
        // Perform setup here
        uint64_t durationTime = 0;
        for (int i = 0; i < batchSize; i ++) {
            count ++;
            auto sourceBuffer = std::make_shared<SourceBuffer>();
            PipelineEventGroup eventGroup(sourceBuffer);
            // This code gets timed
            eventGroup.FromJsonString(inJson);

            uint64_t startTime = GetCurrentTimeInMicroSeconds();
            processorInstance.Process(eventGroup);
            durationTime += GetCurrentTimeInMicroSeconds() - startTime;

            //std::string outJson = eventGroup.ToJsonString();
            //std::cout << "outJson: " << outJson << std::endl;
        }
        std::cout << "raw json count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;
        
    }
}

static void BM_SplSplit(int size, int batchSize) {
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
    config.mSpl = "* | parse-csv content as _time_,_source_,_pod_name_,_namespace_,_pod_uid_,_container_ip_,_image_name_,_container_name_,traceId,logType,level,request,clientip,x_true_client_ip,real_ip_remote,xforward,xforwardProto,method,status,agent,cost,bytes,host,remove_host,referer,httpversion,postData,cookieData,content_length,\"@timestamp\",__pack_meta__,__topic__,__source__,__tag__:__pack_id__,__tag__:_node_name_,__tag__:_node_ip_,__tag__:__hostname__,__tag__:__client_ip__,__tag__:__receive_time__,__time__";
    std::string data = "2023-11-15T01:04:21.80553511Z,stdout,gpassport-37games-deployment-6d68b45779-rgfcz,go-app,22d6acfa-d55e-4be0-bb3f-ca91584a4f49,10.101.31.136,686337631058.dkr.ecr.ap-southeast-1.amazonaws.com/gpassport-37games:master-ceb4bb745aa101731616baad3c2920a3a0b11dbf,gpassport-37games,44507629d8ebd96a6ff7810618d020ee,http_access_log,INFO,/direct_login,218.225.227.156,218.225.227.156,10.101.128.113,218.225.227.156, 70.132.19.70,https,POST,200,okhttp/3.12.13,0.020,1409,http://gpassport.superfastgame.com,http://gpassport.superfastgame.com,-,HTTP/1.1,gpid=393ed90f-9de0-4343-80bc-a61881cfbde7&language=ja-JP&gaid=393ed90f-9de0-4343-80bc-a61881cfbde7&country=JP&userAgent=Dalvik%2F2.1.0+%28Linux%3B+U%3B+Android+9%3B+TONE+e20+Build%2FPPR1.180610.011%29&advertiser=global&channelId=googlePlay&installTime=1694994381280&jgPid=&phoneModel=TONE+e20&Isdblink=0&ratio=720x1520&gameId=191&netType=MOBILE&phoneTablet=Phone&deepLinkURL=&timeStamp=1700010260521&phoneBrand=TONE&apps=1694994408269-2661115393006544017&packageVersion=146&androidid=81444cf49a3f0f014d30b3e0571d894e&userMode=2&sdkVersionName=3.2.6_beta_1b09b7&isTrackEnabled=1&devicePlate=android&timeZone=JST&mac=&isVpnOn=0&appLanguage=ja-JP&imei=&ueAndroidId=e3010c3cc52667ae&isFirst=0&sign=5fd790e62c8e791388d913e808504c03&thirdPlatForm=mac&packageName=com.global.ztmslg&publishPlatForm=googlePlay&osVersion=9&customUserId=b7c47cec-2c1f-4b5f-8a86-1f27884da5f0&loginId=393ed90f-9de0-4343-80bc-a61881cfbde7&sdkVersion=326&ptCode=global&gameCode=ztmslg&att=1&battery=68,-,986,2023-11-15T09:04:21+08:00,1|MTY5MzU5Njg0MTIwODU1NjgwOQ==|437|426,,10.101.29.105,5BCAE694BB74A062-38D81B,ip-10-101-29-105.ap-southeast-1.compute.internal,10.101.29.105,ip-10-101-29-105.ap-southeast-1.compute.internal,54.251.11.83,1700010262,1700010261";
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
            contents["content"] = data;
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
        int count = 0;
        // Perform setup here
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        // This code gets timed
        eventGroup.FromJsonString(inJson);
        uint64_t durationTime = 0;
        for (int i = 0; i < batchSize; i ++) {
            count ++;
            std::vector<PipelineEventGroup> logGroupList;
            uint64_t startTime = GetCurrentTimeInMicroSeconds();
            processor.Process(eventGroup, logGroupList);
            durationTime += GetCurrentTimeInMicroSeconds() - startTime;
            //for (auto& logGroup : logGroupList) {
            //    std::string outJson = logGroup.ToJsonString();
            //    std::cout << "outJson: " << outJson << std::endl;
            //}
        }
        std::cout << "spl split count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;
        
        std::cout << "stage1TimeTotal: " << processor.stage1TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage2TimeTotal << std::endl;
        std::cout << "stage2TimeTotal: " << processor.stage3TimeTotal << std::endl;
    }
}



static void BM_SplEmpty(int size, int batchSize) {
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
    config.mSpl = R"(* | where 1=1)";
    std::string data = "{\"_time_\":\"2023-11-15T01:04:21.80553511Z\",\"_source_\":\"stdout\",\"_pod_name_\":\"gpassport-37games-deployment-6d68b45779-rgfcz\",\"_namespace_\":\"go-app\",\"_pod_uid_\":\"22d6acfa-d55e-4be0-bb3f-ca91584a4f49\",\"_container_ip_\":\"10.101.31.136\",\"_image_name_\":\"686337631058.dkr.ecr.ap-southeast-1.amazonaws.com/gpassport-37games:master-ceb4bb745aa101731616baad3c2920a3a0b11dbf\",\"_container_name_\":\"gpassport-37games\",\"traceId\":\"44507629d8ebd96a6ff7810618d020ee\",\"logType\":\"http_access_log\",\"level\":\"INFO\",\"request\":\"/direct_login\",\"clientip\":\"218.225.227.156\",\"x_true_client_ip\":\"218.225.227.156\",\"real_ip_remote\":\"10.101.128.113\",\"xforward\":\"218.225.227.156, 70.132.19.70\",\"xforwardProto\":\"https\",\"method\":\"POST\",\"status\":\"200\",\"agent\":\"okhttp/3.12.13\",\"cost\":\"0.020\",\"bytes\":\"1409\",\"host\":\"http://gpassport.superfastgame.com\",\"remove_host\":\"http://gpassport.superfastgame.com\",\"referer\":\"-\",\"httpversion\":\"HTTP/1.1\",\"postData\":\"gpid=393ed90f-9de0-4343-80bc-a61881cfbde7&language=ja-JP&gaid=393ed90f-9de0-4343-80bc-a61881cfbde7&country=JP&userAgent=Dalvik%2F2.1.0+%28Linux%3B+U%3B+Android+9%3B+TONE+e20+Build%2FPPR1.180610.011%29&advertiser=global&channelId=googlePlay&installTime=1694994381280&jgPid=&phoneModel=TONE+e20&Isdblink=0&ratio=720x1520&gameId=191&netType=MOBILE&phoneTablet=Phone&deepLinkURL=&timeStamp=1700010260521&phoneBrand=TONE&apps=1694994408269-2661115393006544017&packageVersion=146&androidid=81444cf49a3f0f014d30b3e0571d894e&userMode=2&sdkVersionName=3.2.6_beta_1b09b7&isTrackEnabled=1&devicePlate=android&timeZone=JST&mac=&isVpnOn=0&appLanguage=ja-JP&imei=&ueAndroidId=e3010c3cc52667ae&isFirst=0&sign=5fd790e62c8e791388d913e808504c03&thirdPlatForm=mac&packageName=com.global.ztmslg&publishPlatForm=googlePlay&osVersion=9&customUserId=b7c47cec-2c1f-4b5f-8a86-1f27884da5f0&loginId=393ed90f-9de0-4343-80bc-a61881cfbde7&sdkVersion=326&ptCode=global&gameCode=ztmslg&att=1&battery=68\",\"cookieData\":\"-\",\"content_length\":\"986\",\"@timestamp\":\"2023-11-15T09:04:21+08:00\",\"__pack_meta__\":\"1|MTY5MzU5Njg0MTIwODU1NjgwOQ==|437|426\",\"__topic__\":\"\",\"__source__\":\"10.101.29.105\",\"__tag__:__pack_id__\":\"5BCAE694BB74A062-38D81B\",\"__tag__:_node_name_\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:_node_ip_\":\"10.101.29.105\",\"__tag__:__hostname__\":\"ip-10-101-29-105.ap-southeast-1.compute.internal\",\"__tag__:__client_ip__\":\"54.251.11.83\",\"__tag__:__receive_time__\":\"1700010262\"}";

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
            contents["content"] = data;
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
        int count = 0;
        // Perform setup here
        auto sourceBuffer = std::make_shared<SourceBuffer>();
        PipelineEventGroup eventGroup(sourceBuffer);
        // This code gets timed
        eventGroup.FromJsonString(inJson);
        uint64_t startTime = GetCurrentTimeInMicroSeconds();
        for (int i = 0; i < batchSize; i ++) {
            count ++;
            std::vector<PipelineEventGroup> logGroupList;
            processor.Process(eventGroup, logGroupList);
            //for (auto& logGroup : logGroupList) {
            //    std::string outJson = logGroup.ToJsonString();
            //    std::cout << "outJson: " << outJson << std::endl;
            //}
        }
        uint64_t durationTime = GetCurrentTimeInMicroSeconds() - startTime;
        std::cout << "spl empty count: " << count << std::endl;
        std::cout << "durationTime: " << durationTime << std::endl;
        std::cout << "process: " << formatSize((data.size() + 7)*(uint64_t)count*1000000*(uint64_t)size/durationTime) << std::endl;
        
    }
}



int main(int argc, char** argv) {
    logtail::Logger::Instance().InitGlobalLoggers();
#ifdef NDEBUG
    std::cout << "release" << std::endl;
#else 
    std::cout << "debug" << std::endl;
#endif

    BM_SplRegex(1000, 100);
    BM_RawRegex(1000, 100);
    BM_SplJson(1000, 100);
    BM_RawJson(1000, 100);
    BM_SplSplit(1000, 100);
    BM_SplEmpty(1000, 100);
    return 0;
}