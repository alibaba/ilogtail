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
#include "prometheus/async//PromFuture.h"
#include "prometheus/async//PromHttpRequest.h"
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
    vector<Labels> targetGroup;
    if (!ParseScrapeSchedulerGroup(content, targetGroup)) {
        return;
    }
    set<shared_ptr<ScrapeScheduler>> newScrapeSchedulerSet = BuildScrapeSchedulerSet(targetGroup);
    UpdateScrapeScheduler(newScrapeSchedulerSet);
}

void TargetSubscriberScheduler::UpdateScrapeScheduler(set<shared_ptr<ScrapeScheduler>>& newScrapeSchedulerSet) {
    set<shared_ptr<ScrapeScheduler>> diff;
    {
        WriteLock lock(mRWLock);

        // remove obsolete scrape work
        set_difference(mScrapeSchedulerSet.begin(),
                       mScrapeSchedulerSet.end(),
                       newScrapeSchedulerSet.begin(),
                       newScrapeSchedulerSet.end(),
                       inserter(diff, diff.begin()));
        for (const auto& work : diff) {
            mScrapeSchedulerSet.erase(work);
            mScrapeMap[work->GetId()]->Cancel();
        }
        diff.clear();

        // save new scrape work
        set_difference(newScrapeSchedulerSet.begin(),
                       newScrapeSchedulerSet.end(),
                       mScrapeSchedulerSet.begin(),
                       mScrapeSchedulerSet.end(),
                       inserter(diff, diff.begin()));
        for (const auto& work : diff) {
            mScrapeSchedulerSet.insert(work);
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

bool TargetSubscriberScheduler::ParseScrapeSchedulerGroup(const std::string& content,
                                                          std::vector<Labels>& scrapeSchedulerGroup) {
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
        if (!targets.empty()) {
            labels.Push(Label{prometheus::ADDRESS_LABEL_NAME, targets[0]});
        }
        labels.Push(Label{prometheus::SCHEME_LABEL_NAME, mScrapeConfigPtr->mScheme});
        labels.Push(Label{prometheus::METRICS_PATH_LABEL_NAME, mScrapeConfigPtr->mMetricsPath});
        labels.Push(
            Label{prometheus::SCRAPE_INTERVAL_LABEL_NAME, SecondToDuration(mScrapeConfigPtr->mScrapeIntervalSeconds)});
        labels.Push(
            Label{prometheus::SCRAPE_TIMEOUT_LABEL_NAME, SecondToDuration(mScrapeConfigPtr->mScrapeTimeoutSeconds)});
        for (const auto& pair : mScrapeConfigPtr->mParams) {
            if (!pair.second.empty()) {
                labels.Push(Label{prometheus::PARAM_LABEL_NAME + pair.first, pair.second[0]});
            }
        }

        if (element.isMember(prometheus::LABELS) && element[prometheus::LABELS].isObject()) {
            for (const auto& labelKey : element[prometheus::LABELS].getMemberNames()) {
                labels.Push(Label{labelKey, element[prometheus::LABELS][labelKey].asString()});
            }
        }
        scrapeSchedulerGroup.push_back(labels);
    }
    return true;
}

std::set<std::shared_ptr<ScrapeScheduler>>
TargetSubscriberScheduler::BuildScrapeSchedulerSet(std::vector<Labels>& targetGroups) {
    set<shared_ptr<ScrapeScheduler>> scrapeSchedulerSet;
    for (const auto& labels : targetGroups) {
        // Relabel Config
        Labels resultLabel = Labels();
        bool keep = prometheus::Process(labels, mScrapeConfigPtr->mRelabelConfigs, resultLabel);
        if (!keep) {
            continue;
        }
        resultLabel.RemoveMetaLabels();
        if (resultLabel.Size() == 0) {
            continue;
        }

        string address = resultLabel.Get(prometheus::ADDRESS_LABEL_NAME);
        auto m = address.find(':');
        string host;
        int32_t port = 0;
        if (m != string::npos) {
            host = address.substr(0, m);
            try {
                port = stoi(address.substr(m + 1));
            } catch (...) {
                continue;
            }
        }

        auto scrapeScheduler
            = std::make_shared<ScrapeScheduler>(mScrapeConfigPtr, host, port, resultLabel, mQueueKey, mInputIndex);
        scrapeScheduler->SetFirstExecTime(std::chrono::steady_clock::now()
                                          + std::chrono::nanoseconds(scrapeScheduler->GetRandSleep()));

        scrapeScheduler->SetTimer(mTimer);
        scrapeSchedulerSet.insert(scrapeScheduler);
    }
    return scrapeSchedulerSet;
}

void TargetSubscriberScheduler::SetTimer(shared_ptr<Timer> timer) {
    mTimer = std::move(timer);
}

string TargetSubscriberScheduler::GetId() const {
    return mJobName;
}

void TargetSubscriberScheduler::ScheduleNext() {
    auto future = std::make_shared<PromFuture>();
    future->AddDoneCallback([this](const HttpResponse& response) {
        this->OnSubscription(response);
        ExecDone();
        this->ScheduleNext();
    });

    {
        WriteLock lock(mLock);
        if (mFuture && mFuture->IsCancelled()) {
            return;
        }
        mFuture = future;
    }

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
                                                     this->mFuture);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));

    return timerEvent;
}


} // namespace logtail
