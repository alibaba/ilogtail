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

#include "plugin/processor/ProcessorParseSimdOndemandJsonNative.h"


#include "common/ParamExtractor.h"
#include "models/LogEvent.h"
#include "monitor/metric_constants/MetricConstants.h"
#include "pipeline/plugin/instance/ProcessorInstance.h"

namespace logtail {

const std::string ProcessorParseSimdOndemandJsonNative::sName = "processor_parse_simd_ondemand_json_native";

bool ProcessorParseSimdOndemandJsonNative::Init(const Json::Value& config) {
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
    auto my_implementation = simdjson::get_available_implementations()["westmere"];
    if (! my_implementation) { exit(1); }
    if (! my_implementation->supported_by_runtime_system()) { exit(1); }
    simdjson::get_active_implementation() = my_implementation;

    std::cout <<  "active : " << simdjson::get_active_implementation()->name() << std::endl;
    return true;
}

void ProcessorParseSimdOndemandJsonNative::Process(PipelineEventGroup& logGroup) {
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

bool ProcessorParseSimdOndemandJsonNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
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

bool ProcessorParseSimdOndemandJsonNative::JsonLogLineParser(LogEvent& sourceEvent,
                                                 const StringView& logPath,
                                                 PipelineEventPtr& e,
                                                 bool& sourceKeyOverwritten) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    if (buffer.empty())
        return false;

    bool parseSuccess = true;

    simdjson::ondemand::parser parser;

    simdjson::padded_string bufStr(buffer.data(), buffer.size());
    auto doc = parser.iterate(bufStr);

    simdjson::ondemand::object object = doc.get_object();
   
    if (!parseSuccess) {
        return false;
    }

    for(auto field : object) {
        // parses and writes out the key, after unescaping it,
        // to a string buffer. It causes a performance penalty.
        std::string_view keyv = field.unescaped_key();
        StringBuffer contentKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(keyv.data(), keyv.size());

        simdjson::ondemand::value value = field.value();
        std::string value_str;
        {
            
            // 将 value 转成字符串
            switch (value.type()) {
                case simdjson::fallback::ondemand::json_type::null: {
                    value_str = "null";
                    break;
                }
                case simdjson::fallback::ondemand::json_type::number: {
                    simdjson::ondemand::number num = value.get_number();
                    simdjson::ondemand::number_type t = num.get_number_type();
                    switch(t) {
                        case simdjson::ondemand::number_type::signed_integer:
                            value_str = std::to_string(num.get_int64());

                            break;
                        case simdjson::ondemand::number_type::unsigned_integer:
                            value_str = std::to_string(num.get_uint64());
                            
                            break;
                        case simdjson::ondemand::number_type::floating_point_number:
                            value_str = std::to_string(num.get_double());
                            
                            break;
                        case simdjson::ondemand::number_type::big_integer:
                            std::string_view tmp = value.get_string();
                            value_str = {tmp.data(), tmp.size()};
                            break;
                    }
                    break;
                }
                case simdjson::fallback::ondemand::json_type::string: {
                    std::string_view tmp = value.get_string();
                    value_str = {tmp.data(), tmp.size()};
                    break;
                }
                case simdjson::fallback::ondemand::json_type::boolean: {
                    value_str = value.get_bool() ? "true" : "false";
                    break;
                }
                case simdjson::fallback::ondemand::json_type::object: {
                    value_str = "unknown type";
                    break;
                }
                case simdjson::fallback::ondemand::json_type::array: {
                    value_str = "unknown type";
                    break;
                }
                default: {
                    value_str = "unknown type";
                    break;
                }
            }
        }

        StringBuffer contentValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(value_str);

        //std::string_view valuev = field.value().raw_json();
        //StringBuffer contentValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(valuev.data(), valuev.size());   

        if (keyv == mSourceKey) {
            sourceKeyOverwritten = true;
        }

        AddLog(StringView(contentKeyBuffer.data, contentKeyBuffer.size),
               StringView(contentValueBuffer.data, contentValueBuffer.size),
               sourceEvent);
    } 
    return true;
}


std::string ProcessorParseSimdOndemandJsonNative::SimdjsonValueToString(const simdjson::ondemand::value& value) {
    std::string value_str;
    
    return value_str;
}

void ProcessorParseSimdOndemandJsonNative::AddLog(const StringView& key,
                                      const StringView& value,
                                      LogEvent& targetEvent,
                                      bool overwritten) {
    if (!overwritten && targetEvent.HasContent(key)) {
        return;
    }
    targetEvent.SetContentNoCopy(key, value);
    *mLogGroupSize += key.size() + value.size() + 5;
}

bool ProcessorParseSimdOndemandJsonNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
