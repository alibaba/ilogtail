#pragma once
#include <cstdint>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "prometheus/Mock.h"


namespace logtail {


class PromEvent {
public:
    virtual ~PromEvent() = default;

    // Process should support oneshot or streaming mode.
    virtual void Process(const HttpResponse&) = 0;
};

class TickerHttpRequest : public AsynHttpRequest {
public:
    TickerHttpRequest(const std::string& method,
                      bool httpsFlag,
                      const std::string& host,
                      int32_t port,
                      const std::string& url,
                      const std::string& query,
                      const std::map<std::string, std::string>& header,
                      const std::string& body,
                      const std::string& hash,
                      std::shared_ptr<PromEvent> event,
                      ReadWriteLock& rwLock,
                      std::unordered_set<std::string>& contextSet,
                      uint64_t intervalSeconds,
                      std::chrono::steady_clock::time_point execTime,
                      std::shared_ptr<Timer> timer);
    TickerHttpRequest(const TickerHttpRequest&) = default;
    ~TickerHttpRequest() override = default;

    void OnSendDone(const HttpResponse& response) override;
    [[nodiscard]] bool IsContextValid() const override;

private:
    [[nodiscard]] std::unique_ptr<TimerEvent> BuildTimerEvent() const;
    std::chrono::steady_clock::time_point GetNextExecTime() const;
    void SetNextExecTime(std::chrono::steady_clock::time_point execTime);

    std::shared_ptr<PromEvent> mEvent;

    int64_t mIntervalSeconds;
    std::chrono::steady_clock::time_point mExecTime;
    std::shared_ptr<Timer> mTimer;

    // mHash is used to identify the context
    std::string mHash;
    ReadWriteLock& mRWLock;
    std::unordered_set<std::string>& mContextSet;
};

} // namespace logtail