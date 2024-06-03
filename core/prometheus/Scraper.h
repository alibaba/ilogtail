#pragma once

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
};


/// @brief 对scrape的包装类
class Scraper {
private:
public:
    Scraper();
};

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
};

} // namespace logtail
