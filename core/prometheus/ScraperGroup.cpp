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

#include <memory>
#include <mutex>
#include <string>

#include "logger/Logger.h"
#include "prometheus/Mock.h"

using namespace std;

namespace logtail {


ScraperGroup::ScraperGroup() : mServicePort(0), mUnRegisterMs(0), mTimer(make_shared<Timer>()), mIsStarted(false) {
}

void ScraperGroup::UpdateScrapeJob(std::shared_ptr<TargetsSubscriber> scrapeJobEventPtr) {
    RemoveScrapeJob(scrapeJobEventPtr->GetId());

    scrapeJobEventPtr->mUnRegisterMs = mUnRegisterMs;
    scrapeJobEventPtr->SetTimer(mTimer);
    scrapeJobEventPtr->SetFirstExecTime(std::chrono::steady_clock::now());
    // 1. add job to mJobEventMap
    WriteLock lock(mJobRWLock);
    mJobEventMap[scrapeJobEventPtr->GetId()] = scrapeJobEventPtr;

    // 2. build Ticker Event and add it to Timer
    scrapeJobEventPtr->ScheduleNext();
}

void ScraperGroup::RemoveScrapeJob(const string& jobName) {
    WriteLock lock(mJobRWLock);
    mJobEventMap[jobName]->mFuture->Cancel();
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
        mTimer->Init();
        // TODO(liqiang): init curl thread
    });
}

void ScraperGroup::Stop() {
    // 1. stop threads
    mTimer->Stop();

    // 2. clear resources
    {
        WriteLock lock(mJobRWLock);
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

} // namespace logtail
