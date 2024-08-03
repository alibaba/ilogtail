#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "prometheus/Mock.h"
#include "sdk/Common.h"


namespace logtail {


class AsyncEvent {
public:
    virtual ~AsyncEvent() = default;
    virtual void Process(const HttpResponse&) = 0;
};


class AsyncEventDecorator : public AsyncEvent {
public:
    explicit AsyncEventDecorator(std::shared_ptr<AsyncEvent> event);
    ~AsyncEventDecorator() override = default;
    void Process(const HttpResponse& response) override;

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
    void Process(const HttpResponse& response) override;

private:
    uint64_t mIntervalSeconds;
    uint64_t mDeadlineNanoSeconds;
    std::shared_ptr<Timer> mTimer;

    ReadWriteLock& mRWLock;
    std::string mHash;
    std::unordered_set<std::string>& mValidationSet;
    [[nodiscard]] std::unique_ptr<TimerEvent> BuildTimerEvent() const;
    [[nodiscard]] bool IsValidation() const;
};

struct PromHttpRequest : public AsynHttpRequest {
    std::shared_ptr<TickerEvent> mTickerEvent;
    PromHttpRequest(const std::string& method,
                    bool httpsFlag,
                    const std::string& host,
                    const std::string& url,
                    const std::string& query,
                    const std::map<std::string, std::string>& header,
                    const std::string& body,
                    std::shared_ptr<TickerEvent> tickerEvent)
        : AsynHttpRequest(method, httpsFlag, host, url, query, header, body), mTickerEvent(std::move(tickerEvent)) {}
    void OnSendDone(const HttpResponse& response) override {
        mTickerEvent->Process(response);
    }
};

} // namespace logtail