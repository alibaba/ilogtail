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

#include "common/StringTools.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/Scraper.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

PrometheusInputRunner::PrometheusInputRunner() {
    mClient = std::make_unique<sdk::CurlClient>();

    // get operator info by environment variables
    mOperatorHost = ToString(getenv(prometheus::OPERATOR_HOST));
    string portStr = ToString(getenv(prometheus::OPERATOR_PORT));
    // get agent pod name by environment variables
    mPodName = ToString(getenv(prometheus::POD_NAME));

    if (portStr.empty()) {
        LOG_ERROR(sLogger, ("PrometheusInputRunner operator port error", portStr));
        mOperatorPort = 8888;
    } else {
        mOperatorPort = stoi(portStr);
    }
}

/// @brief receive scrape jobs from input plugins and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(const string& inputName, std::unique_ptr<ScrapeJob> scrapeJobPtr) {
    mPrometheusInputsMap[inputName] = scrapeJobPtr->mJobName;

    // set job info
    scrapeJobPtr->mOperatorHost = mOperatorHost;
    scrapeJobPtr->mOperatorPort = mOperatorPort;
    scrapeJobPtr->mPodName = mPodName;

    ScraperGroup::GetInstance()->UpdateScrapeJob(std::move(scrapeJobPtr));
}

void PrometheusInputRunner::RemoveScrapeInput(const std::string& inputName) {
    ScraperGroup::GetInstance()->RemoveScrapeJob(mPrometheusInputsMap[inputName]);
    mPrometheusInputsMap.erase(inputName);
}

/// @brief targets discovery and start scrape work
void PrometheusInputRunner::Start() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Start"));
    while (true) {
        map<string, string> httpHeader;
        httpHeader[sdk::X_LOG_REQUEST_ID] = prometheus::MATRIX_PROMETHEUS_PREFIX + mPodName;
        sdk::HttpMessage httpResponse;
        httpResponse.header[sdk::X_LOG_REQUEST_ID] = prometheus::MATRIX_PROMETHEUS_PREFIX + mPodName;
        try {
            mClient->Send(sdk::HTTP_GET,
                          mOperatorHost,
                          mOperatorPort,
                          prometheus::REGISTER_COLLECTOR_PATH,
                          "pod_name=" + mPodName,
                          httpHeader,
                          "",
                          10,
                          httpResponse,
                          "",
                          false);
        } catch (const sdk::LOGException& e) {
            LOG_ERROR(sLogger, ("curl error", e.what()));
        }
        if (httpResponse.statusCode != 200) {
            LOG_ERROR(sLogger, ("register failed, statusCode", httpResponse.statusCode));
        } else {
            // register success
            if (httpResponse.content.empty()) {
                LOG_ERROR(sLogger, ("unregister failed, content is empty", ""));
            } else {
                ScraperGroup::GetInstance()->mUnRegisterMs = stoll(httpResponse.content);
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG_INFO(sLogger, ("Register Success", mPodName));
    ScraperGroup::GetInstance()->Start();
}

/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Stop"));
    for (int retry = 0; retry < 3; ++retry) {
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
            LOG_ERROR(sLogger, ("curl error", e.what()));
        }
        if (httpResponse.statusCode != 200) {
            LOG_ERROR(sLogger, ("unregister failed, statusCode", httpResponse.statusCode));
        } else {
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG_INFO(sLogger, ("Unregister Success", mPodName));
    ScraperGroup::GetInstance()->Stop();
    mPrometheusInputsMap.clear();
}

bool PrometheusInputRunner::HasRegisteredPlugin() {
    return !mPrometheusInputsMap.empty();
}

}; // namespace logtail