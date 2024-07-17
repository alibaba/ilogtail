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

#include <mutex>
#include <string>
#include <unordered_map>

#include "Relabel.h"
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

    std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>> GetScrapeTargetsMapCopy();

    std::string mJobName;
    std::string mScheme;
    std::string mMetricsPath;
    std::string mScrapeIntervalString;
    std::string mScrapeTimeoutString;
    std::map<std::string, std::vector<std::string>> mParams;

    std::string mOperatorHost;
    uint32_t mOperatorPort;
    std::string mPodName;

    QueueKey mQueueKey;
    size_t mInputIndex;

#ifdef APSARA_UNIT_TEST_MAIN
    ScrapeJob(std::string jobName, std::string metricsPath, std::string scheme, int interval, int timeout)
        : mJobName(jobName),
          mScheme(scheme),
          mMetricsPath(metricsPath),
          mScrapeIntervalString(std::to_string(interval) + "s"),
          mScrapeTimeoutString(std::to_string(timeout) + "s") {}

    void AddScrapeTarget(std::string hash, ScrapeTarget& target) {
        std::lock_guard<std::mutex> lock(mMutex);
        mScrapeTargetsMap[hash] = std::make_unique<ScrapeTarget>(target);
    }

    const Json::Value& GetScrapeConfig() { return mScrapeConfig; }
#endif

private:
    Json::Value mScrapeConfig;
    std::vector<RelabelConfig> mRelabelConfigs;
    std::map<std::string, std::string> mHeaders;

    std::mutex mMutex;
    std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>> mScrapeTargetsMap;

    std::atomic<bool> mFinished;
    ThreadPtr mTargetsDiscoveryLoopThread;

    std::unique_ptr<sdk::HTTPClient> mClient;

    bool Validation() const;

    void TargetsDiscoveryLoop();

    bool FetchHttpData(std::string& readBuffer) const;
    bool ParseTargetGroups(const std::string& response,
                           std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>>& newScrapeTargetsMap) const;
    int GetIntSeconds(const std::string& str) const;
    std::string ConvertMapParamsToQueryString() const;

#ifdef APSARA_UNIT_TEST_MAIN
    void SetMockHTTPClient(sdk::HTTPClient* httpClient);
    friend class ScrapeJobUnittest;
    friend class PrometheusInputRunnerUnittest;
#endif
};

} // namespace logtail