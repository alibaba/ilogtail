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
#include <ctime>
#include <mutex>

namespace logtail {

enum class LimiterLabel { REGION, PROJECT, LOGSTORE };

class ConcurrencyLimiter {
public:
    ConcurrencyLimiter(LimiterLabel limiterLabel, uint32_t maxConcurrency, uint32_t minConcurrency = 0,  
        uint32_t maxRetryIntervalSecs = 3600, uint32_t minRetryIntervalSecs = 30, 
        double upRatio = 1.5, double downRatio = 0.5) : 
        mMaxConcurrency(maxConcurrency), mMinConcurrency(minConcurrency), mCurrenctConcurrency(maxConcurrency),
        mMaxRetryIntervalSecs(maxRetryIntervalSecs), mMinRetryIntervalSecs(minRetryIntervalSecs), 
        mRetryIntervalSecs(minRetryIntervalSecs), mUpRatio(upRatio), mDownRatio(downRatio) {}

    bool IsValidToPop();
    void PostPop();
    void OnSendDone();
    LimiterLabel GetLimiterLabel() {
        return mLimiterLabel;
    }

    void OnSuccess();
    void OnFail(time_t curTime);

    uint32_t GetCurrentLimit() const;

    uint32_t GetCurrentInterval() const;

#ifdef APSARA_UNIT_TEST_MAIN
    void SetCurrentLimit(uint32_t limit);

    void SetInSendingCount(uint32_t count);
    uint32_t GetInSendingCount() const;

#endif

private:
    std::atomic<uint32_t> mInSendingCnt = 0;

    LimiterLabel mLimiterLabel;

    uint32_t mMaxConcurrency = 0;
    uint32_t mMinConcurrency = 0;

    mutable std::mutex mCurrenctConcurrencyMux;
    uint32_t mCurrenctConcurrency = 0;

    uint32_t mMaxRetryIntervalSecs = 0;
    uint32_t mMinRetryIntervalSecs = 0;

    mutable std::mutex mIntervalMux;
    uint32_t mRetryIntervalSecs = 0;

    double mUpRatio = 0.0;
    double mDownRatio = 0.0;

    std::chrono::system_clock::time_point mLastCheckTime;
};

} // namespace logtail
