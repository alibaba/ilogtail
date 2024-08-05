#pragma once


#include <memory>

#include "prometheus/async/PromTaskFuture.h"

namespace logtail {
class BaseScheduler {
public:
    BaseScheduler() = default;
    virtual ~BaseScheduler() = default;

    virtual void ScheduleNext() = 0;

    void ExecDone() { this->mExecCount++; }

    std::chrono::steady_clock::time_point GetNextExecTime() {
        return mFirstExecTime + std::chrono::seconds(mExecCount * mInterval);
    }

    void SetFirstExecTime(std::chrono::steady_clock::time_point firstExecTime) { mFirstExecTime = firstExecTime; }

    std::shared_ptr<PromTaskFuture> mFuture;

protected:
    std::chrono::steady_clock::time_point mFirstExecTime;
    int64_t mExecCount = 0;
    int64_t mInterval = 0;
};
} // namespace logtail