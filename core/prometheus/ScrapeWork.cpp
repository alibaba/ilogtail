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

#include "ScrapeWork.h"

#include "TextParser.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

ScrapeWork::ScrapeWork() {
    client = std::make_unique<sdk::CurlClient>();
    finished = false;
}

ScrapeWork::ScrapeWork(const ScrapeTarget& target) : target(target), finished(false) {
    client = std::make_unique<sdk::CurlClient>();
}

void ScrapeWork::StartScrapeLoop() {
    finished = false;
    // 以线程的方式实现
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { scrapeLoop(); });
    }
}

void ScrapeWork::StopScrapeLoop() {
    finished = true;
    // 清理线程
    mScrapeLoopThread->Wait(0ULL);
    mScrapeLoopThread.reset();
}

/// @brief scrape target loop
void ScrapeWork::scrapeLoop() {
    printf("start prometheus scrape loop\n");
    uint64_t randSleep = getRandSleep();
    printf("randSleep %llums\n", randSleep / 1000ULL / 1000ULL);
    std::this_thread::sleep_for(std::chrono::nanoseconds(randSleep));

    while (!finished) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();
        // ReadLock lock(mScrapeLoopThreadRWL);
        printf("scrape time %llus\n", lastProfilingTime / 1000ULL / 1000ULL / 1000ULL);

        // scrape target by CurlClient
        // TODO: scrape超时处理逻辑，和出错处理
        auto httpResponse = scrape();
        if (httpResponse.statusCode == 200) {
            // TODO: 生成自监控指标，但走下面pushEventGroup链路的指标 up指标

            // text parser
            const auto& sourceBuffer = make_shared<SourceBuffer>();
            auto parser = TextParser(sourceBuffer);
            auto eventGroup = parser.Parse(httpResponse.content);

            // 发送到对应的处理队列
            // TODO: 框架处理超时了处理逻辑，如果超时了如何保证下一次采集有效并且发送
            pushEventGroup(std::move(*eventGroup));
        } else {
            // scrape failed
            // TODO: scrape超时处理逻辑，和出错处理
            printf("scrape failed, status code: %d\n", httpResponse.statusCode);
            // continue;
        }


        // 需要校验花费的时间是否比采集间隔短
        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = (uint64_t)target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL
            - elapsedTime % ((uint64_t)target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL);
        printf("time to wait: %llums\n", timeToWait / 1000ULL / 1000ULL);
        std::this_thread::sleep_for(std::chrono::nanoseconds(timeToWait));
    }
    printf("stop loop\n");
}

uint64_t ScrapeWork::getRandSleep() {
    // 根据target信息构造hash输入
    string key = string(target.jobName + target.scheme + target.host + target.metricsPath);
    // 遇到性能问题可以考虑使用xxhash代替Boost.hash优化
    boost::hash<std::string> string_hash;
    uint64_t h = string_hash(key);
    uint64_t randSleep = ((double)1.0) * target.scrapeInterval * (1.0 * h / (uint64_t)0xFFFFFFFFFFFFFFFF);
    uint64_t sleepOffset = GetCurrentTimeInNanoSeconds() % (target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += target.scrapeInterval * 1000ULL * 1000ULL * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

inline sdk::HttpMessage ScrapeWork::scrape() {
    std::map<std::string, std::string> httpHeader;
    string reqBody;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusScrapeWork";
    // 使用CurlClient抓取目标
    try {
        client->Send(sdk::HTTP_GET,
                     target.host,
                     target.port,
                     target.metricsPath,
                     "",
                     httpHeader,
                     reqBody,
                     target.scrapeTimeout,
                     httpResponse,
                     "",
                     false);
    } catch (const sdk::LOGException& e) {
        printf("errCode %s, errMsg %s \n", e.GetErrorCode().c_str(), e.GetMessage().c_str());
    }
    return httpResponse;
}

void ScrapeWork::pushEventGroup(PipelineEventGroup eGroup) {
    // parser.Parse返回unique_ptr但下面的构造函数接收右值引用，所以又一次拷贝消耗
    auto item = std::make_unique<ProcessQueueItem>(std::move(eGroup), target.inputIndex);
    for (size_t i = 0; i < 1000; ++i) {
        if (ProcessQueueManager::GetInstance()->PushQueue(target.queueKey, std::move(item)) == 0) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool ScrapeTarget::operator<(const ScrapeTarget& other) const {
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

ScrapeTarget::ScrapeTarget() {
}

ScrapeTarget::ScrapeTarget(const Json::Value& target) {
    host = target["host"].asString();
    if (host.find(':') != std::string::npos) {
        port = stoi(host.substr(host.find(':') + 1));
    } else {
        port = 8080;
    }
}


} // namespace logtail
