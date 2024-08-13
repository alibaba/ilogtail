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
    if (cap + 1 == mCapacity) {
        return;
    }

    vector<unique_ptr<ProcessQueueItem>> tmp(cap + 1);
    size_t size = Size();
    if (cap < size) {
        LOG_WARNING(sLogger,
                    ("new circular process queue capacity is smaller than old queue size", "discard old data")(
                        "discard cnt", size - cap)("config", QueueKeyManager::GetInstance()->GetName(mKey)));
    }

    size_t begin = cap >= size ? mTail : (mTail + size - cap) % mCapacity;
    size_t index = 0;
    for (size_t i = begin; i != mHead; i = (i + 1) % mCapacity, ++index) {
        tmp[index] = std::move(mQueue[i]);
    }
    mTail = 0;
    mHead = index;
    mQueue.swap(tmp);
    ProcessQueueInterface::Reset();
    QueueInterface::Reset(cap + 1);
}

} // namespace logtail
