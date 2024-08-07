#include "prometheus/async/PromFuture.h"

#include "common/Lock.h"

namespace logtail {

void PromFuture::Process(const HttpResponse& response) {
    {
        WriteLock lock(mStateRWLock);
        if (mState == PromFutureState::New) {
            mState = PromFutureState::Processing;
        } else {
            return;
        }
    }
    {
        WriteLock lock(mStateRWLock);
        for (auto& callback : mDoneCallbacks) {
            callback(response);
        }
        mState = PromFutureState::Done;
    }
}

void PromFuture::AddDoneCallback(std::function<void(const HttpResponse&)>&& callback) {
    mDoneCallbacks.emplace_back(std::move(callback));
}

void PromFuture::Cancel() {
    WriteLock lock(mStateRWLock);
    mState = PromFutureState::Done;
}

} // namespace logtail