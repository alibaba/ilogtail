/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "queue/QueueKey.h"

namespace logtail {

template <typename T>
class QueueInterface {
public:
    QueueInterface(QueueKey key, size_t cap) : mKey(key), mCapacity(cap) {}
    virtual ~QueueInterface() = default;

    QueueInterface(const QueueInterface& que) = delete;
    QueueInterface& operator=(const QueueInterface&) = delete;

    virtual bool Push(T&& item) = 0;
    virtual bool Pop(T& item) = 0;

    bool Empty() const { return Size() == 0; }
    size_t Capacity() const { return mCapacity; }

    QueueKey GetKey() const { return mKey; }

    void Reset(size_t cap) { mCapacity = cap; }

protected:
    const QueueKey mKey;
    size_t mCapacity = 0;

private:
    virtual size_t Size() const = 0;
};

} // namespace logtail