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

ScrapeTarget::ScrapeTarget(const std::string& jobName,
                           const std::string& metricsPath,
                           const std::string& scheme,
                           const std::string& queryString,
                           int interval,
                           int timeout,
                           const map<string, string>& headers)
    : mJobName(jobName),
      mMetricsPath(metricsPath),
      mScheme(scheme),
      mQueryString(queryString),
      mScrapeInterval(interval),
      mScrapeTimeout(timeout),
      mHeaders(headers) {
    mPort = mScheme == "http" ? 80 : 443;
}

bool ScrapeTarget::SetLabels(const Labels& labels) {
    mLabels = labels;

    // TODO: 移到更合适的地方去
    if (mLabels.Get("job").empty()) {
        mLabels.Push(Label{"job", mJobName});
    }

    //
    string address = mLabels.Get("__address__");
    if (address.empty()) {
        return false;
    }

    // st.mScheme
    if (address.find("http://") == 0) {
        mScheme = "http";
        address = address.substr(strlen("http://"));
    } else if (address.find("https://") == 0) {
        mScheme = "https";
        address = address.substr(strlen("https://"));
    }

    // st.mMetricsPath
    auto n = address.find('/');
    if (n != string::npos) {
        mMetricsPath = address.substr(n);
        address = address.substr(0, n);
    }
    if (mMetricsPath.find('/') != 0) {
        mMetricsPath = "/" + mMetricsPath;
    }

    // st.mHost & st.mPort
    auto m = address.find(':');
    if (m != string::npos) {
        mHost = address.substr(0, m);
        mPort = stoi(address.substr(m + 1));
    } else {
        mHost = address;
        if (mScheme == "http") {
            mPort = 80;
        } else if (mScheme == "https") {
            mPort = 443;
        } else {
            return false;
        }
    }

    // set URL
    mTargetURL = mScheme + "://" + mHost + ":" + to_string(mPort) + mMetricsPath
        + (mQueryString.empty() ? "" : "?" + mQueryString);
    mHash = mJobName + mTargetURL + ToString(mLabels.Hash());

    return true;
}

void ScrapeTarget::SetPipelineInfo(QueueKey queueKey, size_t index) {
    mQueueKey = queueKey;
    mInputIndex = index;
}

void ScrapeTarget::SetHostAndPort(const std::string& host, uint32_t port) {
    mHost = host;
    mPort = port;
}

string ScrapeTarget::GetHash() {
    return mHash;
}

string ScrapeTarget::GetJobName() {
    return mJobName;
}

string ScrapeTarget::GetHost() {
    return mHost;
}

map<string, string> ScrapeTarget::GetHeaders() {
    return mHeaders;
}

bool ScrapeTarget::operator<(const ScrapeTarget& other) const {
    if (mHash != other.mHash) {
        return mHash < other.mHash;
    }
    if (mJobName != other.mJobName) {
        return mJobName < other.mJobName;
    }
    if (mScrapeInterval != other.mScrapeInterval) {
        return mScrapeInterval < other.mScrapeInterval;
    }
    if (mScrapeTimeout != other.mScrapeTimeout) {
        return mScrapeTimeout < other.mScrapeTimeout;
    }
    if (mScheme != other.mScheme) {
        return mScheme < other.mScheme;
    }
    if (mMetricsPath != other.mMetricsPath) {
        return mMetricsPath < other.mMetricsPath;
    }
    if (mHost != other.mHost) {
        return mHost < other.mHost;
    }
    return mPort < other.mPort;
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
    this_thread::sleep_for(chrono::nanoseconds(randSleep));

    while (!mFinished.load()) {
        uint64_t lastProfilingTime = GetCurrentTimeInNanoSeconds();
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
            LOG_WARNING(sLogger,
                        ("scrape failed, status code", httpResponse.statusCode)("target", mTarget.mHash)("http header",
                                                                                                         headerStr));
        }

        // 需要校验花费的时间是否比采集间隔短
        uint64_t elapsedTime = GetCurrentTimeInNanoSeconds() - lastProfilingTime;
        uint64_t timeToWait = (uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL
            - elapsedTime % ((uint64_t)mTarget.mScrapeInterval * 1000ULL * 1000ULL * 1000ULL);
        this_thread::sleep_for(chrono::nanoseconds(timeToWait));
    }
    LOG_INFO(sLogger, ("stop prometheus scrape loop", mTarget.mHash));
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


} // namespace logtail
