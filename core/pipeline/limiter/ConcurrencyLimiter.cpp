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
    if (mCocurrency > mInSendingCnt) {
        return true;
    } else {
        time_t curTime = time(nullptr);
        if (curTime -  mLastSendTime > mRetryIntervalSecond) {
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
    ++ mCocurrency;
    if (mCocurrency > mMaxCocurrency) {
        mCocurrency = mMaxCocurrency;
    }
    int oldValue;
    int newValue;
    do {
        oldValue = mRetryIntervalSecond.load(); // 读取当前值
        newValue = oldValue * mDownRatio;   // 计算新的值
    } while (!mRetryIntervalSecond.compare_exchange_strong(oldValue, newValue)); // 进行比较和交换
    if (mRetryIntervalSecond.load() < mMinRetryIntervalSeconds) {
        mRetryIntervalSecond.store(mMinRetryIntervalSeconds);
    }
}

void ConcurrencyLimiter::OnFail(time_t curTime) {
    int oldValue;
    int newValue;
    do {
        oldValue = mCocurrency.load(); // 读取当前值
        newValue = oldValue * mDownRatio;   // 计算新的值
    } while (!mCocurrency.compare_exchange_strong(oldValue, newValue)); // 进行比较和交换

    if (mCocurrency.load() < mMinCocurrency) {
        mCocurrency.store(mMinCocurrency);
        do {
            oldValue = mRetryIntervalSecond.load(); // 读取当前值
            newValue = oldValue * mUpRatio;   // 计算新的值
        } while (!mRetryIntervalSecond.compare_exchange_strong(oldValue, newValue)); // 进行比较和交换
    }
    if (mRetryIntervalSecond.load() > mMaxRetryIntervalSecond ) {
        mRetryIntervalSecond.store(mMaxRetryIntervalSecond);
    }
}

} // namespace logtail
