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

#include "Scraper.h"

using namespace std;

namespace logtail {


ScrapeJob::ScrapeJob() {
}

/// @brief Construct from json config
ScrapeJob::ScrapeJob(const Json::Value& jobConfig) {
    if (jobConfig.isMember("job_name")) {
        jobName = jobConfig["job_name"].asString();
    }
    if (jobConfig.isMember("metrics_path")) {
        metricsPath = jobConfig["metrics_path"].asString();
    }
    if (jobConfig.isMember("scheme")) {
        scheme = jobConfig["scheme"].asString();
    }
    if (jobConfig.isMember("scrape_interval")) {
        scrapeInterval = jobConfig["scrape_interval"].asInt();
    }
    if (jobConfig.isMember("scrape_timeout")) {
        scrapeTimeout = jobConfig["scrape_timeout"].asInt();
    }
}


bool ScrapeJob::isValid() const {
    return !jobName.empty();
}

/// @brief Sort temporarily by name
bool ScrapeJob::operator<(const ScrapeJob& other) const {
    return jobName < other.jobName;
}

vector<ScrapeTarget> ScrapeJob::TargetsDiscovery() const {
    // 临时过渡
    return scrapeTargets;
}

ScraperGroup::ScraperGroup() : runningState(false), scraperThread(nullptr) {
}

void ScraperGroup::UpdateScrapeJob(const ScrapeJob& job) {
    if (scrapeJobTargetsMap.count(job.jobName)) {
        // target级别更新，先全部清理之前的work，然后开启新的work
        RemoveScrapeJob(job);
    }

    // 注意解决多个jobs请求targets时的阻塞问题，relabel阻塞
    // 和master交互时 targets更新而config没变的过程
    auto updateJobEvnet = [this, &job]() {
        // 进行targets发现
        vector<ScrapeTarget> targets = job.TargetsDiscovery();

        // targets relabel 逻辑

        scrapeJobTargetsMap[job.jobName] = set(targets.begin(), targets.end());
        for (const ScrapeTarget& target : scrapeJobTargetsMap[job.jobName]) {
            scrapeIdWorkMap[target.targetId] = std::make_unique<ScrapeWork>(target);
            scrapeIdWorkMap[target.targetId]->StartScrapeLoop();
        }
    };
    unique_lock<std::mutex> lock(eventMutex);
    jobEventQueue.push(updateJobEvnet);
    scraperThreadCondition.notify_one();
}

void ScraperGroup::RemoveScrapeJob(const ScrapeJob& job) {
    auto removeJobEvent = [this, &job]() {
        for (auto target : scrapeJobTargetsMap[job.jobName]) {
            // 找到对应的线程（协程）并停止
            auto work = scrapeIdWorkMap.find(target.targetId);
            if (work != scrapeIdWorkMap.end()) {
                work->second->StopScrapeLoop();
                scrapeIdWorkMap.erase(target.targetId);
            }
        }
        scrapeJobTargetsMap.erase(job.jobName);
    };
    unique_lock<std::mutex> lock(eventMutex);
    jobEventQueue.push(removeJobEvent);
    scraperThreadCondition.notify_one();
}

void ScraperGroup::Start() {
    runningState = true;
    scraperThread = CreateThread([this]() { ProcessEvents(); });
}

void ScraperGroup::Stop() {
    runningState = false;
    scraperThreadCondition.notify_one();
    for (auto& iter : scrapeIdWorkMap) {
        // 根据key找到对应的线程（协程）并停止
        iter.second->StopScrapeLoop();
    }
    if (scraperThread)
        scraperThread->Wait(0ULL);
    scraperThread.reset();
    scrapeJobTargetsMap.clear();
    scrapeIdWorkMap.clear();
    while (!jobEventQueue.empty())
        jobEventQueue.pop();
}

void ScraperGroup::ProcessEvents() {
    JobEvent event;
    while (runningState) {
        {
            std::unique_lock<std::mutex> lock(eventMutex);
            scraperThreadCondition.wait(lock, [this]() { return !jobEventQueue.empty() || !runningState; });

            if (!runningState && jobEventQueue.empty()) {
                break;
            }

            event = jobEventQueue.front();
            jobEventQueue.pop();
        }

        event();
    }
}


} // namespace logtail
