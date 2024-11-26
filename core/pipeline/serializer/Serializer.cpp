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

#include "Serializer.h"

namespace logtail {

std::string JsonToString(Json::Value& value) {
    Json::StreamWriterBuilder writer;
    return Json::writeString(writer, value);
}

std::string SerializeSpanTagsToString(const SpanEvent& event) {
    Json::Value jsonVal;
    for (auto it = event.TagsBegin(); it != event.TagsEnd(); ++it) {
        jsonVal[it->first.to_string()] = it->second.to_string();
    }
    for (auto it = event.ScopeTagsBegin(); it != event.ScopeTagsEnd(); ++it) {
        jsonVal[it->first.to_string()] = it->second.to_string();
    }
    return JsonToString(jsonVal);
}

std::string SerializeSpanLinksToString(const SpanEvent& event) {
    if (event.GetLinks().empty()) {
        return "";
    }
    Json::Value jsonLinks(Json::arrayValue);
    for (const auto& link : event.GetLinks()) {
        jsonLinks.append(link.ToJson());
    }
    return JsonToString(jsonLinks);
}

std::string SerializeSpanEventsToString(const SpanEvent& event) {
    if (event.GetEvents().empty()) {
        return "";
    }
    Json::Value jsonEvents(Json::arrayValue);
    for (const auto& event : event.GetEvents()) {
        jsonEvents.append(event.ToJson());
    }
    return JsonToString(jsonEvents);
}

} // namespace logtail