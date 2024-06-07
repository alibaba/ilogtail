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

#include <condition_variable>
#include <mutex>
#include <queue>
#include <string>

#include "ScrapeWork.h"
#include "queue/FeedbackQueueKey.h"

namespace logtail {

class ScrapeJob {
public:
    ScrapeJob();
    ScrapeJob(const Json::Value& job);

    bool isValid() const;
    bool operator<(const ScrapeJob& other) const;

    std::vector<ScrapeTarget> TargetsDiscovery() const;

    // jobName作为id，有待商榷
    std::string jobName;
    // std::string serviceDiscover;
    std::string metricsPath;
    // std::string relabelConfigs;
    std::string scheme;
    int scrapeInterval;
    int scrapeTimeout;

    bool enableHttp2 = false;
    bool followRedirects = false;
    bool honorTimestamps = false;
    // std::string k8sSdConfig;
    // std::string relabelConfigs;
    // std::string metricRelabelConfigs;

    QueueKey queueKey;
    size_t inputIndex;

    // target需要根据ConfigServer的情况调整
    std::vector<ScrapeTarget> scrapeTargets;

#ifdef APSARA_UNIT_TEST_MAIN
    ScrapeJob(std::string jobName, std::string metricsPath, std::string scheme, int interval, int timeout)
        : jobName(jobName),
          metricsPath(metricsPath),
          scheme(scheme),
          scrapeInterval(interval),
          scrapeTimeout(timeout) {}
#endif

private:
};

using JobEvent = std::function<void()>;

/// @brief 管理所有的ScrapeJob
class ScraperGroup {
public:
    ScraperGroup();
    static ScraperGroup* GetInstance() {
        static auto group = new ScraperGroup;
        return group;
    }

    void UpdateScrapeJob(const ScrapeJob&);
    void RemoveScrapeJob(const ScrapeJob&);

    void Start();
    void Stop();

private:
    std::unordered_map<std::string, std::set<ScrapeTarget>> scrapeJobTargetsMap;
    std::unordered_map<std::string, std::unique_ptr<ScrapeWork>> scrapeIdWorkMap;

    std::atomic<bool> runningState = false;
    ThreadPtr scraperThread;
    std::condition_variable scraperThreadCondition;

    std::mutex eventMutex;
    std::queue<JobEvent> jobEventQueue;

    void ProcessEvents();

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScraperUnittest;
#endif
};

} // namespace logtail
