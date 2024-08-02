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

#include "prometheus/ScrapeWorkEvent.h"

#include <xxhash/xxhash.h>

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "TimeUtil.h"
#include "common/StringTools.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/ScrapeTarget.h"
#include "queue/FeedbackQueueKey.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"

using namespace std;

namespace logtail {

ScrapeWorkEvent::ScrapeWorkEvent(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
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

bool ScrapeWorkEvent::operator<(const ScrapeWorkEvent& other) const {
    return mHash < other.mHash;
}

void ScrapeWorkEvent::Process(const sdk::HttpMessage& response) {
    // TODO(liqiang): get scrape timestamp
    time_t timestampInNs = GetCurrentTimeInNanoSeconds();
    if (response.statusCode != 200) {
        string headerStr;
        for (const auto& [k, v] : mScrapeConfigPtr->mHeaders) {
            headerStr.append(k).append(":").append(v).append(";");
        }
        LOG_WARNING(sLogger,
                    ("scrape failed, status code", response.statusCode)("target", mHash)("http header", headerStr));
        return;
    }
    // TODO(liqiang): set jobName, instance metadata
    auto eventGroup = SplitByLines(response.content, timestampInNs);

    PushEventGroup(std::move(eventGroup));
}
PipelineEventGroup ScrapeWorkEvent::SplitByLines(const std::string& content, time_t timestamp) {
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

sdk::AsynRequest ScrapeWorkEvent::BuildAsyncRequest() const {
    map<string, string> httpHeader;
    string reqBody;

    auto* response = new sdk::Response();
    auto* closure = new sdk::PostLogStoreLogsResponse;

    return sdk::AsynRequest(sdk::HTTP_GET,
                            mScrapeTarget.mHost,
                            mScrapeTarget.mPort,
                            mScrapeConfigPtr->mMetricsPath,
                            mScrapeConfigPtr->mQueryString,
                            mScrapeConfigPtr->mHeaders,
                            "",
                            mScrapeConfigPtr->mScrapeTimeoutSeconds,
                            "",
                            mScrapeConfigPtr->mScheme == prometheus::HTTPS,
                            (sdk::LogsClosure*)closure,
                            response);
}

void ScrapeWorkEvent::PushEventGroup(PipelineEventGroup&& eGroup) {
    auto item = make_unique<ProcessQueueItem>(std::move(eGroup), mInputIndex);
    ProcessQueueManager::GetInstance()->PushQueue(mQueueKey, std::move(item));
}

} // namespace logtail
