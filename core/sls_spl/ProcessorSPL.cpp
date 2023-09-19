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
#include "sls_spl/StdoutOutput.h"
#include "sls_spl/PipeLineEventGroupInput.h"

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

bool ProcessorSPL::Init(const ComponentConfig& componentConfig) {
    Config config = componentConfig.GetConfig();
    mTopicFormat = config.mTopicFormat;
    if (config.mLogType != APSARA_LOG && ToLowerCaseString(mTopicFormat) == "default") {
        mTopicFormat = "none";
    }
    mGroupTopic = config.mGroupTopic;
    if ("customized" == config.mTopicFormat) {
        mStaticTopic = config.mCustomizedTopic;
        mIsStaticTopic = true;
    }
    SetMetricsRecordRef(Name(), componentConfig.GetId());

    // 全局初始化一次SPLLib，多次调用幂等
    initSPL();

    // logger初始化
    // logger由调用方提供
    auto logger = std::make_shared<StdoutLogger>();

    std::string errorMsg;
    // 从parser服务，获取spl plan
    const std::string parserEndpoint = "http://11.164.91.19:15107/v1/spl";
    long httpCode = 0;
    //std::string splPlan;
    std::string spl = "* | where c0 = 'value_3_0'";
    bool isSuccess = httpPost(parserEndpoint, spl, httpCode, mSplPlan, errorMsg);
    if (!isSuccess) {
        std::cerr << "request spl parser failed: " << errorMsg << ", spl is:" << spl << std::endl;
        return false;
    }
    std::cout << "splPlan: " << mSplPlan << std::endl;
    return true;
}



void ProcessorSPL::Process(PipelineEventGroup& logGroup) {
    std::string errorMsg;
    // logger初始化
    // logger由调用方提供
    auto logger = std::make_shared<StdoutLogger>();
    // std::cout << "splPlan: " << splPlan << std::endl;

    // 创建Pipeline
    // 可以设置 logger, execute超时设置(ms), 最大内存(字节)
    Error error;
    // const uint64_t timeoutMills = 100;
    // const int64_t maxMemoryBytes = 2 * 1024L * 1024L * 1024L;
    // SplPipeline spip = SplPipeline(splPlan, error, timeoutMills, maxMemoryBytes, logger);
    Error err;
    SplPipeline spip(mSplPlan, err);
    if (error.code_ != StatusCode::OK) {
        SPL_LOG_ERROR(logger, ("pipeline create error", error.msg_));
        return;
    }

    /*
    std::vector<std::unordered_map<std::string, std::string>> rows;
    for (auto r = 0; r < 10; ++r) {
        std::unordered_map<std::string, std::string> row;
        for (auto c = 0; c < 5; ++c) {
            row["c" + std::to_string(c)] = "value_" + std::to_string(r) + "_" + std::to_string(c);
        }
        rows.emplace_back(std::move(row));
    }
    */
    std::vector<std::string> colNames{"c0", "c1", "c2", "c3"};
    
    auto input = std::make_shared<PipelineEventGroupInput>(colNames, logGroup);

    // 根据spip->getInputSearches()，设置input数组
    // 此处需要调用方实现 RowInput
    std::vector<InputPtr> inputs;
    for (auto search : spip.getInputSearches()) {
        inputs.push_back(input);
    }

    // 根据spip->getOutputLabels()，设置output数组
    // 此处需要调用方实现 Output
    std::vector<OutputPtr> outputs;
    for (auto resultTaskLabel : spip.getOutputLabels()) {
        outputs.emplace_back(std::make_shared<StdoutOutput>(resultTaskLabel));
    }

    // pipeline.execute可以多次调用，每次处理一个chunk构造readers/writers
    // pipeline.execute支持多线程调用

    // 开始调用pipeline.execute
    // 传入inputs, outputs
    // 输出pipelineStats, error
    PipelineStatsPtr pipelineStatsPtr = std::make_shared<PipelineStats>();
    auto errCode = spip.execute(inputs, outputs, &errorMsg, pipelineStatsPtr);
    if (errCode != StatusCode::OK) {
        SPL_LOG_ERROR(logger, ("execute error", errorMsg));
        return;
    }
    std::cout << "pipelineStats: " << *pipelineStatsPtr.get() << std::endl;

    // 第二次调用pipeline.execute
    std::vector<InputPtr> inputs2;
    for (auto search : spip.getInputSearches()) {
        inputs2.push_back(input);
    }
    errCode = spip.execute(inputs2, outputs, &errorMsg, pipelineStatsPtr);
    if (errCode != StatusCode::OK) {
        SPL_LOG_ERROR(logger, ("errorCode", errCode)("execute error", errorMsg));
        return;
    }
    // std::cout << "pipelineStats: " << pipelineStats << std::endl;
    return;
}

bool ProcessorSPL::IsSupportedEvent(const PipelineEventPtr& /*e*/) {
    return true;
}

std::string ProcessorSPL::GetTopicName(const std::string& path, std::vector<sls_logs::LogTag>& extraTags) {
    return "";
}


} // namespace logtail