#pragma once
#include <cstdint>
#include <string>

#include "common/Lock.h"
#include "prometheus/Mock.h"


namespace logtail {

class PromTaskCallback {
public:
    virtual ~PromTaskCallback() = default;

    // Process should support oneshot and streaming mode.
    virtual void Process(const HttpResponse&) = 0;

    [[nodiscard]] virtual std::string GetId() const = 0;

    void Cancel(bool message);
    virtual bool IsCancelled() = 0;


protected:
    bool mValidState = true;
    ReadWriteLock mStateRWLock;
};

class PromMessageDispatcher {
public:
    static PromMessageDispatcher& GetInstance() {
        static PromMessageDispatcher sInstance;
        return sInstance;
    }

    void RegisterEvent(std::shared_ptr<PromTaskCallback> promEvent);
    void UnRegisterEvent(const std::string& promEventId);

    void SendMessage(const std::string& promEventId, bool message);

    void Stop();

private:
    PromMessageDispatcher() = default;
    ReadWriteLock mEventsRWLock;
    std::unordered_map<std::string, std::shared_ptr<PromTaskCallback>> mPromEvents;
};

class PromHttpRequest : public AsynHttpRequest {
public:
    PromHttpRequest(const std::string& method,
                    bool httpsFlag,
                    const std::string& host,
                    int32_t port,
                    const std::string& url,
                    const std::string& query,
                    const std::map<std::string, std::string>& header,
                    const std::string& body,
                    std::shared_ptr<PromTaskCallback> event,
                    uint64_t intervalSeconds,
                    std::chrono::steady_clock::time_point execTime,
                    std::shared_ptr<Timer> timer);
    PromHttpRequest(const PromHttpRequest&) = default;
    ~PromHttpRequest() override = default;

    void OnSendDone(const HttpResponse& response) override;
    [[nodiscard]] bool IsContextValid() const override;

private:
    [[nodiscard]] std::unique_ptr<TimerEvent> BuildTimerEvent() const;
    std::chrono::steady_clock::time_point GetNextExecTime() const;
    void SetNextExecTime(std::chrono::steady_clock::time_point execTime);

    std::shared_ptr<PromTaskCallback> mEvent;

    int64_t mIntervalSeconds;
    std::chrono::steady_clock::time_point mExecTime;
    std::shared_ptr<Timer> mTimer;
};

} // namespace logtail