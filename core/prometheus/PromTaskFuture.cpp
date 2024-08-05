#include "prometheus/PromTaskFuture.h"

#include <cstdint>
#include <string>

#include "common/Lock.h"
#include "common/http/HttpRequest.h"
#include "prometheus/Mock.h"

namespace logtail {

void PromTaskFuture::Process(const HttpResponse& response) {
    for (auto& callback : mDoneCallbacks) {
        callback(response);
    }
}

void PromTaskFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mValidState = false;
}

bool PromTaskFuture::IsCancelled() {
    ReadLock lock(mStateRWLock);
    return mValidState;
}

PromHttpRequest::PromHttpRequest(const std::string& method,
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
                                 std::shared_ptr<Timer> timer)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body),
      mEvent(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mExecTime(execTime),
      mTimer(std::move(timer)) {
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    if (IsContextValid() && mTimer) {
        mTimer->PushEvent(BuildTimerEvent());
    }

    // ignore valid state, just process the response
    mEvent->Process(response);
}

[[nodiscard]] bool PromHttpRequest::IsContextValid() const {
    return mEvent->IsCancelled();
}

[[nodiscard]] std::unique_ptr<TimerEvent> PromHttpRequest::BuildTimerEvent() const {
    auto execTime = GetNextExecTime();
    auto request = std::make_unique<PromHttpRequest>(*this);
    request->SetNextExecTime(execTime);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));
    return timerEvent;
}

std::chrono::steady_clock::time_point PromHttpRequest::GetNextExecTime() const {
    // if timeout, use current time
    auto nextTime = mExecTime + std::chrono::seconds(mIntervalSeconds);
    if (nextTime < std::chrono::steady_clock::now()) {
        nextTime = std::chrono::steady_clock::now();
    }

    return nextTime;
}

void PromHttpRequest::SetNextExecTime(std::chrono::steady_clock::time_point execTime) {
    mExecTime = execTime;
}

} // namespace logtail