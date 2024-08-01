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

#include "prometheus/ScraperGroup.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/Thread.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScrapeWorkEvent.h"

using namespace std;

namespace logtail {

ScraperGroup::ScraperGroup() : mUnRegisterMs(0), mTimer(make_shared<Timer>()), mIsStarted(false) {
}

ScraperGroup::~ScraperGroup() {
    Stop();
}

void ScraperGroup::UpdateScrapeJob(std::shared_ptr<ScrapeJobEvent> scrapeJobEventPtr) {
    RemoveScrapeJob(scrapeJobEventPtr->mJobName);

    scrapeJobEventPtr->mUnRegisterMs = mUnRegisterMs;
    scrapeJobEventPtr->SetTimer(mTimer);
    // 1. add job to mJobSet
    WriteLock lock(mJobRWLock);
    mJobValidSet.insert(scrapeJobEventPtr->mJobName);
    mJobEventMap[scrapeJobEventPtr->mJobName] = scrapeJobEventPtr;

    // 2. build Ticker Event and add it to Timer
    ScrapeEvent scrapeEvent = BuildScrapeEvent(std::move(scrapeJobEventPtr),
                                               prometheus::RefeshIntervalSeconds,
                                               mJobRWLock,
                                               mJobValidSet,
                                               scrapeJobEventPtr->mJobName);
    mTimer->PushEvent(scrapeEvent);
}

void ScraperGroup::RemoveScrapeJob(const string& jobName) {
    WriteLock lock(mJobRWLock);
    mJobValidSet.erase(jobName);
    mJobEventMap.erase(jobName);
}

void ScraperGroup::Start() {
    {
        lock_guard<mutex> lock(mStartMux);
        if (mIsStarted) {
            return;
        }
        mIsStarted = true;
    }
    mThreadRes = std::async(launch::async, [this]() {
        mTimer->Start();
        // TODO(liqiang): init curl thread
    });
}

void ScraperGroup::Stop() {
    // 1. stop threads
    mTimer->Stop();

    // 2. clear resources
    {
        WriteLock lock(mJobRWLock);
        mJobValidSet.clear();
        mJobEventMap.clear();
    }
    {
        lock_guard<mutex> lock(mStartMux);
        if (!mIsStarted) {
            return;
        }
        mIsStarted = false;
    }
    future_status s = mThreadRes.wait_for(chrono::seconds(1));
    if (s == future_status::ready) {
        LOG_INFO(sLogger, ("scraper group", "stopped successfully"));
    } else {
        LOG_WARNING(sLogger, ("scraper group", "forced to stopped"));
    }
}

ScrapeEvent ScraperGroup::BuildScrapeEvent(std::shared_ptr<AsyncEvent> asyncEvent,
                                           uint64_t intervalSeconds,
                                           ReadWriteLock& rwLock,
                                           unordered_set<string>& validationSet,
                                           string hash) {
    ScrapeEvent scrapeEvent;
    uint64_t deadlineNanoSeconds = GetCurrentTimeInNanoSeconds();
    scrapeEvent.mDeadline = deadlineNanoSeconds;
    auto tickerEvent = TickerEvent(
        std::move(asyncEvent), intervalSeconds, deadlineNanoSeconds, mTimer, rwLock, validationSet, std::move(hash));
    auto asyncCallback = [&tickerEvent](const sdk::HttpMessage& response) { tickerEvent.Process(response); };
    scrapeEvent.mCallback = asyncCallback;
    return scrapeEvent;
}

} // namespace logtail
