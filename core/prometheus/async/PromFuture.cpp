#include "prometheus/async/PromFuture.h"

#include "common/Lock.h"

namespace logtail {

void PromFuture::Process(const HttpResponse& response) {
    if (IsCancelled()) {
        return;
    }
    for (auto& callback : mDoneCallbacks) {
        callback(response);
    }
}

void PromFuture::AddDoneCallback(std::function<void(const HttpResponse&)>&& callback) {
    mDoneCallbacks.emplace_back(std::move(callback));
}

void PromFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mValidState = false;
}

bool PromFuture::IsCancelled() {
    ReadLock lock(mStateRWLock);
    return mValidState;
}

} // namespace logtail