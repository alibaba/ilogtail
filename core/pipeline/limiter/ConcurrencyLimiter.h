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

class ConcurrencyLimiter {
public:
    ConcurrencyLimiter() {}
    ConcurrencyLimiter(int maxCocurrency, int minCocurrency, int cocurrency, 
        int maxRetryIntervalSeconds = 3600, int minRetryIntervalSeconds = 30, int retryIntervalSeconds = 60, 
        double upRatio = 1.5, double downRatio = 0.5) : 
        mMaxCocurrency(maxCocurrency), mMinCocurrency(minCocurrency), mConcurrency(cocurrency),
        mMaxRetryIntervalSeconds(maxRetryIntervalSeconds), mMinRetryIntervalSeconds(minRetryIntervalSeconds), 
        mRetryIntervalSeconds(retryIntervalSeconds), mUpRatio(upRatio), mDownRatio(downRatio) {}

    bool IsValidToPop();
    void PostPop();
    void OnDone();

    void OnSuccess();
    void OnFail(time_t curTime);

#ifdef APSARA_UNIT_TEST_MAIN
    void Reset() { mConcurrency.store(-1); }
    void SetLimit(int limit) { mConcurrency.store(limit); }
    int GetLimit() const { return mConcurrency.load(); }
    int GetCount() const { return mInSendingCnt.load(); }
    int GetInterval() const { return mRetryIntervalSeconds.load(); }

#endif

private:
    std::atomic_int mInSendingCnt = 0;

    int mMaxCocurrency = 0;
    int mMinCocurrency = 0;

    mutable std::mutex mConcurrencyMux;
    std::atomic_int mConcurrency = 0;

    int mMaxRetryIntervalSeconds = 0;
    int mMinRetryIntervalSeconds = 0;

    mutable std::mutex mIntervalMux;
    std::atomic_int mRetryIntervalSeconds = 0;

    double mUpRatio = 0.0;
    double mDownRatio = 0.0;

    time_t mLastSendTime = 0;
};

} // namespace logtail
