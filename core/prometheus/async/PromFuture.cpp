#include "prometheus/async/PromFuture.h"

#include "common/Lock.h"

namespace logtail {

void PromFuture::Process(const HttpResponse& response, uint64_t timestampNanoSec) {
    WriteLock lock(mStateRWLock);
    if (mState == PromFutureState::New) {
        for (auto& callback : mDoneCallbacks) {
            callback(response, timestampNanoSec);
        }
        mState = PromFutureState::Done;
    } else {
        return;
    }
}

void PromFuture::AddDoneCallback(std::function<void(const HttpResponse&, uint64_t timestampNanoSec)>&& callback) {
    mDoneCallbacks.emplace_back(std::move(callback));
}

void PromFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mState = PromFutureState::Done;
}

} // namespace logtail