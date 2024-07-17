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
#include <map>
#include <string>

#include "Labels.h"
#include "common/Thread.h"
#include "models/PipelineEventGroup.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

class ScrapeTarget {
public:
    ScrapeTarget(const std::string& jobName,
                 const std::string& metricsPath,
                 const std::string& scheme,
                 const std::string& queryString,
                 int interval,
                 int timeout,
                 const std::map<std::string, std::string>& headers);

    bool SetLabels(const Labels& labels);
    void SetPipelineInfo(QueueKey queueKey, size_t inputIndex);
    void SetHostAndPort(const std::string& host, uint32_t port);

    std::string GetHash();
    std::string GetJobName();
    std::string GetHost();
    std::map<std::string, std::string> GetHeaders();

    bool operator<(const ScrapeTarget& other) const;

private:
    Labels mLabels;

    std::string mJobName;
    std::string mMetricsPath;
    std::string mScheme;
    std::string mQueryString;
    int mScrapeInterval;
    int mScrapeTimeout;
    std::map<std::string, std::string> mHeaders;
    uint32_t mPort;

    std::string mHost;
    std::string mTargetURL;
    std::string mHash;

    QueueKey mQueueKey;
    size_t mInputIndex;

    friend class ScrapeWork;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeTargetUnittest;
#endif
};

class ScrapeWork {
public:
    ScrapeWork(const ScrapeTarget&);

    ScrapeWork(const ScrapeWork&) = delete;
    ScrapeWork(ScrapeWork&& other) noexcept = delete;
    ScrapeWork& operator=(const ScrapeWork&) = delete;
    ScrapeWork& operator=(ScrapeWork&&) noexcept = delete;

    void StartScrapeLoop();
    void StopScrapeLoop();

private:
    void ScrapeLoop();

    uint64_t GetRandSleep();
    sdk::HttpMessage Scrape();
    void PushEventGroup(PipelineEventGroup&&);

    ScrapeTarget mTarget;
    std::atomic<bool> mFinished;
    std::unique_ptr<sdk::HTTPClient> mClient;
    ThreadPtr mScrapeLoopThread;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeWorkUnittest;
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
