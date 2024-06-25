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
    ScraperGroup::GetInstance()->Start();
}


/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
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