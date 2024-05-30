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

#include <map>
#include <string>

#include "common/memory/SourceBuffer.h"
#include "models/MetricValue.h"
#include "models/PipelineEvent.h"

namespace logtail {

class MetricEvent : public PipelineEvent {
    friend class PipelineEventGroup;

public:
    StringView GetName() const { return mName; }
    void SetName(const std::string& name);

    template <typename T>
    bool Is() const {
        return std::holds_alternative<T>(mValue);
    }

    template <typename T>
    constexpr std::add_pointer_t<const T> GetValue() const noexcept {
        return std::get_if<T>(&mValue);
    }

    template <typename T>
    void SetValue(const T& value) {
        mValue = value;
    }

    template <typename T, typename... Args>
    void SetValue(Args&&... args) {
        mValue = T{std::forward<Args>(args)...};
    }

    StringView GetTag(StringView key) const;
    bool HasTag(StringView key) const;
    void SetTag(StringView key, StringView val);
    void SetTag(const std::string& key, const std::string& val);
    void SetTagNoCopy(const StringBuffer& key, const StringBuffer& val);
    void SetTagNoCopy(StringView key, StringView val);
    void DelTag(StringView key);

    uint64_t EventsSizeBytes() override;

#ifdef APSARA_UNIT_TEST_MAIN
    Json::Value ToJson(bool enableEventMeta = false) const override;
    bool FromJson(const Json::Value&) override;
#endif

private:
    MetricEvent(PipelineEventGroup* ptr);

    StringView mName;
    MetricValue mValue;
    std::map<StringView, StringView> mTags;
};

} // namespace logtail
