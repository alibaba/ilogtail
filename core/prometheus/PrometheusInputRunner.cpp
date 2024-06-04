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
PrometheusInputRunner::PrometheusInputRunner() {
}

/// @brief 接收并更新插件注册的每个ScrapeJob
void PrometheusInputRunner::UpdateScrapeInput(const string& inputName, const vector<ScrapeJob>& jobs) {
    for (const ScrapeJob& job : scrapeInputsMap[inputName]) {
        ScraperGroup::GetInstance()->RemoveScrapeJob(job);
    }
    scrapeInputsMap[inputName].clear();
    for (const ScrapeJob& job : jobs) {
        if (!scrapeInputsMap[inputName].count(job)) {
            scrapeInputsMap[inputName].insert(job);
            ScraperGroup::GetInstance()->UpdateScrapeJob(job);
        }
    }
}

void PrometheusInputRunner::RemoveScrapeInput(const std::string& inputName) {
    for (const ScrapeJob& job : scrapeInputsMap[inputName]) {
        ScraperGroup::GetInstance()->RemoveScrapeJob(job);
    }
    scrapeInputsMap.erase(inputName);
}

/// @brief 初始化管理类
void PrometheusInputRunner::Start() {
    ScraperGroup::GetInstance()->Start();
}


/// @brief 停止管理类，资源回收
void PrometheusInputRunner::Stop() {
    ScraperGroup::GetInstance()->Stop();
    scrapeInputsMap.clear();
}

bool PrometheusInputRunner::HasRegisteredPlugin() {
    return !scrapeInputsMap.empty();
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