#pragma once
#include <cstdint>
#include <string>

#include "prometheus/Mock.h"
#include "prometheus/async/PromTaskFuture.h"


namespace logtail {

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
                    std::shared_ptr<PromTaskFuture> event,
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

    std::shared_ptr<PromTaskFuture> mEvent;

    int64_t mIntervalSeconds;
    std::chrono::steady_clock::time_point mExecTime;
    std::shared_ptr<Timer> mTimer;
};

} // namespace logtail