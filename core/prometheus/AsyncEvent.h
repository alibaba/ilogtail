#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "sdk/Common.h"


namespace logtail {

struct ScrapeEvent {
    void* httpRequest = nullptr;
    std::function<void(const sdk::HttpMessage&)> mCallback;
    uint64_t mDeadline = 0UL;
};

class Timer {
public:
    void PushEvent(const ScrapeEvent&) {}
    void Start() {
        // start thread
    }
    void Stop() {
        // stop thread
    }

private:
};

class AsyncEvent {
public:
    virtual ~AsyncEvent() = default;
    virtual void Process(const sdk::HttpMessage&) = 0;
};


class AsyncEventDecorator : public AsyncEvent {
public:
    explicit AsyncEventDecorator(std::shared_ptr<AsyncEvent> event);
    ~AsyncEventDecorator() override = default;
    void Process(const sdk::HttpMessage& response) override;

protected:
    std::shared_ptr<AsyncEvent> mEvent;
};

class TickerEvent : public AsyncEventDecorator {
public:
    TickerEvent(std::shared_ptr<AsyncEvent> event,
                uint64_t intervalSeconds,
                uint64_t deadlineNanoSeconds,
                std::shared_ptr<Timer> timer,
                ReadWriteLock& mutex,
                std::unordered_set<std::string>& validationSet,
                std::string hash);
    ~TickerEvent() override = default;
    void Process(const sdk::HttpMessage& response) override;

private:
    uint64_t mIntervalSeconds;
    uint64_t mDeadlineNanoSeconds;
    std::shared_ptr<Timer> mTimer;

    ReadWriteLock& mRWLock;
    std::string mHash;
    std::unordered_set<std::string>& mValidationSet;
    [[nodiscard]] ScrapeEvent BuildAsyncEvent() const;
    [[nodiscard]] bool IsValidation() const;
};

} // namespace logtail