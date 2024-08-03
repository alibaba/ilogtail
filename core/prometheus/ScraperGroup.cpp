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
#include <utility>

#include "Common.h"
#include "StringTools.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/Mock.h"
#include "prometheus/ScrapeWorkEvent.h"

using namespace std;

namespace logtail {

string URLEncode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        // Keep alphanumeric characters and other safe characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        // Any other characters are percent-encoded
        escaped << '%' << setw(2) << int((unsigned char)c);
    }

    return escaped.str();
}

ScraperGroup::ScraperGroup() : mServicePort(0), mUnRegisterMs(0), mTimer(make_shared<Timer>()), mIsStarted(false) {
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
    auto timerEvent = BuildJobTimerEvent(std::move(scrapeJobEventPtr), prometheus::RefeshIntervalSeconds);
    mTimer->PushEvent(std::move(timerEvent));
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

std::unique_ptr<TimerEvent> ScraperGroup::BuildJobTimerEvent(std::shared_ptr<ScrapeJobEvent> jobEvent,
                                                             uint64_t intervalSeconds) {
    map<string, string> httpHeader;
    httpHeader[prometheus::ACCEPT] = prometheus::APPLICATION_JSON;
    httpHeader[prometheus::X_PROMETHEUS_REFRESH_INTERVAL_SECONDS] = ToString(prometheus::RefeshIntervalSeconds);
    httpHeader[prometheus::USER_AGENT] = prometheus::PROMETHEUS_PREFIX + mPodName;
    auto execTime = std::chrono::steady_clock::now();
    auto request = std::make_unique<TickerHttpRequest>(sdk::HTTP_GET,
                                                       false,
                                                       mServiceHost,
                                                       mServicePort,
                                                       "/jobs/" + URLEncode(jobEvent->mJobName) + "/targets",
                                                       "collector_id=" + mPodName,
                                                       httpHeader,
                                                       "",
                                                       jobEvent->mJobName,
                                                       std::move(jobEvent),
                                                       mJobRWLock,
                                                       mJobValidSet,
                                                       intervalSeconds,
                                                       execTime,
                                                       mTimer);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));

    return timerEvent;
}

} // namespace logtail
