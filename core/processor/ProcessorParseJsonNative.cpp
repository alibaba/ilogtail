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

#include "processor/ProcessorParseJsonNative.h"
#include "common/Constants.h"
#include "models/LogEvent.h"
#include "plugin/instance/ProcessorInstance.h"
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "parser/LogParser.h"
#include "monitor/MetricConstants.h"

namespace logtail {
const std::string ProcessorParseJsonNative::sName = "processor_parse_json_native";

bool ProcessorParseJsonNative::Init(const ComponentConfig& componentConfig) {
    const PipelineConfig& config = componentConfig.GetConfig();

    mSourceKey = DEFAULT_CONTENT_KEY;
    mDiscardUnmatch = config.mDiscardUnmatch;
    mUploadRawLog = config.mUploadRawLog;
    mRawLogTag = config.mAdvancedConfig.mRawLogTag;

    if (mUploadRawLog && mRawLogTag == mSourceKey) {
        mSourceKeyOverwritten = true;
    }

    mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
    mLogGroupSize = &(GetContext().GetProcessProfile().logGroupSize);

    mProcParseInSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_IN_SIZE_BYTES);
    mProcParseOutSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_OUT_SIZE_BYTES);
    mProcDiscardRecordsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_DISCARD_RECORDS_TOTAL);
    mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);

    return true;
}

void ProcessorParseJsonNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(logPath, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorParseJsonNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }

    auto rawContent = sourceEvent.GetContent(mSourceKey);

    bool res = true;
    res = JsonLogLineParser(sourceEvent, logPath, e);

    if (!res && !mDiscardUnmatch) {
        AddLog(LogParser::UNMATCH_LOG_KEY, // __raw_log__
               rawContent,
               sourceEvent); // legacy behavior, should use sourceKey
    }
    if (res || !mDiscardUnmatch) {
        if (mUploadRawLog && (!res || !mRawLogTagOverwritten)) {
            AddLog(mRawLogTag, rawContent, sourceEvent); // __raw__
        }
        if (res && !mSourceKeyOverwritten) {
            sourceEvent.DelContent(mSourceKey);
        }
        return true;
    }
    mProcDiscardRecordsTotal->Add(1);
    return false;
}

bool ProcessorParseJsonNative::JsonLogLineParser(LogEvent& sourceEvent,
                                                 const StringView& logPath,
                                                 PipelineEventPtr& e) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    if (buffer.empty())
        return false;

    mProcParseInSizeBytes->Add(buffer.size());

    bool parseSuccess = true;
    rapidjson::Document doc;
    doc.Parse(buffer.data(), buffer.size());
    if (doc.HasParseError()) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(sLogger,
                        ("parse json log fail, log", buffer)("rapidjson offset", doc.GetErrorOffset())(
                            "rapidjson error", doc.GetParseError())("project", GetContext().GetProjectName())(
                            "logstore", GetContext().GetLogstoreName())("file", logPath));
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("parse json fail:") + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        }
        ++(*mParseFailures);
        mProcParseErrorTotal->Add(1);
        parseSuccess = false;
    } else if (!doc.IsObject()) {
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
        mProcParseErrorTotal->Add(1);
        parseSuccess = false;
    }
    if (!parseSuccess) {
        return false;
    }

    for (rapidjson::Value::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
        std::string contentKey = RapidjsonValueToString(itr->name);
        std::string contentValue = RapidjsonValueToString(itr->value);

        StringBuffer contentKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(contentKey);
        StringBuffer contentValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(contentValue);

        if (contentKey.c_str() == mSourceKey) {
            mSourceKeyOverwritten = true;
        }
        if (contentKey.c_str() == mRawLogTag) {
            mRawLogTagOverwritten = true;
        }

        AddLog(StringView(contentKeyBuffer.data, contentKeyBuffer.size),
               StringView(contentValueBuffer.data, contentValueBuffer.size),
               sourceEvent);
    }
    return true;
}

std::string ProcessorParseJsonNative::RapidjsonValueToString(const rapidjson::Value& value) {
    if (value.IsString())
        return value.GetString();
    else if (value.IsBool())
        return ToString(value.GetBool());
    else if (value.IsInt())
        return ToString(value.GetInt());
    else if (value.IsUint())
        return ToString(value.GetUint());
    else if (value.IsInt64())
        return ToString(value.GetInt64());
    else if (value.IsUint64())
        return ToString(value.GetUint64());
    else if (value.IsDouble())
        return ToString(value.GetDouble());
    else if (value.IsNull())
        return "";
    else // if (value.IsObject() || value.IsArray())
    {
        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        value.Accept(writer);
        return buffer.GetString();
    }
}

void ProcessorParseJsonNative::AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent) {
    targetEvent.SetContentNoCopy(key, value);
    size_t keyValueSize = key.size() + value.size();
    *mLogGroupSize += keyValueSize + 5;
    mProcParseOutSizeBytes->Add(keyValueSize);
}

bool ProcessorParseJsonNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail