#include "prometheus/AsyncEvent.h"

#include <cstdint>
#include <string>

#include "Logger.h"
#include "common/Lock.h"
#include "common/http/HttpRequest.h"
#include "prometheus/Mock.h"

namespace logtail {

void PromEvent::Send(bool message) {
    // this is a state that is not easy to change, so here we use a read-write lock
    {
        ReadLock lock(mStateRWLock);
        if (message == mValidState) {
            return;
        }
    }
    WriteLock lock(mStateRWLock);
    mValidState = message;
}

void PromMessageDispatcher::RegisterEvent(std::shared_ptr<PromEvent> promEvent) {
    WriteLock lock(mEventsRWLock);
    mPromEvents[promEvent->GetId()] = std::move(promEvent);
}

void PromMessageDispatcher::UnRegisterEvent(const std::string& promEventId) {
    WriteLock lock(mEventsRWLock);
    mPromEvents.erase(promEventId);
}

void PromMessageDispatcher::SendMessage(const std::string& promEventId, bool message) {
    std::shared_ptr<PromEvent> promEvent;
    {
        ReadLock lock(mEventsRWLock);
        auto it = mPromEvents.find(promEventId);
        if (it != mPromEvents.end()) {
            promEvent = it->second;
        }
    }
    if (promEvent) {
        promEvent->Send(message);
    } else {
        LOG_INFO(sLogger, ("PromEvent not found", promEventId));
    }
}

void PromMessageDispatcher::Stop() {
    WriteLock lock(mEventsRWLock);
    for (auto& it : mPromEvents) {
        it.second->Send(false);
    }
    mPromEvents.clear();
}

TickerHttpRequest::TickerHttpRequest(const std::string& method,
                                     bool httpsFlag,
                                     const std::string& host,
                                     int32_t port,
                                     const std::string& url,
                                     const std::string& query,
                                     const std::map<std::string, std::string>& header,
                                     const std::string& body,
                                     std::shared_ptr<PromEvent> event,
                                     uint64_t intervalSeconds,
                                     std::chrono::steady_clock::time_point execTime,
                                     std::shared_ptr<Timer> timer)
    : AsynHttpRequest(method, httpsFlag, host, port, url, query, header, body),
      mEvent(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mExecTime(execTime),
      mTimer(std::move(timer)) {
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
    return mEvent->ReciveMessage();
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