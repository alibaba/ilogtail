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

#include "PrometheusInputRunner.h"

#include <memory>
#include <string>

#include "common/Flags.h"
#include "common/JsonUtil.h"
#include "common/StringTools.h"
#include "common/http/AsynCurlRunner.h"
#include "common/timer/Timer.h"
#include "logger/Logger.h"
#include "prometheus/Constants.h"
#include "sdk/Common.h"
#include "sdk/Exception.h"

using namespace std;

DECLARE_FLAG_STRING(loong_collector_operator_service);
DECLARE_FLAG_INT32(loong_collector_operator_service_port);
DECLARE_FLAG_STRING(_pod_name_);

namespace logtail {

PrometheusInputRunner::PrometheusInputRunner() : mUnRegisterMs(0) {
    mIsStarted.store(false);
    mClient = std::make_unique<sdk::CurlClient>();

    mServiceHost = STRING_FLAG(loong_collector_operator_service);
    mServicePort = INT32_FLAG(loong_collector_operator_service_port);
    mPodName = STRING_FLAG(_pod_name_);
    mTimer = std::make_shared<Timer>();
}

/// @brief receive scrape jobs from input plugins and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(std::shared_ptr<TargetSubscriberScheduler> targetSubscriber) {
    RemoveScrapeInput(targetSubscriber->GetId());

    targetSubscriber->mServiceHost = mServiceHost;
    targetSubscriber->mServicePort = mServicePort;
    targetSubscriber->mPodName = mPodName;

    targetSubscriber->mUnRegisterMs = mUnRegisterMs;
    targetSubscriber->SetTimer(mTimer);
    targetSubscriber->SetFirstExecTime(std::chrono::steady_clock::now());
    // 1. add subscriber to mTargetSubscriberSchedulerMap
    {
        WriteLock lock(mSubscriberMapRWLock);
        mTargetSubscriberSchedulerMap[targetSubscriber->GetId()] = targetSubscriber;
    }
    // 2. build Ticker Event and add it to Timer
    targetSubscriber->ScheduleNext();
}

void PrometheusInputRunner::RemoveScrapeInput(const std::string& jobName) {
    WriteLock lock(mSubscriberMapRWLock);
    if (mTargetSubscriberSchedulerMap.count(jobName)) {
        mTargetSubscriberSchedulerMap[jobName]->Cancel();
        mTargetSubscriberSchedulerMap.erase(jobName);
    }
}

/// @brief targets discovery and start scrape work
void PrometheusInputRunner::Start() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Start"));
    if (mIsStarted.load()) {
        return;
    }
    mIsStarted.store(true);
    mTimer->Init();
    AsynCurlRunner::GetInstance()->Init();

    mThreadRes = std::async(launch::async, [this]() {
        // only register when operator exist
        if (!mServiceHost.empty()) {
            int retry = 0;
            while (mIsThreadRunning.load()) {
                ++retry;
                sdk::HttpMessage httpResponse = SendRegisterMessage(prometheus::REGISTER_COLLECTOR_PATH);
                if (httpResponse.statusCode != 200) {
                    LOG_ERROR(sLogger, ("register failed, statusCode", httpResponse.statusCode));
                    if (retry % 3 == 0) {
                        LOG_INFO(sLogger, ("register failed, retried", ToString(retry)));
                    }
                } else {
                    // register success
                    // response will be { "unregister_ms": 30000 }
                    if (!httpResponse.content.empty()) {
                        string responseStr = httpResponse.content;
                        string errMsg;
                        Json::Value responseJson;
                        if (!ParseJsonTable(responseStr, responseJson, errMsg)) {
                            LOG_ERROR(sLogger, ("register failed, parse response failed", responseStr));
                        }
                        if (responseJson.isMember(prometheus::UNREGISTER_MS)
                            && responseJson[prometheus::UNREGISTER_MS].isUInt64()) {
                            mUnRegisterMs = responseJson[prometheus::UNREGISTER_MS].asUInt64();
                        }
                    }
                    LOG_INFO(sLogger, ("Register Success", mPodName));
                    // start subscribe immediately
                    SubscribeOnce();
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    });
}

/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Stop"));

    mIsStarted.store(false);
    mIsThreadRunning.store(false);
    mTimer->Stop();

    AsynCurlRunner::GetInstance()->Stop();

    CancelAllTargetSubscriber();
    {
        WriteLock lock(mSubscriberMapRWLock);
        mTargetSubscriberSchedulerMap.clear();
    }

    // only unregister when operator exist
    if (!mServiceHost.empty()) {
        auto res = std::async(launch::async, [this]() {
            for (int retry = 0; retry < 3; ++retry) {
                sdk::HttpMessage httpResponse = SendRegisterMessage(prometheus::UNREGISTER_COLLECTOR_PATH);
                if (httpResponse.statusCode != 200) {
                    LOG_ERROR(sLogger, ("unregister failed, statusCode", httpResponse.statusCode));
                } else {
                    LOG_INFO(sLogger, ("Unregister Success", mPodName));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
}

sdk::HttpMessage PrometheusInputRunner::SendRegisterMessage(const string& url) const {
    map<string, string> httpHeader;
    httpHeader[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
    try {
        mClient->Send(sdk::HTTP_GET,
                      mServiceHost,
                      mServicePort,
                      url,
                      "pod_name=" + mPodName,
                      httpHeader,
                      "",
                      10,
                      httpResponse,
                      "",
                      false);
    } catch (const sdk::LOGException& e) {
        LOG_ERROR(sLogger, ("curl error", e.what())("url", url)("pod_name", mPodName));
    }
    return httpResponse;
}

bool PrometheusInputRunner::HasRegisteredPlugin() {
    ReadLock lock(mSubscriberMapRWLock);
    return !mTargetSubscriberSchedulerMap.empty();
}

void PrometheusInputRunner::CancelAllTargetSubscriber() {
    ReadLock lock(mSubscriberMapRWLock);
    for (auto& it : mTargetSubscriberSchedulerMap) {
        it.second->Cancel();
    }
}

void PrometheusInputRunner::SubscribeOnce() {
    ReadLock lock(mSubscriberMapRWLock);
    for (auto& [k, v] : mTargetSubscriberSchedulerMap) {
        LOG_WARNING(sLogger, ("subscribe once", "k"));
        v->SubscribeOnce(std::chrono::steady_clock::now());
    }
}

}; // namespace logtail