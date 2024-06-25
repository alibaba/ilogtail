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

#include "Labels.h"
#include "common/Thread.h"
#include "models/PipelineEventGroup.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

struct ScrapeTarget {
    ScrapeTarget(const std::vector<std::string>& targets, const Labels& labels, const std::string& source);
    bool operator<(const ScrapeTarget& other) const;

    std::vector<std::string> mTargets;
    Labels mLabels;
    std::string mSource;

    std::string mHash;

    std::string mJobName;
    std::string mMetricsPath;
    std::string mScheme;
    std::string mHost;
    std::string mQueryString;
    int mPort;
    int mScrapeInterval;
    int mScrapeTimeout;

    QueueKey queueKey;
    size_t inputIndex;

#ifdef APSARA_UNIT_TEST_MAIN
    ScrapeTarget(std::string jobName,
                 std::string metricsPath,
                 std::string scheme,
                 std::string host,
                 int port,
                 int interval,
                 int timeout)
        : mJobName(jobName),
          mMetricsPath(metricsPath),
          mScheme(scheme),
          mHost(host),
          mPort(port),
          mScrapeInterval(interval),
          mScrapeTimeout(timeout) {}

#endif
};


class ScrapeWork {
public:
    ScrapeWork(const ScrapeTarget&);

    ScrapeWork(const ScrapeWork&) = delete;
    ScrapeWork(ScrapeWork&& other) noexcept = default;
    ScrapeWork& operator=(const ScrapeWork&) = delete;
    ScrapeWork& operator=(ScrapeWork&&) noexcept = default;


    void StartScrapeLoop();

    void StopScrapeLoop();


private:
    void scrapeLoop();

    uint64_t getRandSleep();
    sdk::HttpMessage scrape();
    void pushEventGroup(PipelineEventGroup&&);

    ScrapeTarget mTarget;
    std::atomic<bool> mFinished;
    std::unique_ptr<sdk::HTTPClient> mClient;
    ThreadPtr mScrapeLoopThread;
    // ReadWriteLock mScrapeLoopThreadRWL;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeWorkUnittest;
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
