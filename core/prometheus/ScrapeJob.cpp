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

#include "ScrapeJob.h"

#include <curl/curl.h>

#include <cstdlib>
#include <memory>
#include <string>

#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

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

ScrapeJob::ScrapeJob() : mServicePort(0), mQueueKey(0), mInputIndex(0) {
    mFinished.store(true);
    mClient = make_unique<sdk::CurlClient>();
}

bool ScrapeJob::Init(const Json::Value& scrapeConfig) {
    mScrapeConfigPtr = std::make_shared<ScrapeConfig>();
    if (!mScrapeConfigPtr->Init(scrapeConfig)) {
        return false;
    }
    mJobName = mScrapeConfigPtr->mJobName;

    return true;
}

bool ScrapeJob::operator<(const ScrapeJob& other) const {
    return mJobName < other.mJobName;
}

void ScrapeJob::StartTargetsDiscoverLoop() {
    mFinished.store(false);
    if (!mTargetsDiscoveryLoopThread) {
        mTargetsDiscoveryLoopThread = CreateThread([this]() { TargetsDiscoveryLoop(); });
    }
}

void ScrapeJob::StopTargetsDiscoverLoop() {
    mFinished.store(true);
    mTargetsDiscoveryLoopThread.reset();

    std::lock_guard<std::mutex> lock(mMutex);
    mScrapeTargetsMap.clear();
}

unordered_map<string, ScrapeTarget> ScrapeJob::GetScrapeTargetsMapCopy() {
    lock_guard<mutex> lock(mMutex);

    unordered_map<string, ScrapeTarget> copy;

    for (const auto& [targetHash, targetPtr] : mScrapeTargetsMap) {
        copy.emplace(targetHash, targetPtr);
    }

    return copy;
}

void ScrapeJob::TargetsDiscoveryLoop() {
    uint64_t nextTargetsDiscoveryLoopTime = GetCurrentTimeInNanoSeconds();
    while (!mFinished.load()) {
        uint64_t timeNow = GetCurrentTimeInNanoSeconds();
        if (timeNow < nextTargetsDiscoveryLoopTime) {
            this_thread::sleep_for(chrono::nanoseconds(nextTargetsDiscoveryLoopTime - timeNow));
        }
        nextTargetsDiscoveryLoopTime
            = GetCurrentTimeInNanoSeconds() + prometheus::RefeshIntervalSeconds * 1000ULL * 1000ULL * 1000ULL;

        string readBuffer;
        if (!FetchHttpData(readBuffer)) {
            continue;
        }
        unordered_map<string, ScrapeTarget> newScrapeTargetsMap;
        if (!ParseTargetGroups(readBuffer, newScrapeTargetsMap)) {
            continue;
        }
        lock_guard<mutex> lock(mMutex);
        mScrapeTargetsMap = std::move(newScrapeTargetsMap);
    }
}

bool ScrapeJob::FetchHttpData(string& readBuffer) const {
    map<string, string> httpHeader;
    httpHeader[prometheus::ACCEPT] = prometheus::APPLICATION_JSON;
    httpHeader[prometheus::X_PROMETHEUS_REFRESH_INTERVAL_SECONDS] = ToString(prometheus::RefeshIntervalSeconds);
    httpHeader[prometheus::USER_AGENT] = prometheus::PROMETHEUS_PREFIX + mPodName;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusTargetsDiscover";

    bool httpsFlag = mScrapeConfigPtr->mScheme == prometheus::HTTPS;
    try {
        mClient->Send(sdk::HTTP_GET,
                      mServiceHost,
                      mServicePort,
                      "/jobs/" + URLEncode(mJobName) + "/targets",
                      "collector_id=" + mPodName,
                      httpHeader,
                      "",
                      prometheus::RefeshIntervalSeconds,
                      httpResponse,
                      "",
                      httpsFlag);
    } catch (const sdk::LOGException& e) {
        LOG_WARNING(sLogger,
                    ("http service discovery from operator failed", e.GetMessage())("errCode",
                                                                                    e.GetErrorCode())("job", mJobName));
        return false;
    }
    readBuffer = httpResponse.content;
    return true;
}

bool ScrapeJob::ParseTargetGroups(const string& response,
                                  unordered_map<string, ScrapeTarget>& newScrapeTargetsMap) const {
    Json::CharReaderBuilder readerBuilder;
    string errs;
    Json::Value root;
    istringstream s(response);
    if (!ParseJsonTable(response, root, errs)) {
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

        newScrapeTargetsMap[scrapeTarget.GetHash()] = scrapeTarget;
    }
    return true;
}

} // namespace logtail
