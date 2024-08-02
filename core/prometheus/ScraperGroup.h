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

#include <cstdint>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "prometheus/ScrapeJobEvent.h"

namespace logtail {

class ScraperGroup {
public:
    ScraperGroup();
    ~ScraperGroup();
    ScraperGroup(const ScraperGroup&) = delete;
    ScraperGroup(ScraperGroup&&) = delete;
    ScraperGroup& operator=(const ScraperGroup&) = delete;
    ScraperGroup& operator=(ScraperGroup&&) = delete;

    void UpdateScrapeJob(std::shared_ptr<ScrapeJobEvent>);
    void RemoveScrapeJob(const std::string& jobName);

    void Start();
    void Stop();

    // zero-cost upgrade
    uint64_t mUnRegisterMs;

private:
    ScrapeEvent BuildScrapeEvent(std::shared_ptr<AsyncEvent> asyncEvent,
                                 uint64_t intervalSeconds,
                                 ReadWriteLock& rwLock,
                                 std::unordered_set<std::string>& validationSet,
                                 std::string hash);

    std::shared_ptr<Timer> mTimer;

    ReadWriteLock mJobRWLock;
    std::unordered_set<std::string> mJobValidSet;
    std::map<std::string, std::shared_ptr<ScrapeJobEvent>> mJobEventMap;

    std::mutex mStartMux;
    bool mIsStarted;
    std::future<void> mThreadRes;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScraperGroupUnittest;
#endif
};

} // namespace logtail
