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

#include "models/SpanEvent.h"

using namespace std;

namespace logtail {

void SpanEvent::SpanLink::SetTraceId(const string& traceId) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(traceId);
    mTraceId = StringView(b.data, b.size);
}

void SpanEvent::SpanLink::SetSpanId(const string& spanId) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(spanId);
    mSpanId = StringView(b.data, b.size);
}

void SpanEvent::SpanLink::SetTraceState(const string& traceState) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(traceState);
    mTraceState = StringView(b.data, b.size);
}

StringView SpanEvent::SpanLink::GetTag(StringView key) const {
    auto it = mTags.mInner.find(key);
    if (it != mTags.mInner.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool SpanEvent::SpanLink::HasTag(StringView key) const {
    return mTags.mInner.find(key) != mTags.mInner.end();
}

void SpanEvent::SpanLink::SetTag(StringView key, StringView val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SpanLink::SetTag(const string& key, const string& val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SpanLink::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

void SpanEvent::SpanLink::SetTagNoCopy(StringView key, StringView val) {
    mTags.Insert(key, val);
}

void SpanEvent::SpanLink::DelTag(StringView key) {
    mTags.Erase(key);
}

shared_ptr<SourceBuffer>& SpanEvent::SpanLink::GetSourceBuffer() {
    return mParent->GetSourceBuffer();
}

size_t SpanEvent::SpanLink::DataSize() const {
    return mTraceId.size() + mSpanId.size() + mTraceState.size() + mTags.DataSize();
}

#ifdef APSARA_UNIT_TEST_MAIN
Json::Value SpanEvent::SpanLink::ToJson() const {
    Json::Value root;
    root["traceId"] = mTraceId.to_string();
    root["spanId"] = mSpanId.to_string();
    if (!mTraceState.empty()) {
        root["traceState"] = mTraceState.to_string();
    }
    if (!mTags.mInner.empty()) {
        Json::Value& tags = root["tags"];
        for (const auto& tag : mTags.mInner) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
    }
    return root;
}

void SpanEvent::SpanLink::FromJson(const Json::Value& value) {
    SetTraceId(value["traceId"].asString());
    SetSpanId(value["spanId"].asString());
    if (value.isMember("traceState")) {
        string s = value["traceState"].asString();
        if (!s.empty()) {
            SetTraceState(s);
        }
    }
    if (value.isMember("tags")) {
        Json::Value tags = value["tags"];
        for (const auto& key : tags.getMemberNames()) {
            SetTag(key, tags[key].asString());
        }
    }
}
#endif

void SpanEvent::InnerEvent::SetName(const string& name) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(name);
    mName = StringView(b.data, b.size);
}

StringView SpanEvent::InnerEvent::GetTag(StringView key) const {
    auto it = mTags.mInner.find(key);
    if (it != mTags.mInner.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool SpanEvent::InnerEvent::HasTag(StringView key) const {
    return mTags.mInner.find(key) != mTags.mInner.end();
}

void SpanEvent::InnerEvent::SetTag(StringView key, StringView val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::InnerEvent::SetTag(const string& key, const string& val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::InnerEvent::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

void SpanEvent::InnerEvent::SetTagNoCopy(StringView key, StringView val) {
    mTags.Insert(key, val);
}

void SpanEvent::InnerEvent::DelTag(StringView key) {
    mTags.Erase(key);
}

shared_ptr<SourceBuffer>& SpanEvent::InnerEvent::GetSourceBuffer() {
    return mParent->GetSourceBuffer();
}

size_t SpanEvent::InnerEvent::DataSize() const {
    return sizeof(decltype(mTimestampNs)) + mName.size() + mTags.DataSize();
}

#ifdef APSARA_UNIT_TEST_MAIN
Json::Value SpanEvent::InnerEvent::ToJson() const {
    Json::Value root;
    root["name"] = mName.to_string();
    root["timestampNs"] = static_cast<int64_t>(mTimestampNs);
    if (!mTags.mInner.empty()) {
        Json::Value& tags = root["tags"];
        for (const auto& tag : mTags.mInner) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
    }
    return root;
}

void SpanEvent::InnerEvent::FromJson(const Json::Value& value) {
    SetName(value["name"].asString());
    SetTimestampNs(value["timestampNs"].asUInt64());
    if (value.isMember("tags")) {
        Json::Value tags = value["tags"];
        for (const auto& key : tags.getMemberNames()) {
            SetTag(key, tags[key].asString());
        }
    }
}
#endif

const string SpanEvent::OTLP_SCOPE_NAME = "otlp.scope.name";
const string SpanEvent::OTLP_SCOPE_VERSION = "otlp.scope.version";

SpanEvent::SpanEvent(PipelineEventGroup* ptr) : PipelineEvent(Type::SPAN, ptr) {
}

void SpanEvent::SetTraceId(const string& traceId) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(traceId);
    mTraceId = StringView(b.data, b.size);
}

void SpanEvent::SetSpanId(const string& spanId) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(spanId);
    mSpanId = StringView(b.data, b.size);
}

void SpanEvent::SetTraceState(const string& traceState) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(traceState);
    mTraceState = StringView(b.data, b.size);
}

void SpanEvent::SetParentSpanId(const string& parentSpanId) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(parentSpanId);
    mParentSpanId = StringView(b.data, b.size);
}

void SpanEvent::SetName(const string& name) {
    const StringBuffer& b = GetSourceBuffer()->CopyString(name);
    mName = StringView(b.data, b.size);
}

StringView SpanEvent::GetTag(StringView key) const {
    auto it = mTags.mInner.find(key);
    if (it != mTags.mInner.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool SpanEvent::HasTag(StringView key) const {
    return mTags.mInner.find(key) != mTags.mInner.end();
}

void SpanEvent::SetTag(StringView key, StringView val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SetTag(const string& key, const string& val) {
    SetTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

void SpanEvent::SetTagNoCopy(StringView key, StringView val) {
    mTags.Insert(key, val);
}

void SpanEvent::DelTag(StringView key) {
    mTags.Erase(key);
}

SpanEvent::InnerEvent* SpanEvent::AddEvent() {
    SpanEvent::InnerEvent e(this);
    mEvents.emplace_back(std::move(e));
    return &mEvents.back();
}

SpanEvent::SpanLink* SpanEvent::AddLink() {
    SpanEvent::SpanLink l(this);
    mLinks.emplace_back(std::move(l));
    return &mLinks.back();
}

StringView SpanEvent::GetScopeTag(StringView key) const {
    auto it = mScopeTags.mInner.find(key);
    if (it != mScopeTags.mInner.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

bool SpanEvent::HasScopeTag(StringView key) const {
    return mScopeTags.mInner.find(key) != mScopeTags.mInner.end();
}

void SpanEvent::SetScopeTag(StringView key, StringView val) {
    SetScopeTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SetScopeTag(const string& key, const string& val) {
    SetScopeTagNoCopy(GetSourceBuffer()->CopyString(key), GetSourceBuffer()->CopyString(val));
}

void SpanEvent::SetScopeTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetScopeTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

void SpanEvent::SetScopeTagNoCopy(StringView key, StringView val) {
    mScopeTags.Insert(key, val);
}

void SpanEvent::DelScopeTag(StringView key) {
    mScopeTags.Erase(key);
}

size_t SpanEvent::DataSize() const {
    // TODO: this is not O(1), however, these two fields are not frequently used, so it can thought of O(1)
    size_t eventsSize = sizeof(decltype(mEvents));
    for (const auto& item : mEvents) {
        eventsSize += item.DataSize();
    }
    size_t linksSize = sizeof(decltype(mLinks));
    for (const auto& item : mLinks) {
        linksSize += item.DataSize();
    }
    // TODO: for enum, it seems more reasonable to use actual string size instead of size of enum
    return PipelineEvent::DataSize() + mTraceId.size() + mSpanId.size() + mTraceState.size() + mParentSpanId.size()
        + mName.size() + sizeof(decltype(mKind)) + sizeof(decltype(mStartTimeNs)) + sizeof(decltype(mEndTimeNs))
        + mTags.DataSize() + eventsSize + linksSize + sizeof(decltype(mStatus)) + mScopeTags.DataSize();
}

#ifdef APSARA_UNIT_TEST_MAIN
Json::Value SpanEvent::ToJson(bool enableEventMeta) const {
    Json::Value root;
    root["type"] = static_cast<int>(GetType());
    root["timestamp"] = GetTimestamp();
    if (GetTimestampNanosecond()) {
        root["timestampNanosecond"] = static_cast<int32_t>(GetTimestampNanosecond().value());
    }
    root["traceId"] = mTraceId.to_string();
    root["spanId"] = mSpanId.to_string();
    if (!mTraceState.empty()) {
        root["traceState"] = mTraceState.to_string();
    }
    if (!mParentSpanId.empty()) {
        root["parentSpanId"] = mParentSpanId.to_string();
    }
    root["name"] = mName.to_string();
    if (mKind != Kind::Unspecified) {
        root["kind"] = static_cast<int>(mKind);
    }
    // must be int since jsoncpp will take integral as int during parse, which
    // will lead to inequality on json comparison
    root["startTimeNs"] = static_cast<int64_t>(mStartTimeNs);
    root["endTimeNs"] = static_cast<int64_t>(mEndTimeNs);
    if (!mTags.mInner.empty()) {
        Json::Value& tags = root["tags"];
        for (const auto& tag : mTags.mInner) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
    }
    if (!mEvents.empty()) {
        Json::Value& events = root["events"];
        for (const auto& event : mEvents) {
            events.append(event.ToJson());
        }
    }
    if (!mLinks.empty()) {
        Json::Value& links = root["links"];
        for (const auto& link : mLinks) {
            links.append(link.ToJson());
        }
    }
    if (mStatus != StatusCode::Unset) {
        root["status"] = static_cast<int>(mStatus);
    }
    if (!mScopeTags.mInner.empty()) {
        Json::Value& tags = root["scopeTags"];
        for (const auto& tag : mScopeTags.mInner) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
    }
    return root;
}

bool SpanEvent::FromJson(const Json::Value& root) {
    if (root.isMember("timestampNanosecond")) {
        SetTimestamp(root["timestamp"].asInt64(), root["timestampNanosecond"].asInt64());
    } else {
        SetTimestamp(root["timestamp"].asInt64());
    }
    SetTraceId(root["traceId"].asString());
    SetSpanId(root["spanId"].asString());
    if (root.isMember("traceState")) {
        string s = root["traceState"].asString();
        if (!s.empty()) {
            SetTraceState(s);
        }
    }
    if (root.isMember("parentSpanId")) {
        string s = root["parentSpanId"].asString();
        if (!s.empty()) {
            SetParentSpanId(s);
        }
    }
    SetName(root["name"].asString());
    if (root.isMember("kind")) {
        SetKind(static_cast<Kind>(root["kind"].asInt()));
    }
    SetStartTimeNs(root["startTimeNs"].asUInt64());
    SetEndTimeNs(root["endTimeNs"].asUInt64());
    if (root.isMember("tags")) {
        Json::Value tags = root["tags"];
        for (const auto& key : tags.getMemberNames()) {
            SetTag(key, tags[key].asString());
        }
    }
    if (root.isMember("events")) {
        Json::Value events = root["events"];
        for (const auto& event : events) {
            InnerEvent* e = AddEvent();
            e->FromJson(event);
        }
    }
    if (root.isMember("links")) {
        Json::Value links = root["links"];
        for (const auto& link : links) {
            SpanLink* l = AddLink();
            l->FromJson(link);
        }
    }
    if (root.isMember("status")) {
        SetStatus(static_cast<StatusCode>(root["status"].asInt()));
    }
    if (root.isMember("scopeTags")) {
        Json::Value tags = root["scopeTags"];
        for (const auto& key : tags.getMemberNames()) {
            SetScopeTag(key, tags[key].asString());
        }
    }
    return true;
}
#endif

} // namespace logtail
