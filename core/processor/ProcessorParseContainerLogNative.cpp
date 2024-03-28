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

#include "common/JsonUtil.h"
#include "common/ParamExtractor.h"
#include "models/LogEvent.h"
#include "monitor/MetricConstants.h"
#include "processor/ProcessorMergeMultilineLogNative.h"

namespace logtail {

const std::string ProcessorParseContainerLogNative::CONTAINERD_TEXT = "1";
const std::string ProcessorParseContainerLogNative::DOCKER_JSON_FILE = "2";

const std::string ProcessorParseContainerLogNative::sName = "processor_parse_container_log_native";
const char ProcessorParseContainerLogNative::CONTAINERD_DELIMITER = ' '; // 分隔符
const char ProcessorParseContainerLogNative::CONTAINERD_FULL_TAG = 'F'; // 容器全标签
const char ProcessorParseContainerLogNative::CONTAINERD_PART_TAG = 'P'; // 容器部分标签

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

    // IgnoreParseWarning
    if (!GetOptionalBoolParam(config, "IgnoreParseWarning", mIgnoreParseWarning, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoreParseWarning,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // KeepingSourceWhenParseFail
    if (!GetOptionalBoolParam(config, "KeepingSourceWhenParseFail", mKeepingSourceWhenParseFail, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mKeepingSourceWhenParseFail,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    mProcParseInSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_IN_SIZE_BYTES);
    mProcParseOutSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_OUT_SIZE_BYTES);
    mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);
    mProcParseStdoutTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_STDOUT_TOTAL);
    mProcParseStderrTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_STDERR_TOTAL);

    return true;
}

void ProcessorParseContainerLogNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }

    StringView containerType = logGroup.GetMetadata(EventGroupMetaKey::LOG_FORMAT);

    EventsContainer& events = logGroup.MutableEvents();

    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(containerType, *it, logGroup)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
}

bool ProcessorParseContainerLogNative::ProcessEvent(StringView containerType,
                                                    PipelineEventPtr& e,
                                                    PipelineEventGroup& logGroup) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    auto& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    mProcParseInSizeBytes->Add(mSourceKey.size() + sourceEvent.GetContent(mSourceKey).size());

    std::string errorMsg;
    bool shouldKeepEvent = true;
    if (containerType == CONTAINERD_TEXT) {
        shouldKeepEvent = ParseContainerdTextLogLine(sourceEvent, errorMsg, logGroup);
    } else if (containerType == DOCKER_JSON_FILE) {
        shouldKeepEvent = ParseDockerJsonLogLine(sourceEvent, errorMsg);
    }
    if (!errorMsg.empty()) {
        mProcParseErrorTotal->Add(1);
    }

    if (!mIgnoreParseWarning && !errorMsg.empty() && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
        LOG_WARNING(sLogger,
                    ("failed to parse log line, errorMsg", errorMsg)("container runtime", containerType)(
                        "processor", sName)("config", mContext->GetConfigName()));
        errorMsg = "failed to parse log line, error: " + errorMsg + "\tcontainer runtime: " + containerType.to_string()
            + "\tprocessor: " + sName + "\tconfig: " + mContext->GetConfigName();
        LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                               errorMsg,
                                               GetContext().GetProjectName(),
                                               GetContext().GetLogstoreName(),
                                               GetContext().GetRegion());
    }
    return shouldKeepEvent;
}

