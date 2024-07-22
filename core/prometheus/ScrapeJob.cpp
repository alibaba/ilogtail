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

#include <memory>
#include <string>

#include "Common.h"
#include "CurlImp.h"
#include "Exception.h"
#include "ScrapeWork.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {

const int sRefeshIntervalSeconds = 5;

string url_encode(const string& value) {
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

ScrapeJob::ScrapeJob() {
    mClient = make_unique<sdk::CurlClient>();
}

/// @brief Construct from json config
bool ScrapeJob::Init(const Json::Value& scrapeConfig) {
    mScrapeConfigPtr = std::make_shared<ScrapeConfig>();
    if (!mScrapeConfigPtr->Init(scrapeConfig)) {
        return false;
    }
    mJobName = mScrapeConfigPtr->mJobName;

    return Validation();
}


// TODO: completely validate logic
bool ScrapeJob::Validation() const {
    return !mJobName.empty();
}

/// @brief JobName must be unique
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
    mTargetsDiscoveryLoopThread->Wait(0ULL);
    mTargetsDiscoveryLoopThread.reset();
    mScrapeTargetsMap.clear();
}

unordered_map<string, ScrapeTarget> ScrapeJob::GetScrapeTargetsMapCopy() {
    lock_guard<mutex> lock(mMutex);

    unordered_map<string, ScrapeTarget> copy;

    for (const auto& [targetHash, targetPtr] : mScrapeTargetsMap) {
        copy.emplace(targetHash, *targetPtr);
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
            = GetCurrentTimeInNanoSeconds() + sRefeshIntervalSeconds * 1000ULL * 1000ULL * 1000ULL;

        string readBuffer;
        if (!FetchHttpData(readBuffer)) {
            continue;
        }
        unordered_map<string, shared_ptr<ScrapeTarget>> newScrapeTargetsMap;
        if (!ParseTargetGroups(readBuffer, newScrapeTargetsMap)) {
            continue;
        }
        mScrapeTargetsMap = std::move(newScrapeTargetsMap);
    }
}

bool ScrapeJob::FetchHttpData(string& readBuffer) const {
    map<string, string> httpHeader;
    httpHeader["Accept"] = "application/json";
    httpHeader["X-Prometheus-Refresh-Interval-Seconds"] = ToString(sRefeshIntervalSeconds);
    httpHeader["User-Agent"] = "matrix_prometheus_" + ToString(getenv("POD_NAME"));
    sdk::HttpMessage httpResponse;
    // TODO: 等待框架删除对respond返回头的 X_LOG_REQUEST_ID 检查
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusTargetsDiscover";

    bool httpsFlag = mScrapeConfigPtr->mScheme == "https";
    try {
        mClient->Send(sdk::HTTP_GET,
                      mOperatorHost,
                      mOperatorPort,
                      "/jobs/" + url_encode(mJobName) + "/targets",
                      "collector_id=" + mPodName,
                      httpHeader,
                      "",
                      sRefeshIntervalSeconds,
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
                                  unordered_map<string, shared_ptr<ScrapeTarget>>& newScrapeTargetsMap) const {
    Json::CharReaderBuilder readerBuilder;
    JSONCPP_STRING errs;
    Json::Value root;
    istringstream s(response);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
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
        if (element.isMember("targets") && element["targets"].isArray()) {
            for (const auto& target : element["targets"]) {
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
        labels.Push(Label{"job", mJobName});
        labels.Push(Label{"__address__", targets[0]});
        labels.Push(Label{"__scheme__", mScrapeConfigPtr->mScheme});
        labels.Push(Label{"__metrics_path__", mScrapeConfigPtr->mMetricsPath});
        labels.Push(Label{"__scrape_interval__",
                          (mScrapeConfigPtr->mScrapeInterval % 60 == 0)
                              ? ToString(mScrapeConfigPtr->mScrapeInterval / 60) + "m"
                              : ToString(mScrapeConfigPtr->mScrapeInterval) + "s"});
        labels.Push(Label{"__scrape_timeout__",
                          (mScrapeConfigPtr->mScrapeTimeout % 60 == 0)
                              ? ToString(mScrapeConfigPtr->mScrapeTimeout / 60) + "m"
                              : ToString(mScrapeConfigPtr->mScrapeTimeout) + "s"});
        for (const auto& pair : mScrapeConfigPtr->mParams) {
            labels.Push(Label{"__param_" + pair.first, pair.second[0]});
        }

        if (element.isMember("labels") && element["labels"].isObject()) {
            for (const auto& labelKey : element["labels"].getMemberNames()) {
                labels.Push(Label{labelKey, element["labels"][labelKey].asString()});
            }
        }

        // Relabel Config
        std::shared_ptr<Labels> resultPtr = shared_ptr<Labels>();
        bool keep = relabel::Process(labels, mScrapeConfigPtr->mRelabelConfigs, *resultPtr);
        if (!keep) {
            continue;
        }
        resultPtr->RemoveMetaLabels();
        if (resultPtr->Size() == 0) {
            continue;
        }
        LOG_INFO(sLogger, ("target relabel keep", mJobName));

        auto scrapeTargetPtr = std::make_shared<ScrapeTarget>(mScrapeConfigPtr, resultPtr, mQueueKey, mInputIndex);

        newScrapeTargetsMap[scrapeTargetPtr->GetHash()] = scrapeTargetPtr;
    }
    return true;
}

// only support xxs, xxm
int ScrapeJob::GetIntSeconds(const string& str) const {
    int value = stoi(str.substr(0, str.size() - 1));
    if (str.back() == 's') {
        return value;
    } else if (str.back() == 'm') {
        return 60 * value;
    }
    return 30;
}

} // namespace logtail
