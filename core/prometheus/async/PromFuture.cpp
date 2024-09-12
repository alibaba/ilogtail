#include "prometheus/async/PromFuture.h"

#include "common/Lock.h"

namespace logtail {

void PromFuture::Process(const HttpResponse& response, uint64_t timestampMilliSec) {
    WriteLock lock(mStateRWLock);
    if (mState == PromFutureState::New) {
        for (auto& callback : mDoneCallbacks) {
            callback(response, timestampMilliSec);
        }
        mState = PromFutureState::Done;
    } else {
        return;
    }
}

bool PromFuture::PreCheck() {
    for (auto& callback : mPreCheckCallbacks) {
        if (!callback()) {
            return false;
        }
    }
    return true;
}

void PromFuture::AddDoneCallback(std::function<void(const HttpResponse&, uint64_t timestampMilliSec)>&& callback) {
    mDoneCallbacks.emplace_back(std::move(callback));
}

void PromFuture::AddPreCheckCallback(std::function<bool()>&& callback) {
    mPreCheckCallbacks.emplace_back(std::move(callback));
}

void PromFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mState = PromFutureState::Done;
}

} // namespace logtail