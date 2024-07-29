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

#include "prometheus/ScrapeWork.h"

#include <xxhash/xxhash.h>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScrapeTarget.h"
#include "prometheus/TextParser.h"
#include "queue/FeedbackQueueKey.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

ScrapeWork::ScrapeWork(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
                       const ScrapeTarget& scrapeTarget,
                       QueueKey queueKey,
                       size_t inputIndex)
    : mScrapeConfigPtr(std::move(scrapeConfigPtr)),
      mScrapeTarget(scrapeTarget),
      mQueueKey(queueKey),
      mInputIndex(inputIndex),
      mFinished(true),
      mUnRegisterMs(0) {
    mClient = make_unique<sdk::CurlClient>();

    string tmpTargetURL = mScrapeConfigPtr->mScheme + "://" + mScrapeTarget.mHost + ":" + ToString(mScrapeTarget.mPort)
        + mScrapeConfigPtr->mMetricsPath
        + (mScrapeConfigPtr->mQueryString.empty() ? "" : "?" + mScrapeConfigPtr->mQueryString);
    mHash = mScrapeConfigPtr->mJobName + tmpTargetURL + ToString(mScrapeTarget.mLabels.Hash());
}

ScrapeWork::~ScrapeWork() {
    mFinished.store(true);
    mScrapeConfigPtr.reset();
    mClient.reset();
    mScrapeLoopThread.reset();
}

bool ScrapeWork::operator<(const ScrapeWork& other) const {
    return mHash < other.mHash;
}

void ScrapeWork::StartScrapeLoop() {
    mFinished.store(false);
    // sync thread
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { ScrapeLoop(); });
    }
}

void ScrapeWork::StopScrapeLoop() {
    mFinished.store(true);
    mScrapeLoopThread.reset();
}

/// @brief scrape target loop
void ScrapeWork::ScrapeLoop() {
    LOG_INFO(sLogger, ("start prometheus scrape loop", mHash));
    uint64_t randSleep = GetRandSleep();
    // zero-cost upgrade
    // if mUnRegisterMs is expired, scrape and push immediately
    if (mUnRegisterMs != 0
        && (randSleep + GetCurrentTimeInNanoSeconds() > mUnRegisterMs * 1000ULL * 1000ULL
                + (uint64_t)mScrapeConfigPtr->mScrapeIntervalSeconds * 1000ULL * 1000ULL * 1000ULL)) {
        ScrapeAndPush();
        randSleep = GetRandSleep();
    }

    this_thread::sleep_for(chrono::nanoseconds(randSleep));

    while (!mFinished.load()) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();

        ScrapeAndPush();

        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = (uint64_t)mScrapeConfigPtr->mScrapeIntervalSeconds * 1000ULL * 1000ULL * 1000ULL
            - elapsedTime % ((uint64_t)mScrapeConfigPtr->mScrapeIntervalSeconds * 1000ULL * 1000ULL * 1000ULL);
        this_thread::sleep_for(chrono::nanoseconds(timeToWait));
    }
}

void ScrapeWork::ScrapeAndPush() {
    time_t defaultTs = time(nullptr);

    // scrape target by CurlClient
    auto httpResponse = Scrape();
    if (httpResponse.statusCode == 200) {
        auto parser = TextParser();
        auto eventGroup
            = parser.Parse(httpResponse.content, defaultTs, mScrapeConfigPtr->mJobName, mScrapeTarget.mHost);

        PushEventGroup(std::move(eventGroup));
    } else {
        string headerStr;
        for (const auto& [k, v] : mScrapeConfigPtr->mHeaders) {
            headerStr.append(k).append(":").append(v).append(";");
        }
        LOG_WARNING(sLogger,
                    ("scrape failed, status code", httpResponse.statusCode)("target", mHash)("http header", headerStr));
    }
}

uint64_t ScrapeWork::GetRandSleep() {
    const string& key = mHash;
    uint64_t h = XXH64(key.c_str(), key.length(), 0);
    uint64_t randSleep
        = ((double)1.0) * mScrapeConfigPtr->mScrapeIntervalSeconds * (1.0 * h / (double)0xFFFFFFFFFFFFFFFF);
    uint64_t sleepOffset
        = GetCurrentTimeInNanoSeconds() % (mScrapeConfigPtr->mScrapeIntervalSeconds * 1000ULL * 1000ULL * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += mScrapeConfigPtr->mScrapeIntervalSeconds * 1000ULL * 1000ULL * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

inline sdk::HttpMessage ScrapeWork::Scrape() {
    map<string, string> httpHeader;
    string reqBody;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusScrapeWork";

    try {
        mClient->Send(sdk::HTTP_GET,
                      mScrapeTarget.mHost,
                      mScrapeTarget.mPort,
                      mScrapeConfigPtr->mMetricsPath,
                      mScrapeConfigPtr->mQueryString,
                      mScrapeConfigPtr->mHeaders,
                      reqBody,
                      mScrapeConfigPtr->mScrapeTimeoutSeconds,
                      httpResponse,
                      "",
                      mScrapeConfigPtr->mScheme == prometheus::HTTPS);
    } catch (const sdk::LOGException& e) {
        LOG_WARNING(sLogger, ("scrape failed", e.GetMessage())("errCode", e.GetErrorCode())("target", mHash));
    }
    return httpResponse;
}

void ScrapeWork::PushEventGroup(PipelineEventGroup&& eGroup) {
    auto item = make_unique<ProcessQueueItem>(std::move(eGroup), mInputIndex);
    for (size_t i = 0; i < 1000; ++i) {
        if (ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item)) == 0) {
            return;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    LOG_INFO(sLogger, ("push event group failed", mHash));
}

void ScrapeWork::SetUnRegisterMs(uint64_t unRegisterMs) {
    mUnRegisterMs = unRegisterMs;
}

} // namespace logtail
