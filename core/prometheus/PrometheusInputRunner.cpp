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
#include "monitor/LogtailMetric.h"
#include "prometheus/Constants.h"
#include "sdk/Common.h"
#include "sdk/Exception.h"

using namespace std;

DECLARE_FLAG_STRING(loong_collector_operator_service);
DECLARE_FLAG_INT32(loong_collector_operator_service_port);
DECLARE_FLAG_STRING(_pod_name_);

namespace logtail {

PrometheusInputRunner::PrometheusInputRunner()
    : mServiceHost(STRING_FLAG(loong_collector_operator_service)),
      mServicePort(INT32_FLAG(loong_collector_operator_service_port)),
      mPodName(STRING_FLAG(_pod_name_)),
      mUnRegisterMs(0) {
    mClient = std::make_unique<sdk::CurlClient>();


    mTimer = std::make_shared<Timer>();

    MetricLabels labels;
    // labels.emplace_back(METRIC_LABEL_INSTANCE_ID, Application::GetInstance()->GetInstanceId());
    labels.emplace_back(prometheus::POD_NAME, mPodName);
    labels.emplace_back(prometheus::OPERATOR_HOST, mServiceHost);
    labels.emplace_back(prometheus::OPERATOR_PORT, ToString(mServicePort));

    DynamicMetricLabels dynamicLabels;
    // dynamicLabels.emplace_back(METRIC_LABEL_PROJECTS, []() -> std::string { return FlusherSLS::GetAllProjects(); });

    WriteMetrics::GetInstance()->PrepareMetricsRecordRef(
        mMetricsRecordRef, std::move(labels), std::move(dynamicLabels));

    mIntGauges[prometheus::PROM_REGISTER_STATE] = mMetricsRecordRef.CreateIntGauge(prometheus::PROM_REGISTER_STATE);
}

/// @brief receive scrape jobs from input plugins and update scrape jobs
void PrometheusInputRunner::UpdateScrapeInput(std::shared_ptr<TargetSubscriberScheduler> targetSubscriber) {
    RemoveScrapeInput(targetSubscriber->GetId());

    targetSubscriber->mServiceHost = mServiceHost;
    targetSubscriber->mServicePort = mServicePort;
    targetSubscriber->mPodName = mPodName;

    targetSubscriber->mUnRegisterMs = mUnRegisterMs.load();
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
void PrometheusInputRunner::Init() {
    std::lock_guard<mutex> lock(mStartMutex);
    if (mIsStarted) {
        return;
    }
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Start"));
    mIsStarted = true;
    mTimer->Init();
    AsynCurlRunner::GetInstance()->Init();

    LOG_INFO(sLogger, ("PrometheusInputRunner", "register"));
    // only register when operator exist
    if (!mServiceHost.empty()) {
        mIsThreadRunning.store(true);
        auto res = std::async(launch::async, [this]() {
            std::lock_guard<mutex> lock(mRegisterMutex);
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
                            mUnRegisterMs.store(responseJson[prometheus::UNREGISTER_MS].asUInt64());
                        }
                    }
                    mIntGauges[prometheus::PROM_REGISTER_STATE]->Set(1);
                    LOG_INFO(sLogger, ("Register Success", mPodName));
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
}

/// @brief stop scrape work and clear all scrape jobs
void PrometheusInputRunner::Stop() {
    std::lock_guard<mutex> lock(mStartMutex);
    if (!mIsStarted) {
        return;
    }

    mIsStarted = false;
    mIsThreadRunning.store(false);
    mTimer->Stop();

    LOG_INFO(sLogger, ("PrometheusInputRunner", "stop asyn curl runner"));
    AsynCurlRunner::GetInstance()->Stop();

    LOG_INFO(sLogger, ("PrometheusInputRunner", "cancel all target subscribers"));
    CancelAllTargetSubscriber();
    {
        WriteLock lock(mSubscriberMapRWLock);
        mTargetSubscriberSchedulerMap.clear();
    }

    // only unregister when operator exist
    if (!mServiceHost.empty()) {
        LOG_INFO(sLogger, ("PrometheusInputRunner", "unregister"));
        auto res = std::async(launch::async, [this]() {
            std::lock_guard<mutex> lock(mRegisterMutex);
            for (int retry = 0; retry < 3; ++retry) {
                sdk::HttpMessage httpResponse = SendRegisterMessage(prometheus::UNREGISTER_COLLECTOR_PATH);
                if (httpResponse.statusCode != 200) {
                    LOG_ERROR(sLogger, ("unregister failed, statusCode", httpResponse.statusCode));
                } else {
                    LOG_INFO(sLogger, ("Unregister Success", mPodName));
                    mIntGauges[prometheus::PROM_REGISTER_STATE]->Set(0);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }
    LOG_INFO(sLogger, ("PrometheusInputRunner", "Stop"));
}

bool PrometheusInputRunner::HasRegisteredPlugins() const {
    ReadLock lock(mSubscriberMapRWLock);
    return !mTargetSubscriberSchedulerMap.empty();
}

sdk::HttpMessage PrometheusInputRunner::SendRegisterMessage(const string& url) const {
    map<string, string> httpHeader;
    httpHeader[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
    sdk::HttpMessage httpResponse;
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = prometheus::PROMETHEUS_PREFIX + mPodName;
#ifdef APSARA_UNIT_TEST_MAIN
    httpResponse.statusCode = 200;
    return httpResponse;
#endif
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


void PrometheusInputRunner::CancelAllTargetSubscriber() {
    ReadLock lock(mSubscriberMapRWLock);
    for (auto& it : mTargetSubscriberSchedulerMap) {
        it.second->Cancel();
    }
}
}; // namespace logtail