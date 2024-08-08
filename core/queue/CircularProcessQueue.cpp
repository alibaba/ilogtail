// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "queue/CircularProcessQueue.h"

#include "logger/Logger.h"
#include "queue/QueueKeyManager.h"

using namespace std;

namespace logtail {

bool CircularProcessQueue::Push(unique_ptr<ProcessQueueItem>&& item) {
    mQueue[mHead] = std::move(item);
    mHead = (mHead + 1) % mCapacity;
    if (mHead == mTail) {
        auto& tmp = mQueue[mHead];
        LOG_WARNING(sLogger,
                    ("circular buffer is full", "discard old data")(
                        "config", QueueKeyManager::GetInstance()->GetName(mKey))("input idx", tmp->mInputIndex));
        mTail = (mTail + 1) % mCapacity;
    }
    return true;
}

bool CircularProcessQueue::Pop(unique_ptr<ProcessQueueItem>& item) {
    if (!IsValidToPop()) {
        return false;
    }
    item = std::move(mQueue[mTail]);
    mTail = (mTail + 1) % mCapacity;
    return true;
}

size_t CircularProcessQueue::Size() const {
    return mHead >= mTail ? mHead - mTail : mCapacity - (mTail - mHead);
}

void CircularProcessQueue::Reset(size_t cap) {
    mQueue.resize(cap + 1);
    mQueue.clear();
    mHead = mTail = 0;
    ProcessQueueInterface::Reset();
    QueueInterface::Reset(cap + 1);
}

} // namespace logtail
