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

#include <string>

#include "ScrapeWork.h"
#include "queue/FeedbackQueueKey.h"

namespace logtail {

struct ScrapeJob {
    // jobName作为id，有待商榷
    std::string jobName;
    // std::string serviceDiscover;
    std::string metricsPath;
    // std::string relabelConfigs;
    std::string scheme;
    int scrapeInterval;
    int scrapeTimeout;
    // target需要根据ConfigServer的情况调整
    std::vector<ScrapeTarget> scrapeTargets;
    QueueKey queueKey;
    size_t inputIndex;
    bool operator<(const ScrapeJob& other) const { return jobName < other.jobName; }
    ScrapeJob() {}
    ScrapeJob(Json::Value job) {
        jobName = job["jobName"].asString();
        metricsPath = job["metricsPath"].asString();
        scheme = job["scheme"].asString();
        scrapeInterval = job["scrapeInterval"].asInt();
        scrapeTimeout = job["scrapeTimeout"].asInt();
    }
#ifdef APSARA_UNIT_TEST_MAIN
    ScrapeJob(std::string jobName, std::string metricsPath, std::string scheme, int interval, int timeout)
        : jobName(jobName),
          metricsPath(metricsPath),
          scheme(scheme),
          scrapeInterval(interval),
          scrapeTimeout(timeout) {}

#endif
};


/// @brief 对scrape的包装类
// class Scraper {
// private:
// public:
//     Scraper();
// };

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
#ifdef APSARA_UNIT_TEST_MAIN
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
    friend class ScraperUnittest;
#endif
};

} // namespace logtail
