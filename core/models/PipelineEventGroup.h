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
#include "models/PipelineEventPtr.h"
#include <memory>
#include <string>
#include "reader/SourceBuffer.h"

namespace logtail {

class GroupInfo {
public:
    std::map<StringView, StringView> metadata; // source/topic etc.
    std::map<StringView, StringView> tags;
};

// DeepCopy is required if we want to support no-linear topology
// We cannot just use default copy constructor as it won't deep copy PipelineEvent pointed in Events vector.
using EventsContainer = std::vector<PipelineEventPtr>;
class PipelineEventGroup {
public:
    // default constructors are ok
    const GroupInfo& GetGroupInfo() const { return mGroup; }
    // GroupInfo& ModifiableGroupInfo() { return mGroup; }
    const EventsContainer& GetEvents() const { return mEvents; }
    EventsContainer& ModifiableEvents() { return mEvents; }
    void AddEvent(PipelineEventPtr);
    void Swap(PipelineEventGroup& other) { std::swap(*this, other); }
    void SwapEvents(EventsContainer& other) { mEvents.swap(other); }
    void SetSourceBuffer(std::shared_ptr<SourceBuffer> sourceBuffer) { mSourceBuffer = sourceBuffer; }
    std::shared_ptr<SourceBuffer>& GetSourceBuffer() { return mSourceBuffer; }

    void SetMetadata(const StringView& key, const StringView& val);
    void SetMetadata(const std::string& key, const std::string& val);
    void SetMetadata(const StringBuffer& key, const StringBuffer& val);
    const StringView& GetMetadata(const std::string& key) const;
    const StringView& GetMetadata(const char* key) const;
    const StringView& GetMetadata(const StringView& key) const;
    bool HasMetadata(const std::string& key) const;
    bool HasMetadata(const StringView& key) const;
    void SetMetadataNoCopy(const StringView& key, const StringView& val);

private:
    GroupInfo mGroup;
    EventsContainer mEvents;
    std::shared_ptr<SourceBuffer> mSourceBuffer;
};

} // namespace logtail