#pragma once


#include <memory>

#include "PromTaskFuture.h"

namespace logtail {
class BaseScheduler {
public:
    BaseScheduler() = default;
    virtual ~BaseScheduler() = default;

    virtual void ScheduleNext() = 0;

    void SetFirstExecTime(std::chrono::steady_clock::time_point firstExecTime) { mFirstExecTime = firstExecTime; }

    std::shared_ptr<PromTaskFuture> mFuture;

protected:
    std::chrono::steady_clock::time_point mFirstExecTime;
    int64_t mExecCount = 0;
};
} // namespace logtail