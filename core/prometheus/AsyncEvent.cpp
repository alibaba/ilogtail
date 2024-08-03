#include "prometheus/AsyncEvent.h"

#include <cstdint>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "common/http/HttpRequest.h"
#include "prometheus/Mock.h"

namespace logtail {

// TODO(liqiang): fix port
TickerHttpRequest::TickerHttpRequest(const std::string& method,
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
                                     std::shared_ptr<Timer> timer)
    : AsynHttpRequest(method, httpsFlag, host, port,url, query, header, body),
      mEvent(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mExecTime(execTime),
      mTimer(std::move(timer)),
      mHash(hash),
      mRWLock(rwLock),
      mContextSet(contextSet) {
}

void TickerHttpRequest::OnSendDone(const HttpResponse& response) {
    if (!IsContextValid()) {
        return;
    }
    mEvent->Process(response);
    if (IsContextValid() && mTimer) {
        mTimer->PushEvent(BuildTimerEvent());
    }
}
[[nodiscard]] bool TickerHttpRequest::IsContextValid() const {
    ReadLock lock(mRWLock);
    return mContextSet.count(mHash);
}

[[nodiscard]] std::unique_ptr<TimerEvent> TickerHttpRequest::BuildTimerEvent() const {
    auto execTime = GetNextExecTime();
    auto request = std::make_unique<TickerHttpRequest>(*this);
    request->SetNextExecTime(execTime);
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(execTime, std::move(request));
    return timerEvent;
}

std::chrono::steady_clock::time_point TickerHttpRequest::GetNextExecTime() const {
    return mExecTime + std::chrono::seconds(mIntervalSeconds);
}

void TickerHttpRequest::SetNextExecTime(std::chrono::steady_clock::time_point execTime) {
    mExecTime = execTime;
}


} // namespace logtail