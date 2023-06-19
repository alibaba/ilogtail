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

#include "models/PipelineEventGroup.h"

namespace logtail {

void PipelineEventGroup::AddEvent(PipelineEventPtr event) {
    mEvents.emplace_back(event);
}

void PipelineEventGroup::SetMetadata(const StringView& key, const StringView& val) {
    SetMetadata(mSourceBuffer->CopyString(val), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadata(const std::string& key, const std::string& val) {
    SetMetadata(mSourceBuffer->CopyString(val), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadata(const StringBuffer& key, const StringBuffer& val) {
    SetMetadataNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

const StringView& PipelineEventGroup::GetMetadata(const std::string& key) const {
    return GetMetadata(StringView(key));
}

const StringView& PipelineEventGroup::GetMetadata(const char* key) const {
    return GetMetadata(StringView(key));
}

bool PipelineEventGroup::HasMetadata(const std::string& key) const {
    return HasMetadata(StringView(key));
}
bool PipelineEventGroup::HasMetadata(const StringView& key) const {
    return mGroup.metadata.find(key) != mGroup.metadata.end();
}
void PipelineEventGroup::SetMetadataNoCopy(const StringView& key, const StringView& val) {
    mGroup.metadata[key] = val;
}

const StringView& PipelineEventGroup::GetMetadata(const StringView& key) const {
    auto it = mGroup.metadata.find(key);
    if (it != mGroup.metadata.end()) {
        return it->second;
    }
    return gEmptyStringView;
}
} // namespace logtail