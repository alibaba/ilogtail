#pragma once

#include "common/Lock.h"
#include "common/http/HttpResponse.h"

namespace logtail {

class PromTaskFuture {
public:
    virtual ~PromTaskFuture() = default;

    // Process should support oneshot and streaming mode.
    void Process(const HttpResponse&);

    void AddDoneCallback(std::function<void(const HttpResponse&)>&& callback);

    void Cancel();
    bool IsCancelled();

protected:
    bool mValidState = true;
    ReadWriteLock mStateRWLock;

    std::vector<std::function<void(const HttpResponse&)>> mDoneCallbacks;
};

} // namespace logtail