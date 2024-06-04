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
    std::string metricsPath;
    std::string scheme;
    int scrapeInterval;
    int scrapeTimeout;

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
        return targetId < other.targetId;
    }
    ScrapeTarget() {}
    ScrapeTarget(Json::Value target) {
        host = target["host"].asString();
        if (host.find(':') != std::string::npos) {
            port = stoi(host.substr(host.find(':') + 1));
        } else {
            port = 8080;
        }
    }
#ifdef APSARA_UNIT_TEST_MAIN
    ScrapeTarget(std::string jobName,
                 std::string metricsPath,
                 std::string scheme,
                 int interval,
                 int timeout,
                 std::string host,
                 int port)
        : jobName(jobName),
          metricsPath(metricsPath),
          scheme(scheme),
          scrapeInterval(interval),
          scrapeTimeout(timeout),
          host(host),
          port(port) {}

#endif
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
    // ReadWriteLock mScrapeLoopThreadRWL;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeWorkUnittest;
    friend class PrometheusInputRunnerUnittest;
    friend class InputPrometheusUnittest;
#endif
};

} // namespace logtail
