#pragma once

#include "common/Lock.h"
#include "common/http/HttpResponse.h"

namespace logtail {

enum class PromFutureState { New, Processing, Done };

class PromFuture {
public:
    // Process should support oneshot and streaming mode.
    void Process(const HttpResponse&, time_t timestamp);

    void AddDoneCallback(std::function<void(const HttpResponse&, time_t timestamp)>&& callback);

    void Cancel();

protected:
    PromFutureState mState = {PromFutureState::New};
    ReadWriteLock mStateRWLock;

    std::vector<std::function<void(const HttpResponse&, time_t timestamp)>> mDoneCallbacks;
};

} // namespace logtail