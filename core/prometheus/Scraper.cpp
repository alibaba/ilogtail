#include "Scraper.h"


logtail::ScraperGroup::ScraperGroup() {
}

void logtail::ScraperGroup::UpdateScrapeJob(const ScrapeJob& job) {
    if (scrapeJobTargetsMap.count(job.jobName)) {
        // target级别更新，先全部清理之前的work，然后开启新的work
        RemoveScrapeJob(job);
    }
    scrapeJobTargetsMap[job.jobName] = job.scrapeTargetsSet;
    for (const ScrapeTarget& target : scrapeJobTargetsMap[job.jobName]) {
        scrapeIdWorkMap[target.targetId] = std::make_unique<ScrapeWork>(target);
        scrapeIdWorkMap[target.targetId]->StartScrapeLoop();
    }
}

void logtail::ScraperGroup::RemoveScrapeJob(const ScrapeJob& job) {
    for (auto target : scrapeJobTargetsMap[job.jobName]) {
        // 根据key找到对应的线程（协程）并停止
        auto work = scrapeIdWorkMap.find(target.targetId);
        if (work != scrapeIdWorkMap.end()) {
            work->second->StopScrapeLoop();
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
