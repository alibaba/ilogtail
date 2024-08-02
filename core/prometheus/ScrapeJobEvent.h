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

#include "prometheus/AsyncEvent.h"
#include "prometheus/ScrapeConfig.h"
#include "prometheus/ScrapeWorkEvent.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

class ScrapeJobEvent : public AsyncEvent {
public:
    ScrapeJobEvent();
    ~ScrapeJobEvent() override = default;


    bool Init(const Json::Value& scrapeConfig);
    bool operator<(const ScrapeJobEvent& other) const;

    void Process(const sdk::HttpMessage&) override;

    sdk::AsynRequest BuildAsyncRequest() const;
    void SetTimer(std::shared_ptr<Timer> timer);

    std::string mJobName;

    // from environment variable
    std::string mServiceHost;
    uint32_t mServicePort;
    std::string mPodName;

    // from pipeline context
    QueueKey mQueueKey;
    size_t mInputIndex;

    // zero cost upgrade
    uint64_t mUnRegisterMs;

private:
    bool ParseTargetGroups(const std::string& content, std::set<ScrapeWorkEvent> newScrapeWorkSet) const;

    ScrapeEvent BuildScrapeEvent(std::shared_ptr<AsyncEvent> asyncEvent,
                                 uint64_t intervalSeconds,
                                 ReadWriteLock& rwLock,
                                 std::unordered_set<std::string>& validSet,
                                 std::string hash);
    uint64_t GetRandSleep(const std::string& hash) const;

    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;

    ReadWriteLock mRWLock;
    std::set<ScrapeWorkEvent> mScrapeWorkSet;
    std::unordered_set<std::string> mValidSet;

    std::shared_ptr<Timer> mTimer;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeJobEventUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail