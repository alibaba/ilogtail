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

#ifdef APSARA_UNIT_TEST_MAIN
#include <sstream>
#endif

#include "common/HashUtil.h"
#include "logger/Logger.h"
#include "models/EventPool.h"
#ifdef APSARA_UNIT_TEST_MAIN
#include "plugin/processor/inner/ProcessorParseContainerLogNative.h"
#endif

using namespace std;

namespace logtail {

template <class T>
void DestroyEvents(vector<PipelineEventPtr>&& events) {
    unordered_map<EventPool*, vector<T*>> eventsPoolMap;
    for (auto& item : events) {
        if (item && item.IsFromEventPool()) {
            item->Reset();
            eventsPoolMap[item.GetEventPool()].emplace_back(static_cast<T*>(item.Release()));
        }
    }
    for (auto& item : eventsPoolMap) {
        if (item.first) {
            item.first->Release(std::move(item.second));
        } else {
            gThreadedEventPool.Release(std::move(item.second));
        }
    }
}

PipelineEventGroup::PipelineEventGroup(PipelineEventGroup&& rhs) noexcept
    : mMetadata(std::move(rhs.mMetadata)),
      mTags(std::move(rhs.mTags)),
      mEvents(std::move(rhs.mEvents)),
      mSourceBuffer(std::move(rhs.mSourceBuffer)) {
    for (auto& item : mEvents) {
        item->ResetPipelineEventGroup(this);
    }
}

PipelineEventGroup::~PipelineEventGroup() {
    if (mEvents.empty() || !mEvents[0]) {
        return;
    }
    switch (mEvents[0]->GetType()) {
        case PipelineEvent::Type::LOG:
            DestroyEvents<LogEvent>(std::move(mEvents));
            break;
        case PipelineEvent::Type::METRIC:
            DestroyEvents<MetricEvent>(std::move(mEvents));
            break;
        case PipelineEvent::Type::SPAN:
            DestroyEvents<SpanEvent>(std::move(mEvents));
            break;
        case PipelineEvent::Type::RAW:
            DestroyEvents<RawEvent>(std::move(mEvents));
            break;
        default:
            break;
    }
}

PipelineEventGroup& PipelineEventGroup::operator=(PipelineEventGroup&& rhs) noexcept {
    if (this != &rhs) {
        mMetadata = std::move(rhs.mMetadata);
        mTags = std::move(rhs.mTags);
        mEvents = std::move(rhs.mEvents);
        mSourceBuffer = std::move(rhs.mSourceBuffer);
        for (auto& item : mEvents) {
            item->ResetPipelineEventGroup(this);
        }
    }
    return *this;
}

PipelineEventGroup PipelineEventGroup::Copy() const {
    PipelineEventGroup res(mSourceBuffer);
    res.mMetadata = mMetadata;
    res.mTags = mTags;
    res.mExactlyOnceCheckpoint = mExactlyOnceCheckpoint;
    for (auto& event : mEvents) {
        res.mEvents.emplace_back(event.Copy());
        res.mEvents.back()->ResetPipelineEventGroup(&res);
    }
    return res;
}

unique_ptr<LogEvent> PipelineEventGroup::CreateLogEvent(bool fromPool, EventPool* pool) {
    LogEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireLogEvent(this);
        } else {
            e = gThreadedEventPool.AcquireLogEvent(this);
        }
    } else {
        e = new LogEvent(this);
    }
    return unique_ptr<LogEvent>(e);
}

unique_ptr<MetricEvent> PipelineEventGroup::CreateMetricEvent(bool fromPool, EventPool* pool) {
    MetricEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireMetricEvent(this);
        } else {
            e = gThreadedEventPool.AcquireMetricEvent(this);
        }
    } else {
        e = new MetricEvent(this);
    }
    return unique_ptr<MetricEvent>(e);
}

unique_ptr<SpanEvent> PipelineEventGroup::CreateSpanEvent(bool fromPool, EventPool* pool) {
    SpanEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireSpanEvent(this);
        } else {
            e = gThreadedEventPool.AcquireSpanEvent(this);
        }
    } else {
        e = new SpanEvent(this);
    }
    return unique_ptr<SpanEvent>(e);
}

unique_ptr<RawEvent> PipelineEventGroup::CreateRawEvent(bool fromPool, EventPool* pool) {
    RawEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireRawEvent(this);
        } else {
            e = gThreadedEventPool.AcquireRawEvent(this);
        }
    } else {
        e = new RawEvent(this);
    }
    return unique_ptr<RawEvent>(e);
}

LogEvent* PipelineEventGroup::AddLogEvent(bool fromPool, EventPool* pool) {
    LogEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireLogEvent(this);
        } else {
            e = gThreadedEventPool.AcquireLogEvent(this);
        }
    } else {
        e = new LogEvent(this);
    }
    mEvents.emplace_back(e, fromPool, pool);
    return e;
}

