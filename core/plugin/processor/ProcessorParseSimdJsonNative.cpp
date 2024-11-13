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

#include "plugin/processor/ProcessorParseSimdJsonNative.h"


#include "common/ParamExtractor.h"
#include "models/LogEvent.h"
#include "monitor/metric_constants/MetricConstants.h"
#include "pipeline/plugin/instance/ProcessorInstance.h"

namespace logtail {

const std::string ProcessorParseSimdJsonNative::sName = "processor_parse_simd_json_native";

bool ProcessorParseSimdJsonNative::Init(const Json::Value& config) {
    std::string errorMsg;

    // SourceKey
    if (!GetMandatoryStringParam(config, "SourceKey", mSourceKey, errorMsg)) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           errorMsg,
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    if (!mCommonParserOptions.Init(config, *mContext, sName)) {
        return false;
    }

    mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
    mLogGroupSize = &(GetContext().GetProcessProfile().logGroupSize);

    mDiscardedEventsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PLUGIN_DISCARDED_EVENTS_TOTAL);
    mOutFailedEventsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PLUGIN_OUT_FAILED_EVENTS_TOTAL);
    mOutKeyNotFoundEventsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PLUGIN_OUT_KEY_NOT_FOUND_EVENTS_TOTAL);
    mOutSuccessfulEventsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PLUGIN_OUT_SUCCESSFUL_EVENTS_TOTAL);
    for (auto implementation : simdjson::get_available_implementations()) {
        std::cout << implementation->name() << ": " << implementation->description() << std::endl;
    }
    auto my_implementation = simdjson::get_available_implementations()["haswell"];
    if (! my_implementation) { exit(1); }
    if (! my_implementation->supported_by_runtime_system()) { exit(1); }
    simdjson::get_active_implementation() = my_implementation;

    std::cout <<  "active : " << simdjson::get_active_implementation()->name() << std::endl;
    return true;
}

void ProcessorParseSimdJsonNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    EventsContainer& events = logGroup.MutableEvents();

    size_t wIdx = 0;
    for (size_t rIdx = 0; rIdx < events.size(); ++rIdx) {
        if (ProcessEvent(logPath, events[rIdx])) {
            if (wIdx != rIdx) {
                events[wIdx] = std::move(events[rIdx]);
            }
            ++wIdx;
        }
    }
    events.resize(wIdx);
}

bool ProcessorParseSimdJsonNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        mOutFailedEventsTotal->Add(1);
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        mOutKeyNotFoundEventsTotal->Add(1);
        return true;
    }

    auto rawContent = sourceEvent.GetContent(mSourceKey);

    bool sourceKeyOverwritten = false;
    bool parseSuccess = JsonLogLineParser(sourceEvent, logPath, e, sourceKeyOverwritten);

    if (!parseSuccess || !sourceKeyOverwritten) {
        sourceEvent.DelContent(mSourceKey);
    }
    if (mCommonParserOptions.ShouldAddSourceContent(parseSuccess)) {
        AddLog(mCommonParserOptions.mRenamedSourceKey, rawContent, sourceEvent, false);
    }
    if (mCommonParserOptions.ShouldAddLegacyUnmatchedRawLog(parseSuccess)) {
        AddLog(mCommonParserOptions.legacyUnmatchedRawLogKey, rawContent, sourceEvent, false);
    }
    if (mCommonParserOptions.ShouldEraseEvent(parseSuccess, sourceEvent)) {
        mDiscardedEventsTotal->Add(1);
        return false;
    }
    mOutSuccessfulEventsTotal->Add(1);
    return true;
}

bool ProcessorParseSimdJsonNative::JsonLogLineParser(LogEvent& sourceEvent,
                                                 const StringView& logPath,
                                                 PipelineEventPtr& e,
                                                 bool& sourceKeyOverwritten) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    if (buffer.empty())
        return false;

    bool parseSuccess = true;
    simdjson::dom::parser parser;
    simdjson::dom::element element = parser.parse(buffer.data(), buffer.size());

    if (!parseSuccess) {
        ++(*mParseFailures);
        mOutFailedEventsTotal->Add(1);
        parseSuccess = false;
    } else if (!element.is_object()) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(sLogger,
                        ("invalid json object, log", buffer)("project", GetContext().GetProjectName())(
                            "logstore", GetContext().GetLogstoreName())("file", logPath));
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("invalid json object:") + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        }
        ++(*mParseFailures);
        mOutFailedEventsTotal->Add(1);
        parseSuccess = false;
    }
    if (!parseSuccess) {
        return false;
    }
    
    for (auto field : element.get_object()) {
        //std::string contentKey = field.key;
        std::string contentValue = SimdjsonValueToString(field.value);
        StringBuffer contentKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(field.key.data(), field.key.size());
        StringBuffer contentValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(contentValue);

        if (field.key == mSourceKey) {
            sourceKeyOverwritten = true;
        }

        AddLog(StringView(contentKeyBuffer.data, contentKeyBuffer.size),
               StringView(contentValueBuffer.data, contentValueBuffer.size),
               sourceEvent);
    }
    
    return true;
}

std::string ProcessorParseSimdJsonNative::SimdjsonValueToString(const simdjson::dom::element& value) {
    std::string value_str;
    // 将 value 转成字符串
    switch (value.type()) {
        case simdjson::dom::element_type::NULL_VALUE:
            value_str = "null";
            break;
        case simdjson::dom::element_type::INT64:
            value_str = std::to_string(value.get_int64().value());
            break;
        case simdjson::dom::element_type::UINT64:
            value_str = std::to_string(value.get_uint64().value());
            break;
        case simdjson::dom::element_type::DOUBLE:
            value_str = std::to_string(value.get_double().value());
            break;
        case simdjson::dom::element_type::STRING:
            value_str = value.get_c_str().value();
            break;
        case simdjson::dom::element_type::BOOL:
            value_str = value.get_bool().value() ? "true" : "false";
            break;
        default:
            value_str = "unknown type";
            break;
    }
    return value_str;
}

void ProcessorParseSimdJsonNative::AddLog(const StringView& key,
                                      const StringView& value,
                                      LogEvent& targetEvent,
                                      bool overwritten) {
    if (!overwritten && targetEvent.HasContent(key)) {
        return;
    }
    targetEvent.SetContentNoCopy(key, value);
    *mLogGroupSize += key.size() + value.size() + 5;
}

bool ProcessorParseSimdJsonNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
