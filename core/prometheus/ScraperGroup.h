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

#include "common/Lock.h"
#include "prometheus/schedulers/TargetSubscriberScheduler.h"

namespace logtail {

class ScraperGroup {
public:
    ScraperGroup();
    ~ScraperGroup() = default;
    ScraperGroup(const ScraperGroup&) = delete;
    ScraperGroup(ScraperGroup&&) = delete;
    ScraperGroup& operator=(const ScraperGroup&) = delete;
    ScraperGroup& operator=(ScraperGroup&&) = delete;

    void UpdateScrapeJob(std::shared_ptr<TargetSubscriberScheduler>);
    void RemoveScrapeJob(const std::string& jobName);

    void Start();
    void Stop();

    // from environment variable
    std::string mServiceHost;
    int32_t mServicePort;
    std::string mPodName;

    // zero-cost upgrade
    uint64_t mUnRegisterMs;

private:
    std::shared_ptr<Timer> mTimer;

    ReadWriteLock mJobRWLock;
    std::map<std::string, std::shared_ptr<TargetSubscriberScheduler>> mTargetSubscriberSchedulerMap;

    std::atomic<bool> mIsStarted;
    std::future<void> mThreadRes;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScraperGroupUnittest;
#endif
};

} // namespace logtail
