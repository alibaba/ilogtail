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

#include "processor/ProcessorParseContainerLogNative.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "common/ParamExtractor.h"
#include "models/LogEvent.h"
#include "monitor/MetricConstants.h"
#include "plugin/instance/ProcessorInstance.h"

namespace logtail {

const std::string ProcessorParseContainerLogNative::sName = "processor_parse_container_log_native";

bool ProcessorParseContainerLogNative::Init(const Json::Value& config) {
    std::string errorMsg;

    // SourceKey
    if (!GetOptionalStringParam(config, "SourceKey", mSourceKey, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mSourceKey,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // IgnoringStdout
    if (!GetOptionalBoolParam(config, "IgnoringStdout", mIgnoringStdout, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStdout,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // IgnoringStderr
    if (!GetOptionalBoolParam(config, "IgnoringStderr", mIgnoringStderr, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStderr,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    return true;
}

void ProcessorParseContainerLogNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    const StringView& containerType = logGroup.GetMetadata(EventGroupMetaKey::CONTAINER_TYPE);
    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(containerType, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorParseContainerLogNative::ProcessEvent(const StringView& containerType, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    if (containerType == "containerd") {
        return ContainerdLogLineParser(sourceEvent, e);
    } else if (containerType == "docker-json") {
        return DockerJsonLogLineParser(sourceEvent, e);
    }
    return true;
}

bool ProcessorParseContainerLogNative::ContainerdLogLineParser(LogEvent& sourceEvent, PipelineEventPtr& e) {
    StringView contentValue = sourceEvent.GetContent(mSourceKey);

    if (contentValue.empty() || contentValue.length() < 5)
        return true;

    // 寻找第一个分隔符位置 时间 _time_
    StringView timeValue;
    const char* pch1
        = std::search(contentValue.begin(), contentValue.end(), contianerdDelimiter.begin(), contianerdDelimiter.end());
    if (pch1 >= contentValue.end() - 1) {
        // 没有找到分隔符
        return true;
    }
    timeValue = StringView(contentValue.data(), pch1 - contentValue.data());

    // 寻找第二个分隔符位置 容器标签 _source_
    StringView sourceValue;
    const char* pch2
        = std::search(pch1 + 1, contentValue.end(), contianerdDelimiter.begin(), contianerdDelimiter.end());
    if (pch2 == contentValue.end()) {
        // 没有找到分隔符
        return true;
    }
    sourceValue = StringView(pch1 + 1, pch2 - pch1 - 1);

    if (sourceValue != "stdout" && sourceValue != "stderr")
        return true;
    if (sourceValue == "stdout" && mIgnoringStdout)
        return false;
    if (sourceValue == "stderr" && mIgnoringStderr)
        return false;

    // 如果既不以 contianerdPartTag 开头，也不以 contianerdFullTag 开头
    if (*(pch2 + 1) != contianerdPartTag && *(pch2 + 1) != contianerdFullTag) {
        StringBuffer containerTimeKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerTimeKey);
        StringBuffer containerSourceKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerSourceKey);
        StringBuffer containerLogKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerLogKey);
        // content
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        AddLog(StringView(containerTimeKeyBuffer.data, containerTimeKeyBuffer.size), timeValue, sourceEvent);
        AddLog(StringView(containerSourceKeyBuffer.data, containerSourceKeyBuffer.size), sourceValue, sourceEvent);
        AddLog(StringView(containerLogKeyBuffer.data, containerLogKeyBuffer.size), content, sourceEvent);
        return true;
    }

    // 寻找第三个分隔符位置
    const char* pch3
        = std::search(pch2 + 1, contentValue.end(), contianerdDelimiter.begin(), contianerdDelimiter.end());
    if (pch3 == contentValue.end() || pch3 != pch2 + 2) {
        return true;
    }
    // F
    if (*(pch2 + 1) == contianerdFullTag) {
        StringBuffer containerTimeKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerTimeKey);
        StringBuffer containerSourceKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerSourceKey);
        StringBuffer containerLogKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerLogKey);
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        AddLog(StringView(containerTimeKeyBuffer.data, containerTimeKeyBuffer.size), timeValue, sourceEvent);
        AddLog(StringView(containerSourceKeyBuffer.data, containerSourceKeyBuffer.size), sourceValue, sourceEvent);
        AddLog(StringView(containerLogKeyBuffer.data, containerLogKeyBuffer.size), content, sourceEvent);
        return true;
    }
    // P
    if (*(pch2 + 1) == contianerdPartTag) {
        StringBuffer containerTimeKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerTimeKey);
        StringBuffer containerSourceKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerSourceKey);
        StringBuffer containerLogKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerLogKey);
        StringBuffer partTagBuffer = sourceEvent.GetSourceBuffer()->CopyString(PARTLOGFLAG);
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        StringView partTag = StringView(pch2 + 1, 1);
        AddLog(StringView(containerTimeKeyBuffer.data, containerTimeKeyBuffer.size), timeValue, sourceEvent);
        AddLog(StringView(containerSourceKeyBuffer.data, containerSourceKeyBuffer.size), sourceValue, sourceEvent);
        AddLog(StringView(partTagBuffer.data, partTagBuffer.size), partTag, sourceEvent);
        AddLog(StringView(containerLogKeyBuffer.data, containerLogKeyBuffer.size), content, sourceEvent);
        return true;
    }
}

