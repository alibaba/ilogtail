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

const std::string ProcessorParseContainerLogNative::containerTimeKey = "_time_"; // 容器时间字段
const std::string ProcessorParseContainerLogNative::containerSourceKey = "_source_"; // 容器来源字段
const std::string ProcessorParseContainerLogNative::containerLogKey = "content"; // 容器日志字段

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

    StringView containerType = logGroup.GetMetadata(EventGroupMetaKey::FILE_ENCODING);
    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(containerType, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorParseContainerLogNative::ProcessEvent(StringView containerType, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    std::string errorMsg;
    bool parseSuccess = true;
    if (containerType == "containerd_text") {
        parseSuccess = ParseContainerdLogLine(sourceEvent, errorMsg);
    } else if (containerType == "docker_json-file") {
        parseSuccess = ParseDockerJsonLogLine(sourceEvent, errorMsg);
    }
    if (errorMsg.size() > 0 && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
        errorMsg = "containerType: " + containerType.to_string() + ", " + errorMsg;
        LOG_WARNING(sLogger, ("parse Container log error", errorMsg));
        LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                               errorMsg,
                                               GetContext().GetProjectName(),
                                               GetContext().GetLogstoreName(),
                                               GetContext().GetRegion());
    }
    return parseSuccess;
}

bool ProcessorParseContainerLogNative::ParseContainerdLogLine(LogEvent& sourceEvent, std::string& errorMsg) {
    StringView contentValue = sourceEvent.GetContent(mSourceKey);

    // 寻找第一个分隔符位置 时间 _time_
    StringView timeValue;
    const char* pch1 = std::find(contentValue.begin(), contentValue.end(), CONTIANERD_DELIMITER);
    if (pch1 == contentValue.end()) {
        errorMsg = "no delimiter found in containerd log line";
        return true;
    }
    timeValue = StringView(contentValue.data(), pch1 - contentValue.data());

    // 寻找第二个分隔符位置 容器标签 _source_
    StringView sourceValue;
    const char* pch2 = std::find(pch1 + 1, contentValue.end(), CONTIANERD_DELIMITER);
    if (pch2 == contentValue.end()) {
        errorMsg = "no delimiter found in containerd log line";
        return true;
    }
    sourceValue = StringView(pch1 + 1, pch2 - pch1 - 1);

    if (sourceValue != "stdout" && sourceValue != "stderr") {
        errorMsg = "no stdout or stderr found in containerd log line";
        return true;
    }
    if (sourceValue == "stdout" && mIgnoringStdout)
        return false;
    if (sourceValue == "stderr" && mIgnoringStderr)
        return false;

    // 如果既不以 CONTIANERD_PART_TAG 开头，也不以 CONTIANERD_FULL_TAG 开头
    if (*(pch2 + 1) != CONTIANERD_PART_TAG && *(pch2 + 1) != CONTIANERD_FULL_TAG) {
        // content
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        AddContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    }

    // 寻找第三个分隔符位置
    const char* pch3 = std::find(pch2 + 1, contentValue.end(), CONTIANERD_DELIMITER);
    if (pch3 == contentValue.end() || pch3 != pch2 + 2) {
        // case: 2021-08-25T07:00:00.000000000Z stdout P
        // case: 2021-08-25T07:00:00.000000000Z stdout PP 1
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        AddContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    }
    // F
    if (*(pch2 + 1) == CONTIANERD_FULL_TAG) {
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        AddContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    }
    // P
    if (*(pch2 + 1) == CONTIANERD_PART_TAG) {
        // content
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        AddContainerdTextLog(timeValue, sourceValue, content, true, sourceEvent);
        return true;
    }
    return true;
}

bool ProcessorParseContainerLogNative::ParseDockerJsonLogLine(LogEvent& sourceEvent, std::string& errorMsg) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    bool parseSuccess = true;
    rapidjson::Document doc;
    doc.Parse(buffer.data(), buffer.size());
    if (doc.HasParseError()) {
        std::string errorMsg = "parse docker stdout json log fail, log: " + buffer.to_string() + ", rapidjson offset: "
            + std::to_string(doc.GetErrorOffset()) + ", rapidjson error: " + std::to_string(doc.GetParseError())
            + ", project: " + GetContext().GetProjectName() + ", logstore: " + GetContext().GetLogstoreName();
        parseSuccess = false;
    } else if (!doc.IsObject()) {
        std::string errorMsg = "invalid docker stdout json log: " + buffer.to_string();
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
        std::string errorMsg = "invalid docker stdout json log: " + buffer.to_string();
        return true;
    }
    if ((sourceValue == "stdout" && mIgnoringStdout) || (sourceValue == "stderr" && mIgnoringStderr)) {
        return false;
    }
    StringView timeValue = findMemberAndGetString(DOCKER_JSON_TIME);
    if (sourceValue.empty()) {
        std::string errorMsg = "invalid docker stdout json log: " + buffer.to_string();
        return true;
    }
    StringView content = findMemberAndGetString(DOCKER_JSON_LOG);
    if (content.empty()) {
        std::string errorMsg = "invalid docker stdout json log: " + buffer.to_string();
        return true;
    }

    char* data;

    if (buffer.size() < content.size() + timeValue.size() + sourceValue.size()) {
        errorMsg = "The size of the Docker JSON log file has exceeded the limit. Log: " + buffer.to_string();
        return true;
    }
    data = const_cast<char*>(buffer.data());

    // time
    AddDockerJsonLog(data, containerTimeKey, timeValue, sourceEvent);
    data += timeValue.size();
    // source
    AddDockerJsonLog(data, containerSourceKey, sourceValue, sourceEvent);
    data += sourceValue.size();
    // content
    if (content.size() > 0 && content[content.size() - 1] == '\n') {
        content = StringView(content.data(), content.size() - 1);
    }
    AddDockerJsonLog(data, containerLogKey, content, sourceEvent);

    return true;
}

void ProcessorParseContainerLogNative::AddDockerJsonLog(char* data,
                                                        StringView key,
                                                        StringView value,
                                                        LogEvent& targetEvent) {
    memmove(data, value.data(), value.size());
    StringView valueBuffer = StringView(data, value.size());
    targetEvent.SetContentNoCopy(key, valueBuffer);
}

void ProcessorParseContainerLogNative::AddContainerdTextLog(
    StringView time, StringView source, StringView content, bool isPartialLog, LogEvent& sourceEvent) {
    sourceEvent.SetContentNoCopy(containerTimeKey, time);
    sourceEvent.SetContentNoCopy(containerSourceKey, source);
    if (isPartialLog) {
        sourceEvent.SetContent(ProcessorMergeMultilineLogNative::PartLogFlag,
                               ProcessorMergeMultilineLogNative::PartLogFlag);
    }
    sourceEvent.SetContentNoCopy(containerLogKey, content);
}

bool ProcessorParseContainerLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
