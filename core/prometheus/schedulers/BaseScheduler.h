#pragma once


#include <memory>

#include "prometheus/async/PromTaskFuture.h"

namespace logtail {
class BaseScheduler {
public:
    BaseScheduler() = default;
    virtual ~BaseScheduler() = default;

    virtual void ScheduleNext() = 0;

    void ExecDone();

    std::chrono::steady_clock::time_point GetNextExecTime();

    void SetFirstExecTime(std::chrono::steady_clock::time_point firstExecTime);

    void Cancel();

protected:
    std::chrono::steady_clock::time_point mFirstExecTime;
    int64_t mExecCount = 0;
    int64_t mInterval = 0;

    ReadWriteLock mLock;
    std::shared_ptr<PromTaskFuture> mFuture;
};
} // namespace logtail