/*
 * Copyright 2023 iLogtail Authors
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
#include <typeinfo>
#include "models/PipelineEvent.h"
#include "models/LogEvent.h"
#include "models/MetricEvent.h"
#include "models/SpanEvent.h"

namespace logtail {

class PipelineEventPtr {
public:
    PipelineEventPtr(PipelineEvent* ptr) : mData(ptr) {}
    PipelineEventPtr(std::unique_ptr<PipelineEvent> ptr) : mData(ptr.release()) {}
    // default copy/move constructor is ok

    template <typename T>
    bool Is() const {
        if (typeid(T) == typeid(LogEvent)) {
            return mData->mType == LOG_EVENT_TYPE;
        }
        if (typeid(T) == typeid(MetricEvent)) {
            return mData->mType == METRIC_EVENT_TYPE;
        }
        if (typeid(T) == typeid(SpanEvent)) {
            return mData->mType == SPAN_EVENT_TYPE;
        }
        return false;
    }
    template <typename T>
    T& Cast() {
        return *static_cast<T*>(mData.get());
    }
    template <typename T>
    const T& Cast() const {
        return *static_cast<const T*>(mData.get());
    }
    template <typename T>
    T* Get() {
        return Is<T>() ? static_cast<T*>(mData.get()) : nullptr;
    }
    template <typename T>
    const T* Get() const {
        return Is<T>() ? static_cast<const T*>(mData.get()) : nullptr;
    }

private:
    std::shared_ptr<PipelineEvent> mData;
};

} // namespace logtail