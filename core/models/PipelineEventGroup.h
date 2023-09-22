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

using GroupMetadata = std::map<StringView, StringView>;
using GroupTags = std::map<StringView, StringView>;

// DeepCopy is required if we want to support no-linear topology
// We cannot just use default copy constructor as it won't deep copy PipelineEvent pointed in Events vector.
using EventsContainer = std::vector<PipelineEventPtr>;
class PipelineEventGroup {
public:
    PipelineEventGroup(std::shared_ptr<SourceBuffer> sourceBuffer) : mSourceBuffer(sourceBuffer) {}
    PipelineEventGroup(const PipelineEventGroup&) = delete;
    PipelineEventGroup& operator=(const PipelineEventGroup&) = delete;
    const EventsContainer& GetEvents() const { return mEvents; }
    EventsContainer& MutableEvents() { return mEvents; }
    void AddEvent(const PipelineEventPtr& event);
    void AddEvent(std::unique_ptr<PipelineEvent>&& event);
    void SwapEvents(EventsContainer& other) { mEvents.swap(other); }
    // void SetSourceBuffer(std::shared_ptr<SourceBuffer> sourceBuffer) { mSourceBuffer = sourceBuffer; }
    std::shared_ptr<SourceBuffer>& GetSourceBuffer() { return mSourceBuffer; }

    void SetMetadata(const StringView& key, const StringView& val);
    void SetMetadata(const std::string& key, const std::string& val);
    void SetMetadataNoCopy(const StringBuffer& key, const StringBuffer& val);
    const StringView& GetMetadata(const StringView& key) const;
    const GroupMetadata& GetMetadatum() const { return mMetadata; };
    bool HasMetadata(const StringView& key) const;
    void SetMetadataNoCopy(const StringView& key, const StringView& val);
    void DelMetadata(const StringView& key);

    void SetTag(const StringView& key, const StringView& val);
    void SetTag(const std::string& key, const std::string& val);
    void SetTagNoCopy(const StringBuffer& key, const StringBuffer& val);
    const StringView& GetTag(const StringView& key) const;
    const GroupTags& GetTags() const { return mTags; };
    bool HasTag(const StringView& key) const;
    void SetTagNoCopy(const StringView& key, const StringView& val);
    void DelTag(const StringView& key);

    // for debug and test
    Json::Value ToJson() const;
    bool FromJson(const Json::Value&);
    std::string ToJsonString() const;
    bool FromJsonString(const std::string&);
    uint64_t EventGroupSizeBytes();

private:
    GroupMetadata mMetadata; // Keys are EVENT_META_XXX in Constants.h. Used to generate tag/log. Will not output.
    GroupTags mTags; // custom tags to output
    EventsContainer mEvents;
    std::shared_ptr<SourceBuffer> mSourceBuffer;
};

} // namespace logtail