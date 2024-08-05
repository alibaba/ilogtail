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

#include <json/json.h>

#include <cstdint>
#include <memory>
#include <string>

#include "common/http/HttpResponse.h"
#include "prometheus/PromTaskCallback.h"
#include "prometheus/Mock.h"
#include "prometheus/ScrapeConfig.h"
#include "prometheus/ScrapeScheduler.h"
#include "queue/FeedbackQueueKey.h"

namespace logtail {

class TargetsSubscriber : public PromTaskCallback {
public:
    TargetsSubscriber();
    ~TargetsSubscriber() override = default;

    bool Init(const Json::Value& scrapeConfig);
    bool operator<(const TargetsSubscriber& other) const;

    void Process(const HttpResponse&) override;
    void SetTimer(std::shared_ptr<Timer> timer);

    std::string GetId() const override;
    bool IsCancelled() override;

    // from pipeline context
    QueueKey mQueueKey;
    size_t mInputIndex;

    // zero cost upgrade
    uint64_t mUnRegisterMs;

private:
    bool ParseTargetGroups(const std::string& content, std::set<ScrapeScheduler>& newScrapeWorkSet) const;

    std::unique_ptr<TimerEvent> BuildWorkTimerEvent(std::shared_ptr<ScrapeScheduler> workEvent);

    uint64_t GetRandSleep(const std::string& hash) const;

    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;

    ReadWriteLock mRWLock;
    std::set<ScrapeScheduler> mScrapeWorkSet;

    std::string mJobName;
    std::shared_ptr<Timer> mTimer;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeJobEventUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail