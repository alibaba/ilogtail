#pragma once

#include <stdint.h>

#include <atomic>
#include <string>

#include "boost/functional/hash.hpp"
#include "common/Lock.h"
#include "common/Thread.h"
#include "models/PipelineEventGroup.h"
#include "queue/FeedbackQueueKey.h"
#include "sdk/Common.h"

namespace logtail {

struct ScrapeTarget {
    std::string jobName;
    int scrapeInterval;
    int scrapeTimeout;
    std::string scheme;
    std::string metricsPath;
    std::string host;
    int port;
    QueueKey queueKey;
    size_t inputIndex;
    std::string targetId;
    bool operator<(const ScrapeTarget& other) const {
        if (jobName != other.jobName)
            return jobName < other.jobName;
        if (scrapeInterval != other.scrapeInterval)
            return scrapeInterval < other.scrapeInterval;
        if (scrapeTimeout != other.scrapeTimeout)
            return scrapeTimeout < other.scrapeTimeout;
        if (scheme != other.scheme)
            return scheme < other.scheme;
        if (metricsPath != other.metricsPath)
            return metricsPath < other.metricsPath;
        if (host != other.host)
            return host < other.host;
        if (port != other.port)
            return port < other.port;
        if (queueKey != other.queueKey)
            return queueKey < other.queueKey;
        if (inputIndex != other.inputIndex)
            return inputIndex < other.inputIndex;
        return targetId < other.targetId;
    }
};


class ScrapeWork {
public:
    ScrapeWork();
    ScrapeWork(const ScrapeTarget&);

    ScrapeWork(const ScrapeWork&) = delete;
    ScrapeWork(ScrapeWork&& other) noexcept = default;
    ScrapeWork& operator=(const ScrapeWork&) = delete;
    ScrapeWork& operator=(ScrapeWork&&) noexcept = default;


    void StartScrapeLoop();

    void StopScrapeLoop();


private:
    void scrapeLoop();

    uint64_t getRandSleep();
    sdk::HttpMessage scrape();
    void pushEventGroup(PipelineEventGroup);

    ScrapeTarget target;
    std::atomic<bool> finished;
    std::unique_ptr<sdk::HTTPClient> client;
    ThreadPtr mScrapeLoopThread;
    ReadWriteLock mScrapeLoopThreadRWL;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeWorkUnittest;
#endif
};

} // namespace logtail
