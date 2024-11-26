// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "pipeline/serializer/JsonSerializer.h"

#include "constants/SpanConstants.h"
#include "protobuf/sls/LogGroupSerializer.h"

using namespace std;

namespace logtail {

const string JSON_KEY_TIME = "__time__";
const string JSON_KEY_TIME_NANO = "__time_nano__";
const string JSON_KEY_TAGS = "tags";
const string JSON_KEY_CONTENTS = "contents";

bool JsonEventGroupSerializer::Serialize(BatchedEvents&& group, string& res, string& errorMsg) {
    if (group.mEvents.empty()) {
        errorMsg = "empty event group";
        return false;
    }

    PipelineEvent::Type eventType = group.mEvents[0]->GetType();
    if (eventType == PipelineEvent::Type::NONE) {
        // should not happen
        errorMsg = "unsupported event type in event group";
        return false;
    }

    Json::Value groupTags;
    for (const auto& tag : group.mTags.mInner) {
        groupTags[tag.first.to_string()] = tag.second.to_string();
    }

    std::ostringstream oss;
    switch (eventType) {
        case PipelineEvent::Type::LOG:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<LogEvent>();
                Json::Value eventJson;
                // time
                eventJson[JSON_KEY_TIME] = e.GetTimestamp();
                // tags
                eventJson[JSON_KEY_TAGS] = Json::Value();
                eventJson[JSON_KEY_TAGS].copy(groupTags);
                // contents
                eventJson[JSON_KEY_CONTENTS] = Json::Value();
                for (const auto& kv : e) {
                    eventJson[JSON_KEY_CONTENTS][kv.first.to_string()] = kv.second.to_string();
                }
                oss << JsonToString(eventJson) << "\n";
            }
            break;
        case PipelineEvent::Type::METRIC:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<MetricEvent>();
                if (e.Is<std::monostate>()) {
                    continue;
                }
                Json::Value eventJson;
                // time
                eventJson[JSON_KEY_TIME] = e.GetTimestamp();
                // tags
                eventJson[JSON_KEY_TAGS] = Json::Value();
                eventJson[JSON_KEY_TAGS].copy(groupTags);
                // contents
                eventJson[JSON_KEY_CONTENTS] = Json::Value();
                // __labels__
                eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_LABELS] = Json::Value();
                for (auto tag = e.TagsBegin(); tag != e.TagsEnd(); tag++) {
                    eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_LABELS][tag->first.to_string()]
                        = tag->second.to_string();
                }
                // __name__
                eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_NAME] = e.GetName().to_string();
                // __value__
                if (e.Is<UntypedSingleValue>()) {
                    eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_VALUE] = e.GetValue<UntypedSingleValue>()->mValue;
                } else if (e.Is<UntypedMultiDoubleValues>()) {
                    eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_VALUE] = Json::Value();
                    for (auto value = e.GetValue<UntypedMultiDoubleValues>()->ValusBegin();
                         value != e.GetValue<UntypedMultiDoubleValues>()->ValusEnd();
                         value++) {
                        eventJson[JSON_KEY_CONTENTS][METRIC_RESERVED_KEY_VALUE][value->first.to_string()]
                            = value->second;
                    }
                }
                oss << JsonToString(eventJson) << "\n";
            }
            break;
        case PipelineEvent::Type::SPAN:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<SpanEvent>();
                Json::Value eventJson;
                // time
                eventJson[JSON_KEY_TIME] = e.GetTimestamp();
                // tags
                eventJson[JSON_KEY_TAGS] = Json::Value();
                eventJson[JSON_KEY_TAGS].copy(groupTags);
                // contents
                eventJson[JSON_KEY_CONTENTS] = Json::Value();
                // set trace_id span_id span_kind status etc
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_TRACE_ID] = e.GetTraceId().to_string();
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_SPAN_ID] = e.GetSpanId().to_string();
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_PARENT_ID] = e.GetParentSpanId().to_string();
                // span_name
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_SPAN_NAME] = e.GetName().to_string();
                // span_kind
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_SPAN_KIND] = GetKindString(e.GetKind());
                // status_code
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_STATUS_CODE] = GetStatusString(e.GetStatus());
                // trace state
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_TRACE_STATE] = e.GetTraceState().to_string();
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_ATTRIBUTES] = SerializeSpanTagsToString(e);
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_LINKS] = SerializeSpanLinksToString(e);
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_EVENTS] = SerializeSpanEventsToString(e);
                // start_time
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_START_TIME_NANO] = std::to_string(e.GetStartTimeNs());
                // end_time
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_END_TIME_NANO] = std::to_string(e.GetEndTimeNs());
                // duration
                eventJson[JSON_KEY_CONTENTS][DEFAULT_TRACE_TAG_DURATION]
                    = std::to_string(e.GetEndTimeNs() - e.GetStartTimeNs());

                oss << JsonToString(eventJson) << "\n";
            }
            break;
        case PipelineEvent::Type::RAW:
            for (size_t i = 0; i < group.mEvents.size(); ++i) {
                const auto& e = group.mEvents[i].Cast<RawEvent>();
                Json::Value eventJson;
                // time
                eventJson[JSON_KEY_TIME] = e.GetTimestamp();
                // tags
                eventJson[JSON_KEY_TAGS] = Json::Value();
                eventJson[JSON_KEY_TAGS].copy(groupTags);
                // contents
                eventJson[JSON_KEY_CONTENTS] = Json::Value();
                eventJson[JSON_KEY_CONTENTS][DEFAULT_CONTENT_KEY] = e.GetContent().to_string();
                oss << JsonToString(eventJson) << "\n";
            }
            break;
        default:
            break;
    }
    res = oss.str();
    return true;
}

} // namespace logtail