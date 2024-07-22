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
#include <memory>
#include <string>

#include "Labels.h"
#include "ScrapeConfig.h"
#include "common/Thread.h"
#include "models/PipelineEventGroup.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

class ScrapeTarget {
public:
    ScrapeTarget(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
                 std::unique_ptr<Labels> labelsPtr,
                 QueueKey queueKey,
                 size_t inputIndex);

    std::string GetHash();
    std::string GetJobName();
    std::string GetHost();
    std::map<std::string, std::string> GetHeaders();

    bool operator<(const ScrapeTarget& other) const;

private:
    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;
    std::unique_ptr<Labels> mLabelsPtr;

    // target info
    std::string mHost;
    uint32_t mPort;
    std::string mQueryString;

    std::string mHash;

    QueueKey mQueueKey;
    size_t mInputIndex;


    friend class ScrapeWork;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeTargetUnittest;
    friend class ScrapeJobUnittest;
    friend class InputPrometheusUnittest;
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

    void SetUnRegisterMs(uint64_t unRegisterMs);

private:
    void ScrapeLoop();
    void ScrapeAndPush();

    uint64_t GetRandSleep();
    sdk::HttpMessage Scrape();
    void PushEventGroup(PipelineEventGroup&&);

    ScrapeTarget mTarget;
    std::atomic<bool> mFinished;
    std::unique_ptr<sdk::HTTPClient> mClient;
    ThreadPtr mScrapeLoopThread;

    // 无损升级
    uint64_t mUnRegisterMs;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeWorkUnittest;
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScrapeJobUnittest;
#endif
};

} // namespace logtail
