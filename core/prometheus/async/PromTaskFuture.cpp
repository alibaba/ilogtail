#include "prometheus/async/PromTaskFuture.h"

#include "common/Lock.h"

namespace logtail {

void PromTaskFuture::Process(const HttpResponse& response) {
    if (IsCancelled()) {
        return;
    }
    for (auto& callback : mDoneCallbacks) {
        callback(response);
    }
}

void PromTaskFuture::AddDoneCallback(std::function<void(const HttpResponse&)>&& callback) {
    mDoneCallbacks.emplace_back(std::move(callback));
}

void PromTaskFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mValidState = false;
}

bool PromTaskFuture::IsCancelled() {
    ReadLock lock(mStateRWLock);
    return mValidState;
}

} // namespace logtail