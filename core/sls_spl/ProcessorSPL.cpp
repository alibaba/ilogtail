/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sls_spl/ProcessorSPL.h"
#include <curl/curl.h>
#include <iostream>
#include "cmd/MapRowInput.h"
#include "logger/StdoutLogger.h"
#include "pipeline/SplPipeline.h"
#include "sls_spl/PipelineEventGroupInput.h"
#include "sls_spl/PipelineEventGroupOutput.h"
#include "logger/Logger.h"


using namespace apsara::sls::spl;

namespace logtail {

// callback function to store the response
static size_t write_callback(char* buffer, size_t size, size_t nmemb, std::string* write_buf) {
    unsigned long sizes = size * nmemb;
    if (buffer == NULL) {
        return 0;
    }

    write_buf->append(buffer, sizes);
    return sizes;
}

bool httpPost(
    const std::string& requestUri,
    const std::string& requestBody,
    long& httpCode,
    std::string& respBody,
    std::string& errorMessage) {
    if (requestUri.empty()) {
        errorMessage = "request uri is empty";
        return false;
    }

    CURLcode res;
    struct curl_slist* headers = NULL;
    // headers = curl_slist_append(headers, "Content-Type:application/json;charset=UTF-8");
    headers = curl_slist_append(headers, "X-Presto-Time-Zone:Asia/Shanghai");
    headers = curl_slist_append(headers, "User-Agent:Spl");
    headers = curl_slist_append(headers, "X-Presto-Source:presto-cli");
    headers = curl_slist_append(headers, "X-Presto-Language:en-US");
    headers = curl_slist_append(headers, "X-Presto-User:u_1");
    headers = curl_slist_append(headers, "X-Presto-Transaction-id:NONE");
    headers = curl_slist_append(headers, "X-Presto-Catalog: sls");
    headers = curl_slist_append(headers, "X-Presto-Schema: p_1");
    headers = curl_slist_append(headers, "Content-type: application/octet-stream");

    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curlInstance = curl_easy_init();
    if (!curlInstance) {
        curl_slist_free_all(headers);
        errorMessage = "init curl handler failed";
        return false;
    }
    curl_easy_setopt(curlInstance, CURLOPT_URL, requestUri.c_str());
    curl_easy_setopt(curlInstance, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curlInstance, CURLOPT_TIMEOUT, 5);
    curl_easy_setopt(curlInstance, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curlInstance, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(curlInstance, CURLOPT_POST, 1);
    curl_easy_setopt(curlInstance, CURLOPT_POSTFIELDSIZE_LARGE, requestBody.size());
    curl_easy_setopt(curlInstance, CURLOPT_POSTFIELDS, requestBody.c_str());

    curl_easy_setopt(curlInstance, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curlInstance, CURLOPT_WRITEDATA, &respBody);

    res = curl_easy_perform(curlInstance);
    curl_easy_getinfo(curlInstance, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curlInstance);
    curl_slist_free_all(headers);
    if (res != CURLE_OK) {
        errorMessage = curl_easy_strerror(res);
        return false;
    }
    return true;
}

bool ProcessorSPL::Init(const ComponentConfig& componentConfig, PipelineContext& context) {
    Config config = componentConfig.GetConfig();
   
    // SetMetricsRecordRef(Name(), componentConfig.GetId());
    initSPL();

    // logger初始化
    // logger由调用方提供
    auto logger = std::make_shared<StdoutLogger>();

    std::string errorMsg;
    // 从parser服务，获取spl plan
    const std::string parserEndpoint = "http://11.164.91.19:15107/v1/spl";
    long httpCode = 0;
    std::string splPlan;
    std::string spl = config.mSpl;
    bool isSuccess = httpPost(parserEndpoint, spl, httpCode, splPlan, errorMsg);
    if (!isSuccess) {
        LOG_ERROR(sLogger, ("request spl parser failed ", errorMsg)("spl ", spl));
        return false;
    }

    LOG_INFO(sLogger, ("splPlan", splPlan));
    if (splPlan.find("{\"error\"") != std::string::npos) {
        LOG_ERROR(sLogger, ("request spl parser failed ", splPlan)("spl ", spl));
        return false;
    }

    const uint64_t timeoutMills = 100;
    const int64_t maxMemoryBytes = 2 * 1024L * 1024L * 1024L;
    // SplPipeline spip = SplPipeline(splPlan, error, timeoutMills, maxMemoryBytes, logger);
    Error error;
    mSPLPipelinePtr = std::make_shared<SplPipeline>(splPlan, error, timeoutMills, maxMemoryBytes, logger);
    if (error.code_ != StatusCode::OK) {
        LOG_ERROR(sLogger, ("pipeline create error", error.msg_));
        return false;
    }
    return true;
}



void ProcessorSPL::Process(PipelineEventGroup& logGroup, std::vector<PipelineEventGroup>& logGroupList) {
    std::string errorMsg;
    // logger初始化
    // logger由调用方提供
    auto logger = std::make_shared<StdoutLogger>();

    std::vector<std::string> colNames{"timestamp", "timestampNanosecond", "content"};

    std::string outJson = logGroup.ToJsonString();
    LOG_INFO(sLogger, ("before execute", outJson));
    
    auto input = std::make_shared<PipelineEventGroupInput>(colNames, logGroup);

    // 根据spip->getInputSearches()，设置input数组
    std::vector<InputPtr> inputs;
    for (auto search : mSPLPipelinePtr->getInputSearches()) {
        inputs.push_back(input);
    }

    // 根据spip->getOutputLabels()，设置output数组
    std::vector<OutputPtr> outputs;
    EventsContainer newEvents;

    for (auto resultTaskLabel : mSPLPipelinePtr->getOutputLabels()) {
        outputs.emplace_back(std::make_shared<PipelineEventGroupOutput>(logGroup, newEvents, resultTaskLabel));
    }

    // 开始调用pipeline.execute
    // 传入inputs, outputs
    // 输出pipelineStats, error
    PipelineStatsPtr pipelineStatsPtr = std::make_shared<PipelineStats>();
    auto errCode = mSPLPipelinePtr->execute(inputs, outputs, &errorMsg, pipelineStatsPtr);
    if (errCode != StatusCode::OK) {
        LOG_INFO(sLogger, ("execute error", errorMsg));
        return;
    }

    logGroup.SwapEvents(newEvents);

    outJson = logGroup.ToJsonString();
    LOG_INFO(sLogger, ("after swap", outJson));
    

    logGroupList.emplace_back(logGroup);
    LOG_INFO(sLogger, ("pipelineStats", *pipelineStatsPtr.get()));
    return;
}

bool ProcessorSPL::IsSupportedEvent(const PipelineEventPtr& /*e*/) {
    return true;
}

} // namespace logtail