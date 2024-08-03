#include "prometheus/AsyncEvent.h"

#include <cstdint>
#include <string>
#include <unordered_set>

#include "common/Lock.h"
#include "prometheus/Mock.h"
#include "sdk/Common.h"

namespace logtail {


AsyncEventDecorator::AsyncEventDecorator(std::shared_ptr<AsyncEvent> event) : mEvent(std::move(event)) {
}
void AsyncEventDecorator::Process(const HttpResponse& response) {
    if (mEvent) {
        mEvent->Process(response);
    }
}

TickerEvent::TickerEvent(std::shared_ptr<AsyncEvent> event,
                         uint64_t intervalSeconds,
                         uint64_t deadlineNanoSeconds,
                         std::shared_ptr<Timer> timer,
                         ReadWriteLock& mutex,
                         std::unordered_set<std::string>& validationSet,
                         std::string hash)
    : AsyncEventDecorator(std::move(event)),
      mIntervalSeconds(intervalSeconds),
      mDeadlineNanoSeconds(deadlineNanoSeconds),
      mTimer(std::move(timer)),
      mRWLock(mutex),
      mHash(std::move(hash)),
      mValidationSet(validationSet) {
}
void TickerEvent::Process(const HttpResponse& response) {
    AsyncEventDecorator::Process(response);

    if (IsValidation() && mTimer) {
        mTimer->PushEvent(BuildTimerEvent());
    }
}
[[nodiscard]] std::unique_ptr<TimerEvent> TickerEvent::BuildTimerEvent() const {
    auto request = std::make_unique<PromHttpRequest>();
    auto timerEvent = std::make_unique<HttpRequestTimerEvent>(std::move(request));
    uint64_t deadlineNanoSeconds = mDeadlineNanoSeconds + mIntervalSeconds * 1000000000UL;
    auto tickerEvent
        = TickerEvent(mEvent, mIntervalSeconds, deadlineNanoSeconds, mTimer, mRWLock, mValidationSet, mHash);
    return timerEvent;
}
[[nodiscard]] bool TickerEvent::IsValidation() const {
    ReadLock lock(mRWLock);
    return mValidationSet.count(mHash);
}


} // namespace logtail