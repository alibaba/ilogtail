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

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ScrapeJob.h"
#include "common/Thread.h"

namespace logtail {

// TODO: 使用Scraper 进行异步改造
// class Scraper{
// }

/// @brief 管理所有的ScrapeJob
class ScraperGroup {
public:
    static ScraperGroup* GetInstance() {
        static ScraperGroup group;
        return &group;
    }

    void UpdateScrapeJob(std::unique_ptr<ScrapeJob> scrapeJobPtr);
    void RemoveScrapeJob(const std::string& jobName);

    void Start();
    void Stop();

private:
    ScraperGroup();

    std::mutex mMutex;
    std::unordered_map<std::string, std::unique_ptr<ScrapeJob>> mScrapeJobMap;
    std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<ScrapeWork>>> mScrapeWorkMap;

    std::atomic<bool> mFinished;
    ThreadPtr mScraperThread;

    void ProcessScrapeWorkUpdate();
    void UpdateScrapeWork(const std::string& jobName);

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScraperGroupUnittest;
#endif
};

} // namespace logtail
