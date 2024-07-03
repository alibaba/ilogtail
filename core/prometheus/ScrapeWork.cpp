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

ScrapeTarget::ScrapeTarget(const vector<string>& targets, const Labels& labels, const string& source)
    : mTargets(targets), mLabels(labels), mSource(source) {
    mHash = mLabels.Get("job") + mLabels.Get("__address__") + mLabels.Hash();
}

ScrapeWork::ScrapeWork(const ScrapeTarget& target) : mTarget(target) {
    mClient = make_unique<sdk::CurlClient>();
}

void ScrapeWork::StartScrapeLoop() {
    mFinished.store(false);
    // 以线程的方式实现
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { scrapeLoop(); });
    }
}

void ScrapeWork::StopScrapeLoop() {
    mFinished.store(true);
    // 清理线程
    mScrapeLoopThread->Wait(0ULL);
    mScrapeLoopThread.reset();
}

/// @brief scrape target loop
void ScrapeWork::scrapeLoop() {
    LOG_INFO(sLogger, ("start prometheus scrape loop", mTarget.mHash));
    uint64_t randSleep = getRandSleep();
    this_thread::sleep_for(chrono::nanoseconds(randSleep));

    while (!mFinished.load()) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();
        // ReadLock lock(mScrapeLoopThreadRWL);
        time_t defaultTs = time(nullptr);

        // scrape target by CurlClient
        // TODO: scrape超时处理逻辑，和出错处理
        LOG_INFO(sLogger, ("scrape job", mTarget.mJobName)("target", mTarget.mHost));
        auto httpResponse = scrape();
        if (httpResponse.statusCode == 200) {
            LOG_INFO(sLogger, ("scrape success", httpResponse.statusCode)("target", mTarget.mHash));
            // TODO: 生成自监控指标，但走下面pushEventGroup链路的指标 up指标

            // text parser
            const auto& sourceBuffer = make_shared<SourceBuffer>();
            auto parser = TextParser(sourceBuffer);
            auto eventGroup = parser.Parse(httpResponse.content, defaultTs);

            // 发送到对应的处理队列
            // TODO: 框架处理超时了处理逻辑，如果超时了如何保证下一次采集有效并且发送
            pushEventGroup(move(eventGroup));
        } else {
            // scrape failed
            // TODO: scrape超时处理逻辑，和出错处理
            LOG_WARNING(sLogger, ("scrape failed, status code", httpResponse.statusCode)("target", mTarget.mHash));
            // continue;
        }


        // 需要校验花费的时间是否比采集间隔短
        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = (uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL
            - elapsedTime % ((uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL);
        this_thread::sleep_for(chrono::nanoseconds(timeToWait));
    }
    LOG_INFO(sLogger, ("stop prometheus scrape loop", mTarget.mHash));
}

uint64_t ScrapeWork::getRandSleep() {
    // 根据target信息构造hash输入
    string key = string(mTarget.mJobName + mTarget.mScheme + mTarget.mHost + mTarget.mMetricsPath);
    hash<string> hash_fn;
    uint64_t h = hash_fn(key);
    uint64_t randSleep = ((double)1.0) * mTarget.mScrapeInterval * (1.0 * h / (uint64_t)0xFFFFFFFFFFFFFFFF);
    uint64_t sleepOffset = GetCurrentTimeInNanoSeconds() % (mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

inline sdk::HttpMessage ScrapeWork::scrape() {
    map<string, string> httpHeader;
    string reqBody;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusScrapeWork";
    // 使用CurlClient抓取目标
    try {
        mClient->Send(sdk::HTTP_GET,
                      mTarget.mHost,
                      mTarget.mPort,
                      mTarget.mMetricsPath,
                      mTarget.mQueryString,
                      mTarget.mHeaders,
                      reqBody,
                      mTarget.mScrapeTimeout,
                      httpResponse,
                      "",
                      mTarget.mScheme == "https");
    } catch (const sdk::LOGException& e) {
        LOG_WARNING(sLogger, ("scrape failed", e.GetMessage())("errCode", e.GetErrorCode())("target", mTarget.mHash));
    }
    return httpResponse;
}

void ScrapeWork::pushEventGroup(PipelineEventGroup&& eGroup) {
    LOG_INFO(sLogger,
             ("push event group", mTarget.mHash)("target index", mTarget.inputIndex)("target queueKey",
                                                                                     to_string(mTarget.queueKey)));
    // parser.Parse返回unique_ptr但下面的构造函数接收右值引用，所以又一次拷贝消耗
    auto item = make_unique<ProcessQueueItem>(move(eGroup), mTarget.inputIndex);
    for (size_t i = 0; i < 1000; ++i) {
        if (ProcessQueueManager::GetInstance()->PushQueue(mTarget.queueKey, move(item)) == 0) {
            // LOG_INFO(sLogger, ("push event group success", mTarget.mHash)("", item.get()));
            break;
        }
        // LOG_INFO(sLogger, ("push event group failed", mTarget.mHash)(to_string(i), item.get()));
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}

bool ScrapeTarget::operator<(const ScrapeTarget& other) const {
    if (mJobName != other.mJobName)
        return mJobName < other.mJobName;
    if (mScrapeInterval != other.mScrapeInterval)
        return mScrapeInterval < other.mScrapeInterval;
    if (mScrapeTimeout != other.mScrapeTimeout)
        return mScrapeTimeout < other.mScrapeTimeout;
    if (mScheme != other.mScheme)
        return mScheme < other.mScheme;
    if (mMetricsPath != other.mMetricsPath)
        return mMetricsPath < other.mMetricsPath;
    if (mHost != other.mHost)
        return mHost < other.mHost;
    if (mPort != other.mPort)
        return mPort < other.mPort;
    return mHash < other.mHash;
}


} // namespace logtail
