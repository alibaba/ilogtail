#pragma once

#include <future>
#include <memory>
#include <queue>

#include "common/http/HttpRequest.h"

namespace logtail {

class TimerEvent {
    friend bool operator<(const TimerEvent& lhs, const TimerEvent& rhs);

public:
    virtual ~TimerEvent() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;
    virtual bool Execute() = 0;

    [[nodiscard]] std::chrono::steady_clock::time_point GetExecTime() const { return mExecTime; }

private:
    std::chrono::steady_clock::time_point mExecTime;
};

bool operator<(const TimerEvent& lhs, const TimerEvent& rhs) {
    return lhs.mExecTime < rhs.mExecTime;
}

class Timer {
public:
    void Init();
    void Stop();
    void PushEvent(std::unique_ptr<TimerEvent>&& e);

private:
    void Run();

    mutable std::mutex mQueueMux;
    std::priority_queue<std::unique_ptr<TimerEvent>> mQueue;

    std::future<void> mThreadRes;
    mutable std::mutex mThreadRunningMux;
    bool mIsThreadRunning = true;
    mutable std::condition_variable mCV;
};

class HttpRequestTimerEvent : public TimerEvent {
public:
    [[nodiscard]] bool IsValid() const override;
    bool Execute() override;

    explicit HttpRequestTimerEvent(std::unique_ptr<AsynHttpRequest>&& request) : mRequest(std::move(request)) {}

private:
    std::unique_ptr<AsynHttpRequest> mRequest;
};

} // namespace logtail