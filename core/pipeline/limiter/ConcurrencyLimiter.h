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

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

#include "monitor/metric_constants/MetricConstants.h"

namespace logtail {

class ConcurrencyLimiter {
public:
    enum FallbackMode {
        Fast,
        Slow
    };

    ConcurrencyLimiter(const std::string& description,
                       uint32_t maxConcurrency,
                       FallbackMode fallbackMode = Fast,
                       uint32_t maxRetryIntervalSecs = 60,
                       uint32_t minRetryIntervalSecs = 5,
                       double retryIntervalUpRatio = 1.5,
                       double concurrencyDownRatio = 0.5)
        : mDescription(description),
          mMaxConcurrency(maxConcurrency),
          mFallbackMode(fallbackMode),
          mCurrenctConcurrency(maxConcurrency),
          mMaxRetryIntervalSecs(maxRetryIntervalSecs),
          mMinRetryIntervalSecs(minRetryIntervalSecs),
          mRetryIntervalSecs(minRetryIntervalSecs),
          mRetryIntervalUpRatio(retryIntervalUpRatio),
          mConcurrencyDownRatio(concurrencyDownRatio) {}

    bool IsValidToPop();
    void PostPop();
    void OnSendDone();

    void Increase();
    void Decrease(FallbackMode fallbackMode);

    void OnSuccess(std::chrono::system_clock::time_point time);
    void OnFail(std::chrono::system_clock::time_point time);


    void AdjustConcurrency(bool success, std::chrono::system_clock::time_point time);

    static std::string GetLimiterMetricName(const std::string& limiter) {
        if (limiter == "region") {
            return METRIC_COMPONENT_QUEUE_FETCH_REJECTED_BY_REGION_LIMITER_TIMES_TOTAL;
        } else if (limiter == "project") {
            return METRIC_COMPONENT_QUEUE_FETCH_REJECTED_BY_PROJECT_LIMITER_TIMES_TOTAL;
        } else if (limiter == "logstore") {
            return METRIC_COMPONENT_QUEUE_FETCH_REJECTED_BY_LOGSTORE_LIMITER_TIMES_TOTAL;
        }
        return limiter;
    }

#ifdef APSARA_UNIT_TEST_MAIN

    uint32_t GetCurrentLimit() const;
    uint32_t GetCurrentInterval() const;
    void SetCurrentLimit(uint32_t limit);
    void SetInSendingCount(uint32_t count);
    uint32_t GetInSendingCount() const;

#endif

private:
    const std::string mDescription;

    std::atomic_uint32_t mInSendingCnt = 0U;

    uint32_t mMaxConcurrency = 0;

    FallbackMode mFallbackMode;

    mutable std::mutex mLimiterMux;
    uint32_t mCurrenctConcurrency = 0;

    uint32_t mMaxRetryIntervalSecs = 0;
    uint32_t mMinRetryIntervalSecs = 0;

    uint32_t mRetryIntervalSecs = 0;

    double mRetryIntervalUpRatio = 0.0;
    double mConcurrencyDownRatio = 0.0;

    std::chrono::system_clock::time_point mLastCheckTime;



    mutable std::mutex mStatisticsMux;
    std::chrono::system_clock::time_point mLastStatisticsTime;
    uint32_t mStatisticsTotal;
    uint32_t mStatisticsFailTotal;

};

} // namespace logtail
