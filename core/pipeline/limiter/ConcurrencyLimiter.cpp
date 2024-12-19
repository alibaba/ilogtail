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

#include "common/StringTools.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {

#ifdef APSARA_UNIT_TEST_MAIN
uint32_t ConcurrencyLimiter::GetCurrentLimit() const {
    lock_guard<mutex> lock(mLimiterMux);
    return mCurrenctConcurrency;
}

uint32_t ConcurrencyLimiter::GetCurrentInterval() const {
    lock_guard<mutex> lock(mLimiterMux);
    return mRetryIntervalSecs;
}
void ConcurrencyLimiter::SetCurrentLimit(uint32_t limit) {
    lock_guard<mutex> lock(mLimiterMux);
    mCurrenctConcurrency = limit;
}

void ConcurrencyLimiter::SetInSendingCount(uint32_t count) {
    mInSendingCnt.store(count);
}
uint32_t ConcurrencyLimiter::GetInSendingCount() const {
    return mInSendingCnt.load();
}
#endif

bool ConcurrencyLimiter::IsValidToPop() {
    lock_guard<mutex> lock(mLimiterMux);
    if (mCurrenctConcurrency == 0) {
        auto curTime = std::chrono::system_clock::now();
        if (chrono::duration_cast<chrono::seconds>(curTime - mLastCheckTime).count() > mRetryIntervalSecs) {
            mLastCheckTime = curTime;
            return true;
        } else {
            return false;
        }
    }
    if (mCurrenctConcurrency > mInSendingCnt.load()) {
        return true;
    }
    return false;
}

void ConcurrencyLimiter::PostPop() {
    ++mInSendingCnt;
}

void ConcurrencyLimiter::OnSendDone() {
    --mInSendingCnt;
}

void ConcurrencyLimiter::OnSuccess(std::chrono::system_clock::time_point time) {
    AdjustConcurrency(true, time);
}

void ConcurrencyLimiter::OnFail(std::chrono::system_clock::time_point time) {
    AdjustConcurrency(false, time);
}



void ConcurrencyLimiter::Increase() {
    lock_guard<mutex> lock(mLimiterMux);
    if (mCurrenctConcurrency <= 0) {
        mRetryIntervalSecs = mMinRetryIntervalSecs;
        LOG_INFO(sLogger, ("reset send retry interval, type", mDescription));
    }
    if (mCurrenctConcurrency != mMaxConcurrency) {
        ++mCurrenctConcurrency;
         LOG_INFO(sLogger,
                     ("increase send concurrency", mDescription)("concurrency", mCurrenctConcurrency));
        if (mCurrenctConcurrency == mMaxConcurrency) {
            LOG_INFO(sLogger,
                     ("increase send concurrency to maximum, type", mDescription)("concurrency", mCurrenctConcurrency));
        } else {
            LOG_DEBUG(sLogger,
                      ("increase send concurrency, type",
                       mDescription)("from", mCurrenctConcurrency - 1)("to", mCurrenctConcurrency));
        }
    }
}

void ConcurrencyLimiter::Decrease(FallbackMode fallbackMode) {
    lock_guard<mutex> lock(mLimiterMux);
    switch (fallbackMode) {
        case (Fast):
            if (mCurrenctConcurrency != 0) {
                auto old = mCurrenctConcurrency;
                mCurrenctConcurrency = static_cast<uint32_t>(mCurrenctConcurrency * mConcurrencyDownRatio);
                LOG_INFO(sLogger, ("decrease send concurrency, type", mDescription)("from", old)("to", mCurrenctConcurrency));
            } else {
                if (mRetryIntervalSecs != mMaxRetryIntervalSecs) {
                    auto old = mRetryIntervalSecs;
                    mRetryIntervalSecs
                        = min(mMaxRetryIntervalSecs, static_cast<uint32_t>(mRetryIntervalSecs * mRetryIntervalUpRatio));
                    LOG_INFO(sLogger,
                            ("increase send retry interval, type",
                            mDescription)("from", ToString(old) + "s")("to", ToString(mRetryIntervalSecs) + "s"));
                }
            }
            break;
        case (Slow):
            if (mCurrenctConcurrency != 0) {
                mCurrenctConcurrency = mCurrenctConcurrency - 1;
                LOG_INFO(sLogger, ("decrease send concurrency, type", mDescription)("to", mCurrenctConcurrency));
            } else {
                mCurrenctConcurrency = 1;
                LOG_INFO(sLogger, ("decrease send concurrency to min, type", mDescription)("to", mCurrenctConcurrency));
            }
            break;
    }
}




void ConcurrencyLimiter::AdjustConcurrency(bool success, std::chrono::system_clock::time_point time) {
    lock_guard<mutex> lock(mStatisticsMux);
    mStatisticsTotal ++;
    if (!success) {
        mStatisticsFailTotal ++;
    }
    if (mLastStatisticsTime == std::chrono::system_clock::time_point()) {
        mLastStatisticsTime = time;
    }
    // 等10次，或者最大等10s，开始进行统计
    if (mStatisticsTotal == 10 || chrono::duration_cast<chrono::seconds>(time - mLastStatisticsTime).count() > 10) {
        uint32_t failPercentage = mStatisticsFailTotal*100/mStatisticsTotal;
        mStatisticsTotal = 0;
        mStatisticsFailTotal = 0;
        mLastStatisticsTime = time;
        if (failPercentage > 90) {
            // 不调整
            LOG_INFO(sLogger, ("failPercentage > 90, no adjust", ""));
        } else if (failPercentage > 70) {
            // 慢回退
            LOG_INFO(sLogger, ("failPercentage > 70, adjust slow", ""));
            Decrease(Slow);
        } else if (failPercentage > 0) {
            // 快速回退
            LOG_INFO(sLogger, ("failPercentage > 0, adjust fast", ""));
            Decrease(Fast);
        } else {
            // 成功
            LOG_INFO(sLogger, ("success, Increase", ""));
            Increase();
        }
    }
}


} // namespace logtail
