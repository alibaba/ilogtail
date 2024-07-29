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
#include <unordered_map>

#include "common/Flags.h"
#include "common/StringTools.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScraperGroup.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

DEFINE_FLAG_STRING(OPERATOR_HOST, "operator host", "");
DEFINE_FLAG_INT32(OPERATOR_PORT, "operator port", 8888);
DEFINE_FLAG_STRING(POD_NAME, "agent pod name", "");

using namespace std;

namespace logtail {

PrometheusInputRunner::PrometheusInputRunner() {
    mClient = std::make_unique<sdk::CurlClient>();

    mOperatorHost = STRING_FLAG(OPERATOR_HOST);
    mPodName = STRING_FLAG(POD_NAME);
    mOperatorPort = INT32_FLAG(OPERATOR_PORT);

    mScraperGroup = make_unique<ScraperGroup>();
}

/// @brief receive scrape jobs from input plugins and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(const string& inputName, std::unique_ptr<ScrapeJob> scrapeJobPtr) {
    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsMap[inputName] = scrapeJobPtr->mJobName;
    }

    // set job info
    scrapeJobPtr->mOperatorHost = mOperatorHost;
    scrapeJobPtr->mOperatorPort = mOperatorPort;
    scrapeJobPtr->mPodName = mPodName;

    mScraperGroup->UpdateScrapeJob(std::move(scrapeJobPtr));
}

void PrometheusInputRunner::RemoveScrapeInput(const std::string& inputName) {
    string jobName;
    {
        ReadLock lock(mReadWriteLock);
        jobName = mPrometheusInputsMap[inputName];
    }
    mScraperGroup->RemoveScrapeJob(jobName);
    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsMap.erase(inputName);
    }
}

/// @brief targets discovery and start scrape work
void PrometheusInputRunner::Start() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Start"));

    if (!mOperatorHost.empty()) {
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
                // http.content only has one timestamp string in millisecond format
                if (!httpResponse.content.empty()) {
                    mScraperGroup->mUnRegisterMs = stoll(httpResponse.content);
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
    mScraperGroup->Stop();

    {
        WriteLock lock(mReadWriteLock);
        mPrometheusInputsMap.clear();
    }
}

sdk::HttpMessage PrometheusInputRunner::SendGetRequest(const string& url) {
    map<string, string> httpHeader;
    httpHeader[sdk::X_LOG_REQUEST_ID] = prometheus::MATRIX_PROMETHEUS_PREFIX + mPodName;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = prometheus::MATRIX_PROMETHEUS_PREFIX + mPodName;
    try {
        mClient->Send(sdk::HTTP_GET,
                      mOperatorHost,
                      mOperatorPort,
                      prometheus::UNREGISTER_COLLECTOR_PATH,
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
    return !mPrometheusInputsMap.empty();
}
}; // namespace logtail