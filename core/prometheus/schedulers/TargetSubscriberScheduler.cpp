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

#include "prometheus/schedulers/TargetSubscriberScheduler.h"

#include <xxhash/xxhash.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "Common.h"
#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "prometheus/Utils.h"
#include "prometheus/async//PromHttpRequest.h"
#include "prometheus/async//PromTaskFuture.h"
#include "prometheus/schedulers/ScrapeScheduler.h"

using namespace std;

namespace logtail {

TargetSubscriberScheduler::TargetSubscriberScheduler()
    : mQueueKey(0), mInputIndex(0), mServicePort(0), mUnRegisterMs(0) {
}

bool TargetSubscriberScheduler::Init(const Json::Value& scrapeConfig) {
    mScrapeConfigPtr = std::make_shared<ScrapeConfig>();
    if (!mScrapeConfigPtr->Init(scrapeConfig)) {
        return false;
    }
    mJobName = mScrapeConfigPtr->mJobName;
    mInterval = prometheus::RefeshIntervalSeconds;

    return true;
}

bool TargetSubscriberScheduler::operator<(const TargetSubscriberScheduler& other) const {
    return mJobName < other.mJobName;
}

void TargetSubscriberScheduler::OnSubscription(const HttpResponse& response) {
    if (response.mStatusCode != 200) {
        return;
    }
    const string& content = response.mBody;
    set<shared_ptr<ScrapeScheduler>> newScrapeWorkSet;
    if (!ParseTargetGroups(content, newScrapeWorkSet)) {
        return;
    }
    UpdateScrapeScheduler(newScrapeWorkSet);
}

void TargetSubscriberScheduler::UpdateScrapeScheduler(set<shared_ptr<ScrapeScheduler>>& newScrapeWorkSet) {
    set<shared_ptr<ScrapeScheduler>> diff;
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
            mScrapeMap[work->GetId()]->mFuture->Cancel();
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
            mScrapeMap[work->GetId()] = work;
        }
    }

    // create new scrape event
    if (mTimer) {
        for (const auto& work : diff) {
            work->ScheduleNext();
        }
    }
}


uint64_t TargetSubscriberScheduler::GetRandSleep(const string& hash) const {
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

// TODO: simplify method
bool TargetSubscriberScheduler::ParseTargetGroups(const string& content,
                                                  std::set<std::shared_ptr<ScrapeScheduler>>& newScrapeWorkSet) const {
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
        // 产生的labels 放到 sample 中
        bool keep = prometheus::Process(labels, mScrapeConfigPtr->mRelabelConfigs, result);
        if (!keep) {
            continue;
        }
        result.RemoveMetaLabels();
        if (result.Size() == 0) {
            continue;
        }

        auto scrapeTarget = ScrapeTarget(result);
        auto scrapeWorkEvent
            = std::make_shared<ScrapeScheduler>(mScrapeConfigPtr, scrapeTarget, mQueueKey, mInputIndex);
        scrapeWorkEvent->SetFirstExecTime(std::chrono::steady_clock::now()
                                          + std::chrono::nanoseconds(scrapeWorkEvent->GetRandSleep()));

        scrapeWorkEvent->SetTimer(mTimer);
        newScrapeWorkSet.insert(scrapeWorkEvent);
    }
    return true;
}

void TargetSubscriberScheduler::SetTimer(shared_ptr<Timer> timer) {
    mTimer = std::move(timer);
}

string TargetSubscriberScheduler::GetId() const {
    return mJobName;
}

void TargetSubscriberScheduler::ScheduleNext() {
    auto future = std::make_shared<PromTaskFuture>();
    future->AddDoneCallback([this](const HttpResponse& response) {
        this->OnSubscription(response);
        this->ScheduleNext();
        this->mExecCount++;
    });
    mFuture = future;

    auto event = BuildSubscriberTimerEvent(GetNextExecTime());
    mTimer->PushEvent(std::move(event));
}

std::unique_ptr<TimerEvent>
TargetSubscriberScheduler::BuildSubscriberTimerEvent(std::chrono::steady_clock::time_point execTime) {
    map<string, string> httpHeader;
    httpHeader[prometheus::ACCEPT] = prometheus::APPLICATION_JSON;
    httpHeader[prometheus::X_PROMETHEUS_REFRESH_INTERVAL_SECONDS] = ToString(prometheus::RefeshIntervalSeconds);
    httpHeader[prometheus::USER_AGENT] = prometheus::PROMETHEUS_PREFIX + mPodName;
    auto request = std::make_unique<PromHttpRequest>(sdk::HTTP_GET,
                                                     false,
                                                     mServiceHost,
                                                     mServicePort,
                                                     "/jobs/" + URLEncode(GetId()) + "/targets",
                                                     "collector_id=" + mPodName,
                                                     httpHeader,
                                                     "",
                                                     this->mFuture,
                                                     mInterval,
                                                     execTime,
                                                     mTimer);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));

    return timerEvent;
}


} // namespace logtail
