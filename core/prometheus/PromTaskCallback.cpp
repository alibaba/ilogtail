#include "prometheus/PromTaskCallback.h"

#include <cstdint>
#include <string>

#include "Logger.h"
#include "common/Lock.h"
#include "common/http/HttpRequest.h"
#include "prometheus/Mock.h"

namespace logtail {

void PromTaskCallback::Cancel(bool message) {
    // The state changes infrequently, so here we use a read-write lock
    {
        ReadLock lock(mStateRWLock);
        if (message == mValidState) {
            return;
        }
    }
    WriteLock lock(mStateRWLock);
    mValidState = message;
}

void PromMessageDispatcher::RegisterEvent(std::shared_ptr<PromTaskCallback> promEvent) {
    WriteLock lock(mEventsRWLock);
    mPromEvents[promEvent->GetId()] = std::move(promEvent);
}

void PromMessageDispatcher::UnRegisterEvent(const std::string& promEventId) {
    WriteLock lock(mEventsRWLock);
    mPromEvents.erase(promEventId);
}

void PromMessageDispatcher::SendMessage(const std::string& promEventId, bool message) {
    std::shared_ptr<PromTaskCallback> promEvent;
    {
        ReadLock lock(mEventsRWLock);
        auto it = mPromEvents.find(promEventId);
        if (it != mPromEvents.end()) {
            promEvent = it->second;
        }
    }
    if (promEvent) {
        promEvent->Cancel(message);
    } else {
        LOG_INFO(sLogger, ("PromEvent not found", promEventId));
    }
}

void PromMessageDispatcher::Stop() {
    WriteLock lock(mEventsRWLock);
    for (auto& it : mPromEvents) {
        it.second->Cancel(false);
    }
    mPromEvents.clear();
}

PromHttpRequest::PromHttpRequest(const std::string& method,
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
                                     std::shared_ptr<Timer> timer)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body),
      mEvent(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mExecTime(execTime),
      mTimer(std::move(timer)) {
}

void PromHttpRequest::OnSendDone(const HttpResponse& response) {
    if (IsContextValid() && mTimer) {
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