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

logtail::ScraperGroup::ScraperGroup() {
}

void logtail::ScraperGroup::UpdateScrapeJob(const ScrapeJob& job) {
    if (scrapeJobTargetsMap.count(job.jobName)) {
        // target级别更新，先全部清理之前的work，然后开启新的work
        RemoveScrapeJob(job);
    }
    scrapeJobTargetsMap[job.jobName] = set(job.scrapeTargets.begin(), job.scrapeTargets.end());
    for (const ScrapeTarget& target : scrapeJobTargetsMap[job.jobName]) {
        scrapeIdWorkMap[target.targetId] = std::make_unique<ScrapeWork>(target);
        scrapeIdWorkMap[target.targetId]->StartScrapeLoop();
    }
}

void logtail::ScraperGroup::RemoveScrapeJob(const ScrapeJob& job) {
    for (auto target : scrapeJobTargetsMap[job.jobName]) {
        // 找到对应的线程（协程）并停止
        auto work = scrapeIdWorkMap.find(target.targetId);
        if (work != scrapeIdWorkMap.end()) {
            work->second->StopScrapeLoop();
            scrapeIdWorkMap.erase(target.targetId);
        }
    }
    scrapeJobTargetsMap.erase(job.jobName);
}

void logtail::ScraperGroup::Start() {
}

void logtail::ScraperGroup::Stop() {
    for (auto& iter : scrapeIdWorkMap) {
        // 根据key找到对应的线程（协程）并停止
        iter.second->StopScrapeLoop();
    }
}
