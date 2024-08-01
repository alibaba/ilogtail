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

#include "ScrapeJobEvent.h"

#include <curl/curl.h>
#include <xxhash/xxhash.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/AsyncEvent.h"
#include "prometheus/Constants.h"
#include "prometheus/ScrapeWorkEvent.h"
#include "sdk/Common.h"

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

ScrapeJobEvent::ScrapeJobEvent() : mServicePort(0), mQueueKey(0), mInputIndex(0), mUnRegisterMs(0) {
}
bool ScrapeJobEvent::Init(const Json::Value& scrapeConfig) {
    mScrapeConfigPtr = std::make_shared<ScrapeConfig>();
    if (!mScrapeConfigPtr->Init(scrapeConfig)) {
        return false;
    }
    mJobName = mScrapeConfigPtr->mJobName;

    return true;
}

bool ScrapeJobEvent::operator<(const ScrapeJobEvent& other) const {
    return mJobName < other.mJobName;
}

void ScrapeJobEvent::Process(const sdk::HttpMessage& response) {
    if (response.statusCode != 200) {
        return;
    }
    const string& content = response.content;
    set<ScrapeWorkEvent> newScrapeWorkSet;
    if (!ParseTargetGroups(content, newScrapeWorkSet)) {
        return;
    }

    set<ScrapeWorkEvent> diff;
    {
        WriteLock lock(mRWLock);

        // remove obsolete scrape work
        set_difference(mScrapeWorkSet.begin(),
                       mScrapeWorkSet.end(),
                       newScrapeWorkSet.begin(),
                       newScrapeWorkSet.end(),
                       inserter(diff, diff.begin()));
        for (const auto& work : diff) {
            mScrapeWorkSet.erase(work);
            mValidSet.erase(work.mHash);
        }
        diff.clear();

        // save new scrape work
        set_difference(newScrapeWorkSet.begin(),
                       newScrapeWorkSet.end(),
                       mScrapeWorkSet.begin(),
                       mScrapeWorkSet.end(),
                       inserter(diff, diff.begin()));
        for (const auto& work : diff) {
            mScrapeWorkSet.insert(work);
            mValidSet.insert(work.mHash);
        }
    }

    // create new scrape event
    if (mTimer) {
        for (auto work : diff) {
            auto event = BuildScrapeEvent(make_shared<ScrapeWorkEvent>(work),
                                          mScrapeConfigPtr->mScrapeIntervalSeconds,
                                          mRWLock,
                                          mValidSet,
                                          work.mHash);
            mTimer->PushEvent(event);
        }
    }
}

ScrapeEvent ScrapeJobEvent::BuildScrapeEvent(std::shared_ptr<AsyncEvent> asyncEvent,
                                             uint64_t intervalSeconds,
                                             ReadWriteLock& rwLock,
                                             unordered_set<string>& validSet,
                                             string hash) {
    ScrapeEvent scrapeEvent;
    uint64_t deadlineNanoSeconds = GetCurrentTimeInNanoSeconds() + GetRandSleep(hash);
    scrapeEvent.mDeadline = deadlineNanoSeconds;
    auto tickerEvent = TickerEvent(
        std::move(asyncEvent), intervalSeconds, deadlineNanoSeconds, mTimer, rwLock, validSet, std::move(hash));
    auto asyncCallback = [&tickerEvent](const sdk::HttpMessage& response) { tickerEvent.Process(response); };
    scrapeEvent.mCallback = asyncCallback;
    return scrapeEvent;
}

