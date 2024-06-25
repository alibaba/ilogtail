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

#include "Scraper.h"

#include <stdint.h>

#include <atomic>
#include <string>

#include "boost/functional/hash.hpp"
#include "common/Lock.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {

ScraperGroup::ScraperGroup() : mScraperThread(nullptr) {
}

void ScraperGroup::UpdateScrapeWork(const string& jobName) {
    if (mScrapeJobMap.find(jobName) == mScrapeJobMap.end()) {
        LOG_WARNING(sLogger, ("Job not found", jobName));
        return;
    }
    const auto& sTMap = mScrapeJobMap[jobName]->GetScrapeTargetsMapCopy();
    auto& sWMap = mScrapeWorkMap[jobName];

    // stop obsolete targets
    vector<string> obsoleteTargets;
    for (const auto& pair : sWMap) {
        if (sTMap.find(pair.first) == sTMap.end()) {
            obsoleteTargets.push_back(pair.first);
        }
    }
    for (const auto& targetHash : obsoleteTargets) {
        sWMap[targetHash]->StopScrapeLoop();
        sWMap.erase(targetHash);
    }

    // start new targets
    for (const auto& pair : sTMap) {
        if (sWMap.find(pair.first) == sWMap.end()) {
            auto& scrapeTarget = *pair.second;
            sWMap[pair.first] = make_unique<ScrapeWork>(scrapeTarget);
            sWMap[pair.first]->StartScrapeLoop();
        }
    }
}

void ScraperGroup::UpdateScrapeJob(std::unique_ptr<ScrapeJob> scrapeJobPtr) {
    RemoveScrapeJob(scrapeJobPtr->mJobName);

    lock_guard<mutex> lock(mMutex);

    scrapeJobPtr->StartTargetsDiscoverLoop();
    mScrapeJobMap[scrapeJobPtr->mJobName] = move(scrapeJobPtr);
}

void ScraperGroup::RemoveScrapeJob(const string& jobName) {
    lock_guard<mutex> lock(mMutex);

    if (mScrapeJobMap.find(jobName) != mScrapeJobMap.end()) {
        mScrapeJobMap[jobName]->StopTargetsDiscoverLoop();
        for (auto& pair : mScrapeWorkMap[jobName]) {
            pair.second->StopScrapeLoop();
        }
        mScrapeWorkMap.erase(jobName);
        mScrapeJobMap.erase(jobName);
    }
}

void ScraperGroup::Start() {
    mFinished.store(false);
    mScraperThread = CreateThread([this]() { ProcessScrapeWorkUpdate(); });
}

void ScraperGroup::Stop() {
    mFinished.store(true);

    vector<string> jobsToRemove;
    {
        lock_guard<mutex> lock(mMutex);
        for (auto& iter : mScrapeJobMap) {
            jobsToRemove.push_back(iter.first);
        }
    }
    for (const auto& jobName : jobsToRemove) {
        RemoveScrapeJob(jobName);
    }

    if (mScraperThread) {
        mScraperThread->Wait(0ULL);
    }
    mScraperThread.reset();

    {
        lock_guard<mutex> lock(mMutex);
        mScrapeWorkMap.clear();
        mScrapeJobMap.clear();
    }
}

void ScraperGroup::ProcessScrapeWorkUpdate() {
    while (!mFinished.load()) {
        {
            lock_guard<mutex> lock(mMutex);

            for (auto& iter : mScrapeJobMap) {
                UpdateScrapeWork(iter.first);
            }
        }
        this_thread::sleep_for(chrono::seconds(sRefeshIntervalSeconds));
    }
}


} // namespace logtail
