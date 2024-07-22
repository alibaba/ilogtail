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

#include <xxhash/xxhash.h>

#include <cstddef>
#include <map>
#include <string>

#include "FeedbackQueueKey.h"
#include "StringTools.h"
#include "TextParser.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"
#include "queue/ProcessQueueItem.h"
#include "queue/ProcessQueueManager.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;

namespace logtail {

ScrapeTarget::ScrapeTarget(std::shared_ptr<ScrapeConfig> scrapeConfigPtr,
                           std::unique_ptr<Labels> labelsPtr,
                           QueueKey queueKey,
                           size_t inputIndex)
    : mScrapeConfigPtr(scrapeConfigPtr),
      mLabelsPtr(std::move(labelsPtr)),
      mQueueKey(queueKey),
      mInputIndex(inputIndex) {
    mPort = mScrapeConfigPtr->mScheme == "http" ? 80 : 443;

    // host & port
    string address = mLabelsPtr->Get("__address__");
    auto m = address.find(':');
    if (m != string::npos) {
        mHost = address.substr(0, m);
        mPort = stoi(address.substr(m + 1));
    }

    // query string
    for (auto it = mScrapeConfigPtr->mParams.begin(); it != mScrapeConfigPtr->mParams.end(); ++it) {
        const auto& key = it->first;
        const auto& values = it->second;
        for (const auto& value : values) {
            if (!mQueryString.empty()) {
                mQueryString += "&";
            }
            mQueryString = mQueryString + key + "=" + value;
        }
    }

    // URL
    string tmpTargetURL = mScrapeConfigPtr->mScheme + "://" + mHost + ":" + to_string(mPort)
        + mScrapeConfigPtr->mMetricsPath + (mQueryString.empty() ? "" : "?" + mQueryString);
    mHash = mScrapeConfigPtr->mJobName + tmpTargetURL + ToString(mLabelsPtr->Hash());
}

string ScrapeTarget::GetHash() {
    return mHash;
}

string ScrapeTarget::GetJobName() {
    return mScrapeConfigPtr->mJobName;
}

string ScrapeTarget::GetHost() {
    return mHost;
}

map<string, string> ScrapeTarget::GetHeaders() {
    return mScrapeConfigPtr->mHeaders;
}

bool ScrapeTarget::operator<(const ScrapeTarget& other) const {
    return mHash < other.mHash;
}

ScrapeWork::ScrapeWork(const ScrapeTarget& target) : mTarget(target) {
    mClient = make_unique<sdk::CurlClient>();
}

void ScrapeWork::StartScrapeLoop() {
    mFinished.store(false);
    // 以线程的方式实现
    if (!mScrapeLoopThread) {
        mScrapeLoopThread = CreateThread([this]() { ScrapeLoop(); });
    }
}

void ScrapeWork::StopScrapeLoop() {
    mFinished.store(true);
    // 清理线程
    mScrapeLoopThread->Wait(0ULL);
    mScrapeLoopThread.reset();
}

/// @brief scrape target loop
void ScrapeWork::ScrapeLoop() {
    LOG_INFO(sLogger, ("start prometheus scrape loop", mTarget.mHash));
    uint64_t randSleep = GetRandSleep();
    // 无损升级
    // 如果下一次采集点（randSleep +
    // currentTime）在下线时间+一次采集周期（mUnRegisterMs+mScrapeInterval）之后，则立马采集一次补点
    if (randSleep + GetCurrentTimeInNanoSeconds() > (uint64_t)mUnRegisterMs * 1000ULL * 1000ULL
            + (uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL) {
        ScrapeAndPush();
        // 更新randSleep
        randSleep = GetRandSleep();
    }

    this_thread::sleep_for(chrono::nanoseconds(randSleep));

    while (!mFinished.load()) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();

        ScrapeAndPush();

        // 需要校验花费的时间是否比采集间隔短
        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = (uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL
            - elapsedTime % ((uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL);
        this_thread::sleep_for(chrono::nanoseconds(timeToWait));
    }
    LOG_INFO(sLogger, ("stop prometheus scrape loop", mTarget.mHash));
}

void ScrapeWork::ScrapeAndPush() {
    time_t defaultTs = time(nullptr);

    // scrape target by CurlClient
    // TODO: scrape超时处理逻辑，和出错处理
    auto httpResponse = Scrape();
    if (httpResponse.statusCode == 200) {
        // TODO: 生成自监控指标，但走下面pushEventGroup链路的指标 up指标

        // text parser
        auto parser = TextParser();
        auto eventGroup = parser.Parse(httpResponse.content, defaultTs, mTarget.mJobName, mTarget.mHost);

        // 发送到对应的处理队列
        // TODO: 框架处理超时了处理逻辑，如果超时了如何保证下一次采集有效并且发送
        PushEventGroup(std::move(eventGroup));
    } else {
        // scrape failed
        // TODO: scrape超时处理逻辑，和出错处理
        string headerStr;
        for (auto [k, v] : mTarget.mHeaders) {
            headerStr += k + ":" + v + ";";
        }
        LOG_WARNING(
            sLogger,
            ("scrape failed, status code", httpResponse.statusCode)("target", mTarget.mHash)("http header", headerStr));
    }
}

uint64_t ScrapeWork::GetRandSleep() {
    // 根据target信息构造hash输入
    const string& key = mTarget.mHash;
    uint64_t h = XXH64(key.c_str(), key.length(), 0);
    uint64_t randSleep = ((double)1.0) * mTarget.mScrapeInterval * (1.0 * h / (double)0xFFFFFFFFFFFFFFFF);
    uint64_t sleepOffset = GetCurrentTimeInNanoSeconds() % (mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL);
    if (randSleep < sleepOffset) {
        randSleep += mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL;
    }
    randSleep -= sleepOffset;
    return randSleep;
}

inline sdk::HttpMessage ScrapeWork::Scrape() {
    map<string, string> httpHeader;
    string reqBody;
    sdk::HttpMessage httpResponse;
    // TODO: 等待框架删除对respond返回头的 X_LOG_REQUEST_ID 检查
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

void ScrapeWork::PushEventGroup(PipelineEventGroup&& eGroup) {
    LOG_INFO(sLogger,
             ("push event group", mTarget.mHash)("target index", mTarget.mInputIndex)("target queueKey",
                                                                                      to_string(mTarget.mQueueKey)));
    auto item = make_unique<ProcessQueueItem>(std::move(eGroup), mTarget.mInputIndex);
    for (size_t i = 0; i < 1000; ++i) {
        if (ProcessQueueManager::GetInstance()->PushQueue(mTarget.mQueueKey, std::move(item)) == 0) {
            return;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    // TODO: 如果push失败如何处理
    LOG_INFO(sLogger, ("push event group failed", mTarget.mHash));
}

void ScrapeWork::SetUnRegisterMs(uint64_t unRegisterMs) {
    mUnRegisterMs = unRegisterMs;
}


} // namespace logtail
