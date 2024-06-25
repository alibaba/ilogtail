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

#include <mutex>

#include "Relabel.h"
#include "ScrapeWork.h"

namespace logtail {

extern const int sRefeshIntervalSeconds;

class ScrapeJob {
public:
    ScrapeJob(const Json::Value& scrapeConfig);

    bool isValid() const;
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

    QueueKey queueKey;
    size_t inputIndex;

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

    std::mutex mMutex;
    std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>> mScrapeTargetsMap;

    std::atomic<bool> mFinished;
    ThreadPtr mTargetsDiscoveryLoopThread;

    void TargetsDiscoveryLoop();

    static size_t WriteCallback(char* contents, size_t size, size_t nmemb, std::string* userp);
    bool FetchHttpData(const std::string& url, std::string& readBuffer) const;
    bool ParseTargetGroups(const std::string& response,
                           const std::string& url,
                           std::unordered_map<std::string, std::unique_ptr<ScrapeTarget>>& newScrapeTargetsMap) const;
    int GetIntSeconds(const std::string& str) const;
    std::string ConvertMapParamsToQueryString() const;
    bool BuildScrapeURL(Labels& labels, ScrapeTarget& st) const;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeJobUnittest;
#endif
};

} // namespace logtail