bool ProcessorParseContainerLogNative::ParseContainerdTextLogLine(LogEvent& sourceEvent,
                                                                  std::string& errorMsg,
                                                                  PipelineEventGroup& logGroup) {
    StringView contentValue = sourceEvent.GetContent(mSourceKey);

    // 寻找第一个分隔符位置 时间 _time_
    StringView timeValue;
    const char* pch1 = std::find(contentValue.begin(), contentValue.end(), CONTAINERD_DELIMITER);
    if (pch1 == contentValue.end()) {
        std::ostringstream errorMsgStream;
        errorMsgStream << "time field cannot be found in log line."
                       << "\tfirst 1KB log:" << contentValue.substr(0, 1024).to_string();
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }
    timeValue = StringView(contentValue.data(), pch1 - contentValue.data());

    // 寻找第二个分隔符位置 容器标签 _source_
    StringView sourceValue;
    const char* pch2 = std::find(pch1 + 1, contentValue.end(), CONTAINERD_DELIMITER);
    if (pch2 == contentValue.end()) {
        std::ostringstream errorMsgStream;
        errorMsgStream << "source field cannot be found in log line."
                       << "\tfirst 1KB log:" << contentValue.substr(0, 1024).to_string();
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }
    sourceValue = StringView(pch1 + 1, pch2 - pch1 - 1);

    if (sourceValue != "stdout" && sourceValue != "stderr") {
        std::ostringstream errorMsgStream;
        errorMsgStream << "source field not valid"
                       << "\tsource:" << sourceValue.to_string()
                       << "\tfirst 1KB log:" << contentValue.substr(0, 1024).to_string();
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }

    if (sourceValue == "stdout") {
        mProcParseStdoutTotal->Add(1);
        if (mIgnoringStdout) {
            return false;
        }
    } else {
        mProcParseStderrTotal->Add(1);
        if (mIgnoringStderr) {
            return false;
        }
    }

    // 如果既不以 CONTAINERD_PART_TAG 开头，也不以 CONTAINERD_FULL_TAG 开头
    if (*(pch2 + 1) != CONTAINERD_PART_TAG && *(pch2 + 1) != CONTAINERD_FULL_TAG) {
        // content
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        ResetContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    }

    // 寻找第三个分隔符位置
    const char* pch3 = std::find(pch2 + 1, contentValue.end(), CONTAINERD_DELIMITER);
    if (pch3 == contentValue.end() || pch3 != pch2 + 2) {
        // case: 2021-08-25T07:00:00.000000000Z stdout P
        // case: 2021-08-25T07:00:00.000000000Z stdout PP 1
        StringView content = StringView(pch2 + 1, contentValue.end() - pch2 - 1);
        ResetContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    }
    if (*(pch2 + 1) == CONTAINERD_FULL_TAG) {
        // F
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        ResetContainerdTextLog(timeValue, sourceValue, content, false, sourceEvent);
        return true;
    } else {
        // P
        StringView content = StringView(pch3 + 1, contentValue.end() - pch3 - 1);
        ResetContainerdTextLog(timeValue, sourceValue, content, true, sourceEvent);
        logGroup.SetMetadata(EventGroupMetaKey::LOG_PART_LOG, ProcessorMergeMultilineLogNative::PartLogFlag);
        return true;
    }
}

struct DockerLog {
    StringView log;
    std::string stream;
    std::string time;
};

enum class DockerLogType { Log, Stream, Time };

bool ParseDockerLog(char* buffer, int32_t size, DockerLog& dockerLog) {
    int32_t beginIdx = 0;

    int32_t idx = beginIdx;
    DockerLogType logType;
    while (idx < size) {
        if (buffer[idx] == '\"') {
            ++idx;
            if (buffer[idx] == 'l') {
                logType = DockerLogType::Log;
            } else if (buffer[idx] == 's') {
                logType = DockerLogType::Stream;
            } else if (buffer[idx] == 't') {
                logType = DockerLogType::Time;
            }
            while (buffer[idx] != '\"') {
                ++idx;
            }
            ++idx;
            // skip ' ' and ':'
            while (buffer[idx] == ':' || buffer[idx] == ' ') {
                ++idx;
            }
            ++idx; // skip '\"'
            char* valueBegion = buffer + beginIdx;
            size_t begin = 0;
            while (buffer[idx] != '\"') {
                if (buffer[idx] == '\\') {
                    ++idx; // skip escape char
                    if (buffer[idx] == '\"') {
                        switch (logType) {
                            case DockerLogType::Log:
                                buffer[beginIdx++] = '\"';
                                break;
                            case DockerLogType::Stream:
                                dockerLog.stream[begin++] = '\"';
                                break;
                            case DockerLogType::Time:
                                dockerLog.time[begin++] = '\"';
                                break;
                        }
                    } else if (buffer[idx] == '\\') {
                        switch (logType) {
                            case DockerLogType::Log:
                                buffer[beginIdx++] = '\\';
                                break;
                            case DockerLogType::Stream:
                                dockerLog.stream[begin++] = '\\';
                                break;
                            case DockerLogType::Time:
                                dockerLog.time[begin++] = '\\';
                                break;
                        }
                    }
                } else {
                    switch (logType) {
                        case DockerLogType::Log:
                            buffer[beginIdx++] = buffer[idx];
                            break;
                        case DockerLogType::Stream:
                            dockerLog.stream[begin++] = buffer[idx];
                            break;
                        case DockerLogType::Time:
                            dockerLog.time[begin++] = buffer[idx];
                            break;
                    }
                }
                ++idx;
            }
            ++idx; // skip '\"'
            while (idx < size && (buffer[idx] == ' ' || buffer[idx] == ',')) {
                ++idx; // skip ' ' or ','
            }
            switch (logType) {
                case DockerLogType::Log:
                    dockerLog.log = StringView(valueBegion, beginIdx - (valueBegion - buffer));
                    break;
                case DockerLogType::Stream:
                    dockerLog.stream.resize(begin);
                    break;
                case DockerLogType::Time:
                    dockerLog.time.resize(begin);
                    break;
            }
        } else {
            ++idx;
        }
    }

    return true;
}