bool ProcessorParseContainerLogNative::DockerJsonLogLineParser(LogEvent& sourceEvent, PipelineEventPtr& e) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    if (buffer.empty())
        return true;

    bool parseSuccess = true;
    rapidjson::Document doc;
    doc.Parse(buffer.data(), buffer.size());
    if (doc.HasParseError()) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(sLogger,
                        ("parse docker stdout json log fail, log",
                         buffer)("rapidjson offset", doc.GetErrorOffset())("rapidjson error", doc.GetParseError())(
                            "project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName()));
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("parse json fail:") + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        }
        parseSuccess = false;
    } else if (!doc.IsObject()) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(sLogger,
                        ("invalid docker stdout json object, log",
                         buffer)("project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName()));
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("invalid json object:") + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        }
        parseSuccess = false;
    }
    if (!parseSuccess) {
        return true;
    }
    if (!doc.HasMember(dockerJsonLogContent.c_str()) || !doc.HasMember(dockerJsonTime.c_str())
        || !doc.HasMember(dockerJsonStreamType.c_str())) {
        if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(sLogger,
                        ("invalid docker stdout json log, log",
                         buffer)("project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName()));
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("invalid docker stdout json log:") + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
        }
        return true;
    }
    StringView sourceValue(doc[dockerJsonStreamType.c_str()].GetString());
    if (sourceValue == "stdout" && mIgnoringStdout)
        return false;
    if (sourceValue == "stderr" && mIgnoringStderr)
        return false;

    StringView content(doc[dockerJsonLogContent.c_str()].GetString());
    StringView timeValue(doc[dockerJsonTime.c_str()].GetString());

    StringBuffer containerTimeKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerTimeKey);
    StringBuffer containerSourceKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerSourceKey);
    StringBuffer containerLogKeyBuffer = sourceEvent.GetSourceBuffer()->CopyString(containerLogKey);

    StringBuffer containerTimeValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(timeValue);
    StringBuffer containerSourceValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(sourceValue);
    StringBuffer containerLogValueBuffer = sourceEvent.GetSourceBuffer()->CopyString(content);

    AddLog(StringView(containerTimeKeyBuffer.data, containerTimeKeyBuffer.size), StringView(containerTimeValueBuffer.data,containerTimeValueBuffer.size), sourceEvent);
    AddLog(StringView(containerSourceKeyBuffer.data, containerSourceKeyBuffer.size), StringView(containerSourceValueBuffer.data,containerSourceValueBuffer.size), sourceEvent);
    AddLog(StringView(containerLogKeyBuffer.data, containerLogKeyBuffer.size), StringView(containerLogValueBuffer.data,containerLogValueBuffer.size), sourceEvent);
    return true;
}

void ProcessorParseContainerLogNative::AddLog(const StringView& key,
                                              const StringView& value,
                                              LogEvent& targetEvent,
                                              bool overwritten) {
    targetEvent.SetContentNoCopy(key, value);
}

bool ProcessorParseContainerLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
