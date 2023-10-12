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

#include <sstream>

#include "logger/Logger.h"

namespace logtail {

void PipelineEventGroup::AddEvent(const PipelineEventPtr& event) {
    mEvents.emplace_back(event);
}

void PipelineEventGroup::AddEvent(std::unique_ptr<PipelineEvent>&& event) {
    mEvents.emplace_back(std::move(event));
}

void PipelineEventGroup::SetMetadata(EventGroupMetaKey key, const StringView& val) {
    SetMetadataNoCopy(key, mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadata(EventGroupMetaKey key, const std::string& val) {
    SetMetadataNoCopy(key, mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetMetadataNoCopy(EventGroupMetaKey key, const StringBuffer& val) {
    SetMetadataNoCopy(key, StringView(val.data, val.size));
}

bool PipelineEventGroup::HasMetadata(EventGroupMetaKey key) const {
    return mMetadata.find(key) != mMetadata.end();
}
void PipelineEventGroup::SetMetadataNoCopy(EventGroupMetaKey key, const StringView& val) {
    mMetadata[key] = val;
}

const StringView& PipelineEventGroup::GetMetadata(EventGroupMetaKey key) const {
    auto it = mMetadata.find(key);
    if (it != mMetadata.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

void PipelineEventGroup::DelMetadata(EventGroupMetaKey key) {
    mMetadata.erase(key);
}

void PipelineEventGroup::SetTag(const StringView& key, const StringView& val) {
    SetTagNoCopy(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetTag(const std::string& key, const std::string& val) {
    SetTagNoCopy(mSourceBuffer->CopyString(key), mSourceBuffer->CopyString(val));
}

void PipelineEventGroup::SetTagNoCopy(const StringBuffer& key, const StringBuffer& val) {
    SetTagNoCopy(StringView(key.data, key.size), StringView(val.data, val.size));
}

bool PipelineEventGroup::HasTag(const StringView& key) const {
    return mTags.find(key) != mTags.end();
}

void PipelineEventGroup::SetTagNoCopy(const StringView& key, const StringView& val) {
    mTags[key] = val;
}

const StringView& PipelineEventGroup::GetTag(const StringView& key) const {
    auto it = mTags.find(key);
    if (it != mTags.end()) {
        return it->second;
    }
    return gEmptyStringView;
}

void PipelineEventGroup::DelTag(const StringView& key) {
    mTags.erase(key);
}

uint64_t PipelineEventGroup::EventGroupSizeBytes() {
    // TODO
    return 0;
}

#ifdef APSARA_UNIT_TEST_MAIN
const std::string EVENT_GROUP_META_AGENT_TAG = "agent.tag";
const std::string EVENT_GROUP_META_HOST_IP = "host.ip";
const std::string EVENT_GROUP_META_HOST_NAME = "host.name";
const std::string EVENT_GROUP_META_LOG_TOPIC = "log.topic";
const std::string EVENT_GROUP_META_LOG_FILE_PATH = "log.file.path";
const std::string EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED = "log.file.path_resolved";
const std::string EVENT_GROUP_META_LOG_FILE_INODE = "log.file.inode";
const std::string EVENT_GROUP_META_LOG_FILE_OFFSET = "log.file.offset";
const std::string EVENT_GROUP_META_LOG_FILE_LENGTH = "log.file.length";

const std::string EVENT_GROUP_META_K8S_CLUSTER_ID = "k8s.cluster.id";
const std::string EVENT_GROUP_META_K8S_NODE_NAME = "k8s.node.name";
const std::string EVENT_GROUP_META_K8S_NODE_IP = "k8s.node.ip";
const std::string EVENT_GROUP_META_K8S_NAMESPACE = "k8s.namespace.name";
const std::string EVENT_GROUP_META_K8S_POD_UID = "k8s.pod.uid";
const std::string EVENT_GROUP_META_K8S_POD_NAME = "k8s.pod.name";
const std::string EVENT_GROUP_META_CONTAINER_NAME = "container.name";
const std::string EVENT_GROUP_META_CONTAINER_IP = "container.ip";
const std::string EVENT_GROUP_META_CONTAINER_IMAGE_NAME = "container.image.name";
const std::string EVENT_GROUP_META_CONTAINER_IMAGE_ID = "container.image.id";

const std::string& EventGroupMetaKeyToString(EventGroupMetaKey key) {
    switch (key) {
        case EventGroupMetaKey::AGENT_TAG:
            return EVENT_GROUP_META_AGENT_TAG;
        case EventGroupMetaKey::HOST_IP:
            return EVENT_GROUP_META_HOST_IP;
        case EventGroupMetaKey::HOST_NAME:
            return EVENT_GROUP_META_HOST_NAME;
        case EventGroupMetaKey::LOG_TOPIC:
            return EVENT_GROUP_META_LOG_TOPIC;
        case EventGroupMetaKey::LOG_FILE_PATH:
            return EVENT_GROUP_META_LOG_FILE_PATH;
        case EventGroupMetaKey::LOG_FILE_PATH_RESOLVED:
            return EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED;
        case EventGroupMetaKey::LOG_FILE_INODE:
            return EVENT_GROUP_META_LOG_FILE_INODE;
        case EventGroupMetaKey::LOG_READ_OFFSET:
            return EVENT_GROUP_META_LOG_FILE_OFFSET;
        case EventGroupMetaKey::LOG_READ_LENGTH:
            return EVENT_GROUP_META_LOG_FILE_LENGTH;
        default:
            static std::string sEmpty = "unknown";
            return sEmpty;
    }
}

EventGroupMetaKey StringToEventGroupMetaKey(const std::string& key) {
    static std::unordered_map<std::string, EventGroupMetaKey> sStringToEnum
        = {{EVENT_GROUP_META_AGENT_TAG, EventGroupMetaKey::AGENT_TAG},
           {EVENT_GROUP_META_HOST_IP, EventGroupMetaKey::HOST_IP},
           {EVENT_GROUP_META_HOST_NAME, EventGroupMetaKey::HOST_NAME},
           {EVENT_GROUP_META_LOG_TOPIC, EventGroupMetaKey::LOG_TOPIC},
           {EVENT_GROUP_META_LOG_FILE_PATH, EventGroupMetaKey::LOG_FILE_PATH},
           {EVENT_GROUP_META_LOG_FILE_PATH_RESOLVED, EventGroupMetaKey::LOG_FILE_PATH_RESOLVED},
           {EVENT_GROUP_META_LOG_FILE_INODE, EventGroupMetaKey::LOG_FILE_INODE},
           {EVENT_GROUP_META_LOG_FILE_OFFSET, EventGroupMetaKey::LOG_READ_OFFSET},
           {EVENT_GROUP_META_LOG_FILE_LENGTH, EventGroupMetaKey::LOG_READ_LENGTH}};
    auto it = sStringToEnum.find(key);
    if (it != sStringToEnum.end()) {
        return it->second;
    }
    return EventGroupMetaKey::UNKNOWN;
}

Json::Value PipelineEventGroup::ToJson() const {
    Json::Value root;
    if (!mMetadata.empty()) {
        Json::Value metadata;
        for (const auto& meta : mMetadata) {
            metadata[EventGroupMetaKeyToString(meta.first)] = meta.second.to_string();
        }
        root["metadata"] = metadata;
    }
    if (!mTags.empty()) {
        Json::Value tags;
        for (const auto& tag : mTags) {
            tags[tag.first.to_string()] = tag.second.to_string();
        }
        root["tags"] = tags;
    }
    if (!this->GetEvents().empty()) {
        Json::Value events;
        for (const auto& event : this->GetEvents()) {
            events.append(event->ToJson());
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
            PipelineEventPtr eventPtr;
            if (event["type"].asInt() == LOG_EVENT_TYPE) {
                eventPtr = LogEvent::CreateEvent(GetSourceBuffer());
            } else if (event["type"].asInt() == METRIC_EVENT_TYPE) {
                eventPtr = MetricEvent::CreateEvent(GetSourceBuffer());
            } else {
                eventPtr = SpanEvent::CreateEvent(GetSourceBuffer());
            }
            eventPtr->FromJson(event);
            AddEvent(eventPtr);
        }
    }
    return true;
}

std::string PipelineEventGroup::ToJsonString() const {
    Json::Value root = ToJson();
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(root, &oss);
    return oss.str();
}

bool PipelineEventGroup::FromJsonString(const std::string& inJson) {
    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string errs;
    Json::Value root;
    if (!reader->parse(inJson.data(), inJson.data() + inJson.size(), &root, &errs)) {
        LOG_ERROR(sLogger, ("build PipelineEventGroup FromJsonString error", errs));
        return false;
    }
    return FromJson(root);
}
#endif

} // namespace logtail