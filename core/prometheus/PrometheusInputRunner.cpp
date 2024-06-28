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

#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "common/StringTools.h"
#include "prometheus/Scraper.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

/// @brief receive scrape jobs from input plugin and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(const string& inputName, std::unique_ptr<ScrapeJob> scrapeJobPtr) {
    mPrometheusInputsMap[inputName] = scrapeJobPtr->mJobName;
    ScraperGroup::GetInstance()->UpdateScrapeJob(move(scrapeJobPtr));
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
        httpHeader[sdk::X_LOG_REQUEST_ID] = "matrix_prometheus_" + ToString(getenv("POD_NAME"));
        sdk::HttpMessage httpResponse;
        try {
            mClient->Send(sdk::HTTP_GET,
                          ToString(getenv("OPERATOR_HOST")),
                          stoi(getenv("OPERATOR_PORT")),
                          "/register_collector",
                          "pod_name=" + ToString(getenv("POD_NAME")),
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
            break;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    LOG_INFO(sLogger, ("Register Success", ToString(getenv("POD_NAME"))));
    ScraperGroup::GetInstance()->Start();
}


/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Stop"));
    for (int retry = 0; retry < 3; ++retry) {
        map<string, string> httpHeader;
        httpHeader[sdk::X_LOG_REQUEST_ID] = "matrix_prometheus_" + ToString(getenv("POD_NAME"));
        sdk::HttpMessage httpResponse;
        try {
            mClient->Send(sdk::HTTP_GET,
                          ToString(getenv("OPERATOR_HOST")),
                          stoi(getenv("OPERATOR_PORT")),
                          "/unregister_collector",
                          "pod_name=" + ToString(getenv("POD_NAME")),
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
    LOG_INFO(sLogger, ("Unregister Success", ToString(getenv("POD_NAME"))));
    ScraperGroup::GetInstance()->Stop();
    mPrometheusInputsMap.clear();
}

bool PrometheusInputRunner::HasRegisteredPlugin() {
    return !mPrometheusInputsMap.empty();
}

/// 暂时不做处理
/// 使用读写锁控制
// void PrometheusInputRunner::HoldOn() {
//     mEventLoopThreadRWL.lock();
// }

/// 启动或重启
/// 使用读写锁控制
/// 创建scrapeloop
// void PrometheusInputRunner::Reload() {
//     // StartEventLoop();
//     mEventLoopThreadRWL.unlock();
// }

}; // namespace logtail