bool ProcessorParseContainerLogNative::ParseDockerJsonLogLine(LogEvent& sourceEvent, std::string& errorMsg) {
    if (oldJson == 0) {
        char* env_var = std::getenv("old");
        if (env_var != nullptr && std::string(env_var) == "true") {
            oldJson = 1;
        } else {
            oldJson = 2;
        }
    }
    if (oldJson == 1) {
        StringView buffer = sourceEvent.GetContent(mSourceKey);

        bool parseSuccess = true;
        rapidjson::Document doc;
        doc.Parse(buffer.data(), buffer.size());
        if (doc.HasParseError()) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "parse docker stdout json log fail, rapidjson offset: " << doc.GetErrorOffset()
                           << "\trapidjson error: " << doc.GetParseError()
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024);
            errorMsg = errorMsgStream.str();
            parseSuccess = false;
        } else if (!doc.IsObject()) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "docker stdout json log line is not a valid json obejct."
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024);
            errorMsg = errorMsgStream.str();
            parseSuccess = false;
        }
        if (!parseSuccess) {
            return mKeepingSourceWhenParseFail;
        }

        // time
        auto it = doc.FindMember(DOCKER_JSON_TIME.c_str());
        if (it == doc.MemberEnd() || !it->value.IsString()) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "time field cannot be found in log line."
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
            errorMsg = errorMsgStream.str();
            return mKeepingSourceWhenParseFail;
        }
        StringView timeValue = StringView(it->value.GetString());

        // content
        it = doc.FindMember(DOCKER_JSON_LOG.c_str());
        if (it == doc.MemberEnd() || !it->value.IsString()) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "content field cannot be found in log line."
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
            errorMsg = errorMsgStream.str();
            return mKeepingSourceWhenParseFail;
        }
        StringView content = StringView(it->value.GetString());

        // source
        it = doc.FindMember(DOCKER_JSON_STREAM_TYPE.c_str());
        StringView sourceValue;
        if (it == doc.MemberEnd() || !it->value.IsString()) {
            sourceValue = StringView();
        } else {
            sourceValue = StringView(it->value.GetString());
        }
        if (sourceValue.empty() || (sourceValue != "stdout" && sourceValue != "stderr")) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "source field cannot be found in log line."
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
            errorMsg = errorMsgStream.str();
            return mKeepingSourceWhenParseFail;
        }

        if (sourceValue == "stdout") {
            mProcParseStdoutTotal->Add(1);
            if (mIgnoringStdout) {
                return false;
            }
        } else {
            mProcParseStderrTotal->Add(1);
            if (mIgnoringStderr) {
                return false;
            }
        }

        if (buffer.size() < content.size() + timeValue.size() + sourceValue.size()) {
            std::ostringstream errorMsgStream;
            errorMsgStream << "unexpected error: the original log line length is smaller than the sum of parsed fields."
                           << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
            errorMsg = errorMsgStream.str();
            return mKeepingSourceWhenParseFail;
        }

        char* data = const_cast<char*>(buffer.data());
        // time
        ResetDockerJsonLogField(data, containerTimeKey, timeValue, sourceEvent);
        data += timeValue.size();
        // source
        ResetDockerJsonLogField(data, containerSourceKey, sourceValue, sourceEvent);
        data += sourceValue.size();
        // content
        if (!content.empty() && content.back() == '\n') {
            content = StringView(content.data(), content.size() - 1);
        }
        ResetDockerJsonLogField(data, containerLogKey, content, sourceEvent);
        return true;
    }

    StringView buffer = sourceEvent.GetContent(mSourceKey);

    bool parseSuccess = true;

    DockerLog entry;
    entry.time.resize(40);
    entry.stream.resize(10);
    StringView timeValue, sourceValue, content;

    if (!IsValidJson(buffer.data(), buffer.size())) {
        std::ostringstream errorMsgStream;
        errorMsgStream << "docker stdout json log line is not a valid json obejct."
                       << "\tfirst 1KB log:" << buffer.substr(0, 1024);
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }
    char* data = const_cast<char*>(buffer.data());

    if (ParseDockerLog(data, buffer.size(), entry)) {
        timeValue = entry.time;
        content = entry.log;
        sourceValue = entry.stream;
    } else {
        std::ostringstream errorMsgStream;
        errorMsgStream << "docker stdout json log line is not a valid json obejct."
                       << "\tfirst 1KB log:" << buffer.substr(0, 1024);
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }

    if (sourceValue.empty() || (sourceValue != "stdout" && sourceValue != "stderr")) {
        std::ostringstream errorMsgStream;
        errorMsgStream << "source field cannot be found in log line."
                       << "sourceValue:\t" << sourceValue << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }

    if (sourceValue == "stdout") {
        mProcParseStdoutTotal->Add(1);
        if (mIgnoringStdout) {
            return false;
        }
    } else {
        mProcParseStderrTotal->Add(1);
        if (mIgnoringStderr) {
            return false;
        }
    }

    if (buffer.size() < content.size() + timeValue.size() + sourceValue.size()) {
        std::ostringstream errorMsgStream;
        errorMsgStream << "unexpected error: the original log line length is smaller than the sum of parsed fields."
                       << "\tfirst 1KB log:" << buffer.substr(0, 1024).to_string();
        errorMsg = errorMsgStream.str();
        return mKeepingSourceWhenParseFail;
    }

    // time
    sourceEvent.SetContent(containerTimeKey, timeValue);
    mProcParseOutSizeBytes->Add(containerTimeKey.size() + timeValue.size());

    // source
    sourceEvent.SetContent(containerSourceKey, sourceValue);
    mProcParseOutSizeBytes->Add(containerSourceKey.size() + sourceValue.size());

    // content
    if (!content.empty() && content.back() == '\n') {
        content = StringView(content.data(), content.size() - 1);
    }
    sourceEvent.SetContentNoCopy(containerLogKey, content);
    mProcParseOutSizeBytes->Add(containerLogKey.size() + content.size());

    return true;
}