uint64_t ScrapeJobEvent::GetRandSleep(const string& hash) const {
    const string& key = hash;
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

sdk::AsynRequest ScrapeJobEvent::BuildAsyncRequest() const {
    map<string, string> httpHeader;
    httpHeader[prometheus::ACCEPT] = prometheus::APPLICATION_JSON;
    httpHeader[prometheus::X_PROMETHEUS_REFRESH_INTERVAL_SECONDS] = ToString(prometheus::RefeshIntervalSeconds);
    httpHeader[prometheus::USER_AGENT] = prometheus::PROMETHEUS_PREFIX + mPodName;
    auto* response = new sdk::Response();
    auto* closure = new sdk::PostLogStoreLogsResponse;

    return sdk::AsynRequest(sdk::HTTP_GET,
                            mServiceHost,
                            mServicePort,
                            "/jobs/" + URLEncode(mJobName) + "/targets",
                            "collector_id=" + mPodName,
                            httpHeader,
                            "",
                            prometheus::RefeshIntervalSeconds,
                            "",
                            mScrapeConfigPtr->mScheme == prometheus::HTTPS,
                            (sdk::LogsClosure*)closure,
                            response);
}

bool ScrapeJobEvent::ParseTargetGroups(const string& content, set<ScrapeWorkEvent> newScrapeWorkSet) const {
    string errs;
    Json::Value root;
    if (!ParseJsonTable(content, root, errs) || !root.isArray()) {
        LOG_ERROR(sLogger,
                  ("http service discovery from operator failed", "Failed to parse JSON: " + errs)("job", mJobName));
        return false;
    }
    for (const auto& element : root) {
        if (!element.isObject()) {
            LOG_ERROR(
                sLogger,
                ("http service discovery from operator failed", "Invalid target group item found")("job", mJobName));
            return false;
        }

        // Parse targets
        vector<string> targets;
        if (element.isMember(prometheus::TARGETS) && element[prometheus::TARGETS].isArray()) {
            for (const auto& target : element[prometheus::TARGETS]) {
                if (target.isString()) {
                    targets.push_back(target.asString());
                } else {
                    LOG_ERROR(
                        sLogger,
                        ("http service discovery from operator failed", "Invalid target item found")("job", mJobName));
                    return false;
                }
            }
        }

        // Parse labels
        Labels labels;
        labels.Push(Label{prometheus::JOB, mJobName});
        labels.Push(Label{prometheus::ADDRESS_LABEL_NAME, targets[0]});
        labels.Push(Label{prometheus::SCHEME_LABEL_NAME, mScrapeConfigPtr->mScheme});
        labels.Push(Label{prometheus::METRICS_PATH_LABEL_NAME, mScrapeConfigPtr->mMetricsPath});
        labels.Push(Label{prometheus::SCRAPE_INTERVAL_LABEL_NAME,
                          (mScrapeConfigPtr->mScrapeIntervalSeconds % 60 == 0)
                              ? ToString(mScrapeConfigPtr->mScrapeIntervalSeconds / 60) + "m"
                              : ToString(mScrapeConfigPtr->mScrapeIntervalSeconds) + "s"});
        labels.Push(Label{prometheus::SCRAPE_TIMEOUT_LABEL_NAME,
                          (mScrapeConfigPtr->mScrapeTimeoutSeconds % 60 == 0)
                              ? ToString(mScrapeConfigPtr->mScrapeTimeoutSeconds / 60) + "m"
                              : ToString(mScrapeConfigPtr->mScrapeTimeoutSeconds) + "s"});
        for (const auto& pair : mScrapeConfigPtr->mParams) {
            labels.Push(Label{prometheus::PARAM_LABEL_NAME + pair.first, pair.second[0]});
        }

        if (element.isMember(prometheus::LABELS) && element[prometheus::LABELS].isObject()) {
            for (const auto& labelKey : element[prometheus::LABELS].getMemberNames()) {
                labels.Push(Label{labelKey, element[prometheus::LABELS][labelKey].asString()});
            }
        }

        // Relabel Config
        Labels result = Labels();
        bool keep = prometheus::Process(labels, mScrapeConfigPtr->mRelabelConfigs, result);
        if (!keep) {
            continue;
        }
        result.RemoveMetaLabels();
        if (result.Size() == 0) {
            continue;
        }

        auto scrapeTarget = ScrapeTarget(result);
        auto scrapeWorkEvent = ScrapeWorkEvent(mScrapeConfigPtr, scrapeTarget, mQueueKey, mInputIndex);

        newScrapeWorkSet.insert(scrapeWorkEvent);
    }
    return true;
}

void ScrapeJobEvent::SetTimer(shared_ptr<Timer> timer) {
    mTimer = std::move(timer);
}

} // namespace logtail
