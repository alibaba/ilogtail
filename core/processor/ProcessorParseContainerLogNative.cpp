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
#include "processor/ProcessorMergeMultilineLogNative.h"

namespace logtail {

const std::string ProcessorParseContainerLogNative::sName = "processor_parse_container_log_native";
const char ProcessorParseContainerLogNative::CONTIANERD_DELIMITER = ' '; // 分隔符
const char ProcessorParseContainerLogNative::CONTIANERD_FULL_TAG = 'F'; // 容器全标签
const char ProcessorParseContainerLogNative::CONTIANERD_PART_TAG = 'P'; // 容器部分标签

const std::string ProcessorParseContainerLogNative::DOCKER_JSON_LOG = "log"; // docker json 日志字段
const std::string ProcessorParseContainerLogNative::DOCKER_JSON_TIME = "time"; // docker json 时间字段
const std::string ProcessorParseContainerLogNative::DOCKER_JSON_STREAM_TYPE = "stream"; // docker json 流字段

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

    const StringView containerType = logGroup.GetMetadata(EventGroupMetaKey::FILE_ENCODING);
    EventsContainer& events = logGroup.MutableEvents();

    ContainerStdoutKeyStringBuffer keyBuffer = {
        timeKeyBuffer : logGroup.GetSourceBuffer()->CopyString(containerTimeKey),
        sourceKeyBuffer : logGroup.GetSourceBuffer()->CopyString(containerSourceKey),
        logKeyBuffer : logGroup.GetSourceBuffer()->CopyString(containerLogKey),
        flagBuffer : logGroup.GetSourceBuffer()->CopyString(ProcessorMergeMultilineLogNative::PartLogFlag)
    };

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(logGroup, containerType, *it, &keyBuffer)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorParseContainerLogNative::ProcessEvent(PipelineEventGroup& logGroup,
                                                    const StringView containerType,
                                                    PipelineEventPtr& e,
                                                    ContainerStdoutKeyStringBuffer* keyBuffer) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    if (containerType == "containerd_text") {
        return ContainerdLogLineParser(sourceEvent, keyBuffer);
    } else if (containerType == "docker_json-file") {
        return DockerJsonLogLineParser(logGroup, sourceEvent, keyBuffer);
    }
    return true;
}

bool ProcessorParseContainerLogNative::ContainerdLogLineParser(LogEvent& sourceEvent,
                                                               ContainerStdoutKeyStringBuffer* keyBuffer) {
    StringView contentValue = sourceEvent.GetContent(mSourceKey);

    if (contentValue.empty())
        return true;

    // 寻找第一个分隔符位置 时间 _time_
    StringView timeValue;
    const char* pch1 = std::find(contentValue.begin(), contentValue.end(), CONTIANERD_DELIMITER);
    if (pch1 >= contentValue.end()) {
        // 没有找到分隔符
        return true;
    }
    timeValue = StringView(contentValue.data(), pch1 - contentValue.data());

    // 寻找第二个分隔符位置 容器标签 _source_
    StringView sourceValue;
    const char* pch2 = std::find(pch1 + 1, contentValue.end(), CONTIANERD_DELIMITER);
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

    // 如果既不以 CONTIANERD_PART_TAG 开头，也不以 CONTIANERD_FULL_TAG 开头
    if (*(pch2 + 1) != CONTIANERD_PART_TAG && *(pch2 + 1) != CONTIANERD_FULL_TAG) {
        // content
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        AddContainerdTextLog(keyBuffer, timeValue, sourceValue, content, StringView(), sourceEvent);
        return true;
    }

    // 寻找第三个分隔符位置
    const char* pch3 = std::find(pch2 + 1, contentValue.end(), CONTIANERD_DELIMITER);
    if (pch3 == contentValue.end() || pch3 != pch2 + 2) {
        return true;
    }
    // F
    if (*(pch2 + 1) == CONTIANERD_FULL_TAG) {
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        AddContainerdTextLog(keyBuffer, timeValue, sourceValue, content, StringView(), sourceEvent);
        return true;
    }
    // P
    if (*(pch2 + 1) == CONTIANERD_PART_TAG) {
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        StringView partTag = StringView(pch2 + 1, 1);
        AddContainerdTextLog(keyBuffer, timeValue, sourceValue, content, partTag, sourceEvent);
        return true;
    }
}

