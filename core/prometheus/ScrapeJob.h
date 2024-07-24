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

#pragma once

#include <json/json.h>

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "ScrapeConfig.h"
#include "ScrapeTarget.h"
#include "ScrapeWork.h"


namespace logtail {

extern const int sRefeshIntervalSeconds;

class ScrapeJob {
public:
    ScrapeJob();

    bool Init(const Json::Value& scrapeConfig);

    bool operator<(const ScrapeJob& other) const;

    void StartTargetsDiscoverLoop();
    void StopTargetsDiscoverLoop();

    std::unordered_map<std::string, ScrapeTarget> GetScrapeTargetsMapCopy();

    std::string mJobName;
    std::shared_ptr<ScrapeConfig> mScrapeConfigPtr;

    // from environment variable
    std::string mOperatorHost;
    uint32_t mOperatorPort;
    std::string mPodName;

    // from pipeline context
    QueueKey mQueueKey;
    size_t mInputIndex;

#ifdef APSARA_UNIT_TEST_MAIN
    void AddScrapeTarget(std::string hash, ScrapeTarget target) {
        std::lock_guard<std::mutex> lock(mMutex);
        mScrapeTargetsMap[hash] = target;
    }
#endif

private:
    std::mutex mMutex;
    std::unordered_map<std::string, ScrapeTarget> mScrapeTargetsMap;

    std::atomic<bool> mFinished;
    ThreadPtr mTargetsDiscoveryLoopThread;

    std::unique_ptr<sdk::HTTPClient> mClient;

    bool Validation() const;

    void TargetsDiscoveryLoop();

    bool FetchHttpData(std::string& readBuffer) const;
    bool ParseTargetGroups(const std::string& response,
                           std::unordered_map<std::string, ScrapeTarget>& newScrapeTargetsMap) const;
    int GetIntSeconds(const std::string& str) const;
    std::string ConvertMapParamsToQueryString() const;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeJobUnittest;
    friend class PrometheusInputRunnerUnittest;
    friend class ScraperGroupUnittest;
#endif
};

} // namespace logtail