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

#include "models/MetricEvent.h"

namespace logtail {

MetricEvent::MetricEvent(PipelineEventGroup* ptr) : PipelineEvent(Type::METRIC, ptr) {
}

void MetricEvent::SetName(const std::string& name) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(name);
    mName = StringView(b.data, b.size);
}

StringView MetricEvent::GetTag(StringView key) const {
    auto it = mTags.find(key);
    if (it != mTags.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool MetricEvent::HasTag(StringView key) const {
    return mTags.find(key) != mTags.end();
}

void MetricEvent::SetTag(StringView key, StringView val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void MetricEvent::SetTag(const std::string& key, const std::string& val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void MetricEvent::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

void MetricEvent::SetTagNoCopy(StringView key, StringView val) {
    mTags[key] = val;
}

void MetricEvent::DelTag(StringView key) {
    mTags.erase(key);
}

uint64_t MetricEvent::EventsSizeBytes() {
    // TODO
    return 0;
}

#ifdef APSARA_UNIT_TEST_MAIN
Json::Value MetricEvent::ToJson(bool enableEventMeta) const {
    Json::Value root;
    root["type"] = static_cast<int>(GetType());
    root["timestamp"] = GetTimestamp();
    root["timestampNanosecond"] = GetTimestampNanosecond();
    root["name"] = mName.to_string();
    root["value"] = MetricValueToJson(mValue);
    if (!mTags.empty()) {
        Json::Value& tags = root["tags"];
        for (const auto& tag : mTags) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
    }
    return root;
}

bool MetricEvent::FromJson(const Json::Value& root) {
    if (root.isMember("timestampNanosecond")) {
        SetTimestamp(root["timestamp"].asInt64(), root["timestampNanosecond"].asInt64());
    } else {
        SetTimestamp(root["timestamp"].asInt64());
    }
    SetName(root["name"].asString());
    const Json::Value& value = root["value"];
    SetValue(JsonToMetricValue(value["type"].asString(), value["detail"]));
    if (root.isMember("tags")) {
        Json::Value tags = root["tags"];
        for (const auto& key : tags.getMemberNames()) {
            SetTag(key, tags[key].asString());
        }
    }
    return true;
}
#endif

} // namespace logtail
