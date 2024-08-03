/*
 * Copyright 2024 iLogtail Authors
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

#include "PrometheusInputRunner.h"

#include <memory>
#include <string>

#include "common/Flags.h"
#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScraperGroup.h"
#include "sdk/Common.h"
#include "sdk/Exception.h"

DEFINE_FLAG_STRING(SERVICE_HOST, "service host", "");
DEFINE_FLAG_INT32(SERVICE_PORT, "service port", 8888);
DEFINE_FLAG_STRING(_pod_name_, "agent pod name", "");

using namespace std;

namespace logtail {

PrometheusInputRunner::PrometheusInputRunner() {
    mClient = std::make_unique<sdk::CurlClient>();

    mServiceHost = STRING_FLAG(SERVICE_HOST);
    mServicePort = INT32_FLAG(SERVICE_PORT);
    mPodName = STRING_FLAG(_pod_name_);

    mScraperGroup = make_unique<ScraperGroup>();

    mScraperGroup->mServiceHost = mServiceHost;
    mScraperGroup->mServicePort = mServicePort;
    mScraperGroup->mPodName = mPodName;
}

/// @brief receive scrape jobs from input plugins and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(std::shared_ptr<ScrapeJobEvent> scrapeJobEventPtr) {
    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsSet.insert(scrapeJobEventPtr->GetId());
    }

    mScraperGroup->UpdateScrapeJob(std::move(scrapeJobEventPtr));
}

void PrometheusInputRunner::RemoveScrapeInput(const std::string& jobName) {
    mScraperGroup->RemoveScrapeJob(jobName);
    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsSet.erase(jobName);
    }
}

/// @brief targets discovery and start scrape work
void PrometheusInputRunner::Start() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Start"));

    // only register when operator exist
    if (!mServiceHost.empty()) {
        int retry = 0;
        while (true) {
            ++retry;
            sdk::HttpMessage httpResponse = SendGetRequest(prometheus::REGISTER_COLLECTOR_PATH);
            if (httpResponse.statusCode != 200) {
                LOG_ERROR(sLogger, ("register failed, statusCode", httpResponse.statusCode));
                if (retry % 3 == 0) {
                    LOG_INFO(sLogger, ("register failed, retried", ToString(retry)));
                }
            } else {
                // register success
                // response will be { "unregister_ms": 30000 }
                if (!httpResponse.content.empty()) {
                    string responseStr = httpResponse.content;
                    string errMsg;
                    Json::Value responseJson;
                    if (!ParseJsonTable(responseStr, responseJson, errMsg)) {
                        LOG_ERROR(sLogger, ("register failed, parse response failed", responseStr));
                    }
                    if (responseJson.isMember(prometheus::UNREGISTER_MS)
                        && responseJson[prometheus::UNREGISTER_MS].isUInt64()) {
                        mScraperGroup->mUnRegisterMs = responseJson[prometheus::UNREGISTER_MS].asUInt64();
                    }
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        LOG_INFO(sLogger, ("Register Success", mPodName));
    }
    mScraperGroup->Start();
}

/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Stop"));
    // only unregister when operator exist
    if (!mServiceHost.empty()) {
        for (int retry = 0; retry < 3; ++retry) {
            sdk::HttpMessage httpResponse = SendGetRequest(prometheus::UNREGISTER_COLLECTOR_PATH);
            if (httpResponse.statusCode != 200) {
                LOG_ERROR(sLogger, ("unregister failed, statusCode", httpResponse.statusCode));
            } else {
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        LOG_INFO(sLogger, ("Unregister Success", mPodName));
    }
    mScraperGroup->Stop();

    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsSet.clear();
    }
}

sdk::HttpMessage PrometheusInputRunner::SendGetRequest(const string& url) {
    map<string, string> httpHeader;
    httpHeader[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
    try {
        mClient->Send(sdk::HTTP_GET,
                      mServiceHost,
                      mServicePort,
                      url,
                      "pod_name=" + mPodName,
                      httpHeader,
                      "",
                      10,
                      httpResponse,
                      "",
                      false);
    } catch (const sdk::LOGException& e) {
        LOG_ERROR(sLogger, ("curl error", e.what())("url", url)("pod_name", mPodName));
    }
    return httpResponse;
}

bool PrometheusInputRunner::HasRegisteredPlugin() {
    ReadLock lock(mReadWriteLock);
    return !mPrometheusInputsSet.empty();
}
}; // namespace logtail