bool ProcessorParseContainerLogNative::DockerJsonLogLineParser(PipelineEventGroup& logGroup,
                                                               LogEvent& sourceEvent,
                                                               ContainerStdoutKeyStringBuffer* keyBuffer) {
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

    auto findMemberAndGetString = [&](const std::string& memberName) {
        auto it = doc.FindMember(memberName.c_str());
        return it != doc.MemberEnd() ? StringView(it->value.GetString()) : StringView();
    };

    StringView sourceValue = findMemberAndGetString(DOCKER_JSON_STREAM_TYPE);
    if (sourceValue.empty() || (sourceValue != "stdout" && sourceValue != "stderr")) {
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
    if ((sourceValue == "stdout" && mIgnoringStdout) || (sourceValue == "stderr" && mIgnoringStderr)) {
        return false;
    }
    StringView timeValue = findMemberAndGetString(DOCKER_JSON_TIME);
    if (sourceValue.empty()) {
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
    StringView content = findMemberAndGetString(DOCKER_JSON_LOG);
    if (content.empty()) {
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

    char* data;

    if (buffer.size() < content.size() + timeValue.size() + sourceValue.size()) {
        StringBuffer stringBuffer = logGroup.GetSourceBuffer()->AllocateStringBuffer(content.size() + timeValue.size()
                                                                                     + sourceValue.size() + 1);
        data = const_cast<char*>(stringBuffer.data);
    } else {
        data = const_cast<char*>(buffer.data());
    }

    // time
    AddDockerJsonLog(&data, keyBuffer->timeKeyBuffer, timeValue, sourceEvent);
    // source
    AddDockerJsonLog(&data, keyBuffer->sourceKeyBuffer, sourceValue, sourceEvent);
    // content
    if (content.size() > 0 && content[content.size() - 1] == '\n') {
        content = StringView(content.data(), content.size() - 1);
    }
    AddDockerJsonLog(&data, keyBuffer->logKeyBuffer, content, sourceEvent);

    return true;
}

void ProcessorParseContainerLogNative::AddDockerJsonLog(char** data,
                                                        const StringBuffer keyBuffer,
                                                        const StringView value,
                                                        LogEvent& targetEvent) {
    memmove(*data, value.data(), value.size());
    StringView valueBuffer = StringView(*data, value.size());
    *data += value.size();
    targetEvent.SetContentNoCopy(StringView(keyBuffer.data, keyBuffer.size), valueBuffer);
}

void ProcessorParseContainerLogNative::AddContainerdTextLog(ContainerStdoutKeyStringBuffer* keyBuffer,
                                                            StringView timeValue,
                                                            StringView sourceValue,
                                                            StringView content,
                                                            StringView partTag,
                                                            LogEvent& sourceEvent) {
    sourceEvent.SetContentNoCopy(StringView(keyBuffer->timeKeyBuffer.data, keyBuffer->timeKeyBuffer.size), timeValue);
    sourceEvent.SetContentNoCopy(StringView(keyBuffer->sourceKeyBuffer.data, keyBuffer->sourceKeyBuffer.size),
                                 sourceValue);
    if (!partTag.empty()) {
        sourceEvent.SetContentNoCopy(StringView(keyBuffer->flagBuffer.data, keyBuffer->flagBuffer.size), partTag);
    }
    sourceEvent.SetContentNoCopy(StringView(keyBuffer->logKeyBuffer.data, keyBuffer->logKeyBuffer.size), content);
}

bool ProcessorParseContainerLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
