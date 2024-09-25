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

namespace logtail {

class ConcurrencyLimiter {
public:
    ConcurrencyLimiter() {}
    ConcurrencyLimiter(int maxCocurrency, int minCocurrency, int cocurrency) : mMaxCocurrency(maxCocurrency), mMinCocurrency(minCocurrency), mCocurrency(cocurrency) {}

    bool IsValidToPop();
    void PostPop();
    void OnDone();

    void OnSuccess();
    void OnFail(time_t curTime);

#ifdef APSARA_UNIT_TEST_MAIN
    void Reset() { mCocurrency.store(-1); }
    void SetLimit(int limit) { mCocurrency.store(limit); }
    int GetLimit() const { return mCocurrency.load(); }
    int GetCount() const { return mInSendingCnt.load(); }
    int GetInterval() const { return mRetryIntervalSecond.load(); }

#endif

private:
    double mUpRatio = 1.5;
    double mDownRatio = 0.5;

    std::atomic_int mInSendingCnt = 0;

    int mMaxCocurrency = 0;
    int mMinCocurrency = 0;
    std::atomic_int mCocurrency = 0;

    int mMaxRetryIntervalSecond = 3600;
    int mMinRetryIntervalSeconds = 30;
    std::atomic_int mRetryIntervalSecond = 30;

    time_t mLastSendTime = 0;

};

} // namespace logtail
