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

#include "prometheus/ScrapeScheduler.h"

#include <xxhash/xxhash.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>

#include "Common.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScrapeTarget.h"
#include "queue/FeedbackQueueKey.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"

using namespace std;

namespace logtail {

ScrapeScheduler::ScrapeScheduler(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
                                 const ScrapeTarget& scrapeTarget,
                                 QueueKey queueKey,
                                 size_t inputIndex)
    : mScrapeConfigPtr(std::move(scrapeConfigPtr)),
      mScrapeTarget(scrapeTarget),
      mQueueKey(queueKey),
      mInputIndex(inputIndex) {
    string tmpTargetURL = mScrapeConfigPtr->mScheme + "://" + mScrapeTarget.mHost + ":" + ToString(mScrapeTarget.mPort)
        + mScrapeConfigPtr->mMetricsPath
        + (mScrapeConfigPtr->mQueryString.empty() ? "" : "?" + mScrapeConfigPtr->mQueryString);
    mHash = mScrapeConfigPtr->mJobName + tmpTargetURL + ToString(mScrapeTarget.mLabels.Hash());
}

bool ScrapeScheduler::operator<(const ScrapeScheduler& other) const {
    return mHash < other.mHash;
}

void ScrapeScheduler::Process(const HttpResponse& response) {
    // TODO(liqiang): get scrape timestamp
    time_t timestampInNs = GetCurrentTimeInNanoSeconds();
    if (response.mStatusCode != 200) {
        string headerStr;
        for (const auto& [k, v] : mScrapeConfigPtr->mHeaders) {
            headerStr.append(k).append(":").append(v).append(";");
        }
        LOG_WARNING(sLogger,
                    ("scrape failed, status code", response.mStatusCode)("target", mHash)("http header", headerStr));
        return;
    }
    // TODO(liqiang): set jobName, instance metadata
    auto eventGroup = SplitByLines(response.mBody, timestampInNs);
    // 自监控
    // work target 的 label 同步传输到 processor 中

    PushEventGroup(std::move(eventGroup));
}
PipelineEventGroup ScrapeScheduler::SplitByLines(const std::string& content, time_t timestamp) {
    PipelineEventGroup eGroup(std::make_shared<SourceBuffer>());

    for (const auto& line : SplitString(content, "\r\n")) {
        auto newLine = TrimString(line);
        if (newLine.empty() || newLine[0] == '#') {
            continue;
        }
        auto* logEvent = eGroup.AddLogEvent();
        logEvent->SetContent(prometheus::PROMETHEUS, newLine);
        logEvent->SetTimestamp(timestamp);
    }

    return eGroup;
}

void ScrapeScheduler::PushEventGroup(PipelineEventGroup&& eGroup) {
    auto item = make_unique<ProcessQueueItem>(std::move(eGroup), mInputIndex);
#ifdef APSARA_UNIT_TEST_MAIN
    mItem.push_back(std::move(item));
#endif
    ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item));
}

string ScrapeScheduler::GetId() const {
    return mHash;
}

void ScrapeScheduler::ScheduleNext() {
    auto future = std::make_shared<PromTaskFuture>();
    future->AddDoneCallback([this](const HttpResponse& response) {
        this->Process(response);
        this->ScheduleNext();
        this->mExecCount++;
    });
    mFuture = future;
    auto execTime = mFirstExecTime + std::chrono::seconds(mExecCount * prometheus::RefeshIntervalSeconds);

    auto event = BuildWorkTimerEvent(execTime);
    mTimer->PushEvent(std::move(event));
}

std::unique_ptr<TimerEvent> ScrapeScheduler::BuildWorkTimerEvent(std::chrono::steady_clock::time_point execTime) {
    auto request = std::make_unique<PromHttpRequest>(sdk::HTTP_GET,
                                                     mScrapeConfigPtr->mScheme == prometheus::HTTPS,
                                                     mScrapeTarget.mHost,
                                                     mScrapeTarget.mPort,
                                                     mScrapeConfigPtr->mMetricsPath,
                                                     mScrapeConfigPtr->mQueryString,
                                                     mScrapeConfigPtr->mHeaders,
                                                     "",
                                                     this->mFuture,
                                                     mScrapeConfigPtr->mScrapeIntervalSeconds,
                                                     execTime,
                                                     mTimer);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));
    return timerEvent;
}

uint64_t ScrapeScheduler::GetRandSleep() const {
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

void ScrapeScheduler::SetTimer(std::shared_ptr<Timer> timer) {
    mTimer = std::move(timer);
}
} // namespace logtail
