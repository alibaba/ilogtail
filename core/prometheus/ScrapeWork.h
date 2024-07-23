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
#include <memory>
#include <string>

#include "ScrapeConfig.h"
#include "ScrapeTarget.h"
#include "common/Thread.h"
#include "models/PipelineEventGroup.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

class ScrapeWork {
public:
    ScrapeWork(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
               const ScrapeTarget& scrapeTarget,
               QueueKey queueKey,
               size_t inputIndex);

    ScrapeWork(const ScrapeWork&) = delete;
    ScrapeWork(ScrapeWork&& other) noexcept = delete;
    ScrapeWork& operator=(const ScrapeWork&) = delete;
    ScrapeWork& operator=(ScrapeWork&&) noexcept = delete;

    bool operator<(const ScrapeWork& other) const;

    void StartScrapeLoop();
    void StopScrapeLoop();

    void SetUnRegisterMs(uint64_t unRegisterMs);

private:
    void ScrapeLoop();
    void ScrapeAndPush();

    uint64_t GetRandSleep();
    sdk::HttpMessage Scrape();
    void PushEventGroup(PipelineEventGroup&&);

    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;
    ScrapeTarget mScrapeTarget;


    std::string mHash;

    QueueKey mQueueKey;
    size_t mInputIndex;

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