void ProcessorParseContainerLogNative::ResetDockerJsonLogField(char* data,
                                                               StringView key,
                                                               StringView value,
                                                               LogEvent& targetEvent) {
    if (oldJson == 1) {
        memmove(data, value.data(), value.size());
        StringView valueBuffer = StringView(data, value.size());
        targetEvent.SetContentNoCopy(key, valueBuffer);
        mProcParseOutSizeBytes->Add(key.size() + valueBuffer.size());
    } else {
        targetEvent.SetContentNoCopy(key, value);
        mProcParseOutSizeBytes->Add(key.size() + value.size());
    }
}

void ProcessorParseContainerLogNative::ResetContainerdTextLog(
    StringView time, StringView source, StringView content, bool isPartialLog, LogEvent& sourceEvent) {
    sourceEvent.SetContentNoCopy(containerTimeKey, time);
    mProcParseOutSizeBytes->Add(containerTimeKey.size() + time.size());
    sourceEvent.SetContentNoCopy(containerSourceKey, source);
    mProcParseOutSizeBytes->Add(containerSourceKey.size() + source.size());
    if (isPartialLog) {
        sourceEvent.SetContentNoCopy(ProcessorMergeMultilineLogNative::PartLogFlag, StringView());
        mProcParseOutSizeBytes->Add(ProcessorMergeMultilineLogNative::PartLogFlag.size());
    }
    sourceEvent.SetContentNoCopy(containerLogKey, content);
    mProcParseOutSizeBytes->Add(containerLogKey.size() + content.size());
}

bool ProcessorParseContainerLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

} // namespace logtail
