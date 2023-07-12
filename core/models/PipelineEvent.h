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
#include <map>
#include <memory>
#include <string>
#include "models/StringView.h"
#include "reader/SourceBuffer.h"

namespace logtail {

enum PipelineEventType { VOID_EVENT_TYPE = 0, LOG_EVENT_TYPE = 1, METRIC_EVENT_TYPE = 2, SPAN_EVENT_TYPE = 3 };

class PipelineEventPtr;
class PipelineGroupEvents;
class PipelineEvent {
public:
    virtual ~PipelineEvent() {}
    virtual const std::string& GetType() const = 0;
    std::shared_ptr<SourceBuffer>& GetSourceBuffer() { return mSourceBuffer; }

    time_t GetTimestamp() const { return timestamp; }
    void SetTimestamp(time_t t) { timestamp = t; }
protected:
    void SetSourceBuffer(std::shared_ptr<SourceBuffer> sourceBuffer) { mSourceBuffer = sourceBuffer; }

    time_t timestamp = 0;
    PipelineEventType mType = VOID_EVENT_TYPE;
    std::shared_ptr<SourceBuffer> mSourceBuffer;
    friend class PipelineEventPtr;
    friend class PipelineGroupEvents;
};

extern StringView gEmptyStringView;

} // namespace logtail