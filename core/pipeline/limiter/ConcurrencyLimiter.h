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


using namespace std;

namespace logtail {

class ConcurrencyLimiter {
public:
    ConcurrencyLimiter() {}
    ConcurrencyLimiter(uint32_t maxCocurrency, uint32_t minCocurrency, uint32_t cocurrency, 
        uint32_t maxRetryIntervalSeconds = 3600, uint32_t minRetryIntervalSeconds = 30, uint32_t retryIntervalSeconds = 60, 
        double upRatio = 1.5, double downRatio = 0.5) : 
        mMaxCocurrency(maxCocurrency), mMinCocurrency(minCocurrency), mConcurrency(cocurrency),
        mMaxRetryIntervalSeconds(maxRetryIntervalSeconds), mMinRetryIntervalSeconds(minRetryIntervalSeconds), 
        mRetryIntervalSeconds(retryIntervalSeconds), mUpRatio(upRatio), mDownRatio(downRatio) {}

    bool IsValidToPop();
    void PostPop();
    void OnDone();

    void OnSuccess();
    void OnFail(time_t curTime);

    uint32_t GetLimit() const { 
        lock_guard<mutex> lock(mConcurrencyMux);
        return mConcurrency; 
    }

    uint32_t GetInterval() const { 
        lock_guard<mutex> lock(mIntervalMux);
        return mRetryIntervalSeconds; 
    }

#ifdef APSARA_UNIT_TEST_MAIN
    void SetLimit(int limit) { 
        lock_guard<mutex> lock(mConcurrencyMux);
        mConcurrency = limit;
    }

    void SetSendingCount(int count) {
        mInSendingCnt.store(count);
    }
    int GetSendingCount() const { return mInSendingCnt.load(); }

#endif

private:
    std::atomic_int mInSendingCnt = 0;

    uint32_t mMaxCocurrency = 0;
    uint32_t mMinCocurrency = 0;

    mutable std::mutex mConcurrencyMux;
    uint32_t mConcurrency = 0;

    uint32_t mMaxRetryIntervalSeconds = 0;
    uint32_t mMinRetryIntervalSeconds = 0;

    mutable std::mutex mIntervalMux;
    uint32_t mRetryIntervalSeconds = 0;

    double mUpRatio = 0.0;
    double mDownRatio = 0.0;

    time_t mLastSendTime = 0;
};

} // namespace logtail
