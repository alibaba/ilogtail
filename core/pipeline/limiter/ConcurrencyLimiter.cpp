// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pipeline/limiter/ConcurrencyLimiter.h"
#include "app_config/AppConfig.h"
#include "common/TimeUtil.h"

using namespace std;

namespace logtail {

bool ConcurrencyLimiter::IsValidToPop() {
    if (mLastSendTime == 0) {
        mLastSendTime = time(nullptr);
    }
    if (mConcurrency.load() > mInSendingCnt.load()) {
        return true;
    } else {
        time_t curTime = time(nullptr);
        if (curTime -  mLastSendTime > mRetryIntervalSeconds) {
            mLastSendTime = curTime;
            return true;
        }
    }
    return false;
}

void ConcurrencyLimiter::PostPop() {
    mInSendingCnt ++;
}

void ConcurrencyLimiter::OnDone() {
    -- mInSendingCnt;
}

void ConcurrencyLimiter::OnSuccess() {
    {
        lock_guard<mutex> lock(mConcurrencyMux);
        ++ mConcurrency;
        if (mConcurrency.load() != mMaxCocurrency) {
            mConcurrency = min(mMaxCocurrency, mConcurrency.load());
        }
    }
    {
        lock_guard<mutex> lock(mIntervalMux);
        if (mRetryIntervalSeconds.load() != mMinRetryIntervalSeconds) {
            mRetryIntervalSeconds = max(mMinRetryIntervalSeconds, static_cast<int>(mRetryIntervalSeconds.load() * mDownRatio));
        }
    }
}

void ConcurrencyLimiter::OnFail(time_t curTime) {
    {
        lock_guard<mutex> lock(mConcurrencyMux);
        if (mConcurrency.load() != mMinCocurrency) {
            mConcurrency = max(mMinCocurrency, static_cast<int>(mConcurrency.load() * mDownRatio));
        }
    }
    {
        lock_guard<mutex> lock(mIntervalMux);
        if (mRetryIntervalSeconds.load() != mMaxRetryIntervalSeconds) {
            mRetryIntervalSeconds = min(mMaxRetryIntervalSeconds, static_cast<int>(mRetryIntervalSeconds.load() * mUpRatio));
        }
    }
}

} // namespace logtail