MetricEvent* PipelineEventGroup::AddMetricEvent(bool fromPool, EventPool* pool) {
    MetricEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireMetricEvent(this);
        } else {
            e = gThreadedEventPool.AcquireMetricEvent(this);
        }
    } else {
        e = new MetricEvent(this);
    }
    mEvents.emplace_back(e, fromPool, pool);
    return e;
}

SpanEvent* PipelineEventGroup::AddSpanEvent(bool fromPool, EventPool* pool) {
    SpanEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireSpanEvent(this);
        } else {
            e = gThreadedEventPool.AcquireSpanEvent(this);
        }
    } else {
        e = new SpanEvent(this);
    }
    mEvents.emplace_back(e, fromPool, pool);
    return e;
}

RawEvent* PipelineEventGroup::AddRawEvent(bool fromPool, EventPool* pool) {
    RawEvent* e = nullptr;
    if (fromPool) {
        if (pool) {
            e = pool->AcquireRawEvent(this);
        } else {
            e = gThreadedEventPool.AcquireRawEvent(this);
        }
    } else {
        e = new RawEvent(this);
    }
    mEvents.emplace_back(e, fromPool, pool);
    return e;
}

void PipelineEventGroup::SetMetadata(EventGroupMetaKey key, StringView val) {
    SetMetadataNoCopy(key, mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadata(EventGroupMetaKey key, const string& val) {
    SetMetadataNoCopy(key, mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadataNoCopy(EventGroupMetaKey key, const StringBuffer& val) {
    SetMetadataNoCopy(key, StringView(val.data, val.size));
}

bool PipelineEventGroup::HasMetadata(EventGroupMetaKey key) const {
    return mMetadata.find(key) != mMetadata.end();
}
void PipelineEventGroup::SetMetadataNoCopy(EventGroupMetaKey key, StringView val) {
    mMetadata[key] = val;
}

StringView PipelineEventGroup::GetMetadata(EventGroupMetaKey key) const {
    auto it = mMetadata.find(key);
    if (it != mMetadata.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

void PipelineEventGroup::DelMetadata(EventGroupMetaKey key) {
    mMetadata.erase(key);
}

void PipelineEventGroup::SetTag(StringView key, StringView val) {
    SetTagNoCopy(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetTag(const string& key, const string& val) {
    SetTagNoCopy(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetTag(const StringBuffer& key, StringView val) {
    SetTagNoCopy(key, mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

bool PipelineEventGroup::HasTag(StringView key) const {
    return mTags.mInner.find(key) != mTags.mInner.end();
}

void PipelineEventGroup::SetTagNoCopy(StringView key, StringView val) {
    mTags.Insert(key, val);
}

StringView PipelineEventGroup::GetTag(StringView key) const {
    auto it = mTags.mInner.find(key);
    if (it != mTags.mInner.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

void PipelineEventGroup::DelTag(StringView key) {
    mTags.Erase(key);
}

size_t PipelineEventGroup::GetTagsHash() const {
    size_t seed = 0;
    for (const auto& item : mTags.mInner) {
        HashCombine(seed, hash<string>{}(item.first.to_string()));
        HashCombine(seed, hash<string>{}(item.second.to_string()));
    }
    HashCombine(seed, hash<string>{}(GetMetadata(EventGroupMetaKey::SOURCE_ID).to_string()));
    return seed;
}

size_t PipelineEventGroup::DataSize() const {
    size_t eventsSize = sizeof(decltype(mEvents));
    for (const auto& item : mEvents) {
        eventsSize += item->DataSize();
    }
    return eventsSize + mTags.DataSize();
}

bool PipelineEventGroup::IsReplay() const {
    return mExactlyOnceCheckpoint != nullptr && mExactlyOnceCheckpoint->IsComplete();
}

#ifdef APSARA_UNIT_TEST_MAIN
const string EVENT_GROUP_META_LOG_FILE_PATH = "log.file.path";
const string EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED = "log.file.path_resolved";
const string EVENT_GROUP_META_LOG_FILE_INODE = "log.file.inode";
const string EVENT_GROUP_META_CONTAINER_TYPE = "container.type";
const string EVENT_GROUP_META_HAS_PART_LOG = "has.part.log";

const string EVENT_GROUP_META_K8S_CLUSTER_ID = "k8s.cluster.id";
const string EVENT_GROUP_META_K8S_NODE_NAME = "k8s.node.name";
const string EVENT_GROUP_META_K8S_NODE_IP = "k8s.node.ip";
const string EVENT_GROUP_META_K8S_NAMESPACE = "k8s.namespace.name";
const string EVENT_GROUP_META_K8S_POD_UID = "k8s.pod.uid";
const string EVENT_GROUP_META_K8S_POD_NAME = "k8s.pod.name";
const string EVENT_GROUP_META_CONTAINER_NAME = "container.name";
const string EVENT_GROUP_META_CONTAINER_IP = "container.ip";
const string EVENT_GROUP_META_CONTAINER_IMAGE_NAME = "container.image.name";
const string EVENT_GROUP_META_CONTAINER_IMAGE_ID = "container.image.id";

const string EVENT_GROUP_META_CONTAINERD_TEXT = "containerd_text";
const string EVENT_GROUP_META_DOCKER_JSON_FILE = "docker_json-file";
const string EVENT_GROUP_META_SOURCE_ID = "source.id";

const string& EventGroupMetaKeyToString(EventGroupMetaKey key) {
    switch (key) {
        case EventGroupMetaKey::LOG_FILE_PATH:
            return EVENT_GROUP_META_LOG_FILE_PATH;
        case EventGroupMetaKey::LOG_FILE_PATH_RESOLVED:
            return EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED;
        case EventGroupMetaKey::LOG_FILE_INODE:
            return EVENT_GROUP_META_LOG_FILE_INODE;
        case EventGroupMetaKey::SOURCE_ID:
            return EVENT_GROUP_META_SOURCE_ID;
        case EventGroupMetaKey::LOG_FORMAT:
            return EVENT_GROUP_META_CONTAINER_TYPE;
        case EventGroupMetaKey::HAS_PART_LOG:
            return EVENT_GROUP_META_HAS_PART_LOG;
        default:
            static string sEmpty = "unknown";
            return sEmpty;
    }
}

const string EventGroupMetaValueToString(string value) {
    if (value == ProcessorParseContainerLogNative::CONTAINERD_TEXT) {
        return EVENT_GROUP_META_CONTAINERD_TEXT;
    } else if (value == ProcessorParseContainerLogNative::DOCKER_JSON_FILE) {
        return EVENT_GROUP_META_DOCKER_JSON_FILE;
    }
    return value;
}

EventGroupMetaKey StringToEventGroupMetaKey(const string& key) {
    static unordered_map<string, EventGroupMetaKey> sStringToEnum{
        {EVENT_GROUP_META_LOG_FILE_PATH, EventGroupMetaKey::LOG_FILE_PATH},
        {EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED, EventGroupMetaKey::LOG_FILE_PATH_RESOLVED},
        {EVENT_GROUP_META_LOG_FILE_INODE, EventGroupMetaKey::LOG_FILE_INODE},
        {EVENT_GROUP_META_SOURCE_ID, EventGroupMetaKey::SOURCE_ID},
        {EVENT_GROUP_META_HAS_PART_LOG, EventGroupMetaKey::HAS_PART_LOG}};
    auto it = sStringToEnum.find(key);
    if (it != sStringToEnum.end()) {
        return it->second;
    }
    return EventGroupMetaKey::UNKNOWN;
}

Json::Value PipelineEventGroup::ToJson(bool enableEventMeta) const {
    Json::Value root;
    if (!mMetadata.empty()) {
        Json::Value metadata;
        for (const auto& meta : mMetadata) {
            metadata[EventGroupMetaKeyToString(meta.first)] = EventGroupMetaValueToString(meta.second.to_string());
        }
        root["metadata"] = metadata;
    }
    if (!mTags.mInner.empty()) {
        Json::Value tags;
        for (const auto& tag : mTags.mInner) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
        root["tags"] = tags;
    }
    if (!this->GetEvents().empty()) {
        Json::Value events;
        for (const auto& event : this->GetEvents()) {
            events.append(event->ToJson(enableEventMeta));
        }
        root["events"] = std::move(events);
    }
    return root;
}

bool PipelineEventGroup::FromJson(const Json::Value& root) {
    if (root.isMember("metadata")) {
        Json::Value metadata = root["metadata"];
        for (const auto& key : metadata.getMemberNames()) {
            SetMetadata(StringToEventGroupMetaKey(key), metadata[key].asString());
        }
    }
    if (root.isMember("tags")) {
        Json::Value tags = root["tags"];
        for (const auto& key : tags.getMemberNames()) {
            SetTag(key, tags[key].asString());
        }
    }
    if (root.isMember("events")) {
        Json::Value events = root["events"];
        for (const auto& event : events) {
            if (event["type"].asInt() == static_cast<int>(PipelineEvent::Type::LOG)) {
                AddLogEvent()->FromJson(event);
            } else if (event["type"].asInt() == static_cast<int>(PipelineEvent::Type::METRIC)) {
                AddMetricEvent()->FromJson(event);
            } else if (event["type"].asInt() == static_cast<int>(PipelineEvent::Type::SPAN)) {
                AddSpanEvent()->FromJson(event);
            } else {
                AddRawEvent()->FromJson(event);
            }
        }
    }
    return true;
}

string PipelineEventGroup::ToJsonString(bool enableEventMeta) const {
    Json::Value root = ToJson(enableEventMeta);
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    ostringstream oss;
    writer->write(root, &oss);
    return oss.str();
}

bool PipelineEventGroup::FromJsonString(const string& inJson) {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    unique_ptr<Json::CharReader> reader(builder.newCharReader());
    string errs;
    Json::Value root;
    if (!reader->parse(inJson.data(), inJson.data() + inJson.size(), &root, &errs)) {
        LOG_ERROR(sLogger, ("build PipelineEventGroup FromJsonString error", errs));
        return false;
    }
    return FromJson(root);
}
#endif

} // namespace logtail
