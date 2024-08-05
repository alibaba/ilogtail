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

#include <memory>
#include <string>

#include "BaseScheduler.h"
#include "common/http/HttpResponse.h"
#include "models/PipelineEventGroup.h"
#include "prometheus/Mock.h"
#include "prometheus/ScrapeTarget.h"
#include "prometheus/schedulers/ScrapeConfig.h"
#include "queue/FeedbackQueueKey.h"

#ifdef APSARA_UNIT_TEST_MAIN
#include "queue/ProcessQueueItem.h"
#endif

namespace logtail {

class ScrapeScheduler : public BaseScheduler {
public:
    ScrapeScheduler(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
                    const ScrapeTarget& scrapeTarget,
                    QueueKey queueKey,
                    size_t inputIndex);
    ScrapeScheduler(const ScrapeScheduler&) = default;
    ~ScrapeScheduler() override = default;

    bool operator<(const ScrapeScheduler& other) const;

    void OnMetricResult(const HttpResponse&);
    void SetTimer(std::shared_ptr<Timer> timer);

    std::string GetId() const;

    void ScheduleNext() override;

    uint64_t GetRandSleep() const;

private:
    void PushEventGroup(PipelineEventGroup&&);

    PipelineEventGroup BuildPipelineEventGroup(const std::string& content, time_t timestampNs);

    std::unique_ptr<TimerEvent> BuildScrapeTimerEvent(std::chrono::steady_clock::time_point execTime);

    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;

    ScrapeTarget mScrapeTarget;
    std::string mHash;

    QueueKey mQueueKey;
    size_t mInputIndex;
    std::shared_ptr<Timer> mTimer;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorLogToMetricNativeUnittest;
    friend class ScrapeWorkEventUnittest;
    std::vector<std::shared_ptr<ProcessQueueItem>> mItem;
#endif
};

} // namespace logtail
