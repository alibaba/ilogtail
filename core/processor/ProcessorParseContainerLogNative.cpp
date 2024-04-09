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


static const std::unordered_map<char, char> escapeMapping
    = {{'\"', '\"'}, {'\\', '\\'}, {'/', '/'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'}, {'r', '\r'}, {'t', '\t'}};

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

char16_t hexToChar(const std::string& hex) {
    std::istringstream iss(hex);
    int result;
    iss >> std::hex >> result;
    return static_cast<char16_t>(result);
}

// 将UTF-16编码的字符转换为UTF-8编码的字符串
std::string utf16ToUtf8(char16_t unicode) {
    std::string utf8;
    if (unicode <= 0x7F) {
        // 一字节UTF-8编码
        utf8 += static_cast<char>(unicode);
    } else if (unicode <= 0x7FF) {
        // 两字节UTF-8编码
        utf8 += static_cast<char>(0xC0 | ((unicode >> 6) & 0x1F));
        utf8 += static_cast<char>(0x80 | (unicode & 0x3F));
    } else {
        // 三字节UTF-8编码
        utf8 += static_cast<char>(0xE0 | ((unicode >> 12) & 0x0F));
        utf8 += static_cast<char>(0x80 | ((unicode >> 6) & 0x3F));
        utf8 += static_cast<char>(0x80 | (unicode & 0x3F));
    }
    return utf8;
}

// buffer: {"log":"Hello, World!","stream":"stdout","time":"2021-12-01T00:00:00.000Z"}
bool ProcessorParseContainerLogNative::ParseDockerLog(char* buffer, int32_t size, DockerLog& dockerLog) {
    if (size == 0 || buffer[0] != '{' || buffer[size - 1] != '}') {
        return false;
    }
    int logTypeCnt = 0;
    int32_t beginIdx = 0;
    int32_t idx = beginIdx;
    // skip '{'
    if (buffer[idx] != '{') {
        return false;
    }
    ++idx;
    if (idx == size) {
        return false;
    }
    DockerLogType logType;
    while (idx < size) {
        // skip ' '
        while (buffer[idx] == ' ') {
            ++idx;
            if (idx == size) {
                return false;
            }
        }
        // skip '}'
        if (buffer[idx] == '}') {
            if (idx == size - 1) {
                return true;
            }
            return false;
        }
        if (buffer[idx] == '\"') {
            ++idx;
            if (idx + ProcessorParseContainerLogNative::DOCKER_JSON_LOG.size() < size
                && std::equal(ProcessorParseContainerLogNative::DOCKER_JSON_LOG.begin(),
                              ProcessorParseContainerLogNative::DOCKER_JSON_LOG.end(),
                              &buffer[idx])) {
                idx += ProcessorParseContainerLogNative::DOCKER_JSON_LOG.size();
                logType = DockerLogType::Log;
            } else if (idx + ProcessorParseContainerLogNative::DOCKER_JSON_STREAM_TYPE.size() < size
                       && std::equal(ProcessorParseContainerLogNative::DOCKER_JSON_STREAM_TYPE.begin(),
                                     ProcessorParseContainerLogNative::DOCKER_JSON_STREAM_TYPE.end(),
                                     &buffer[idx])) {
                idx += ProcessorParseContainerLogNative::DOCKER_JSON_STREAM_TYPE.size();
                logType = DockerLogType::Stream;
            } else if (idx + ProcessorParseContainerLogNative::DOCKER_JSON_TIME.size() < size
                       && std::equal(ProcessorParseContainerLogNative::DOCKER_JSON_TIME.begin(),
                                     ProcessorParseContainerLogNative::DOCKER_JSON_TIME.end(),
                                     &buffer[idx])) {
                idx += ProcessorParseContainerLogNative::DOCKER_JSON_TIME.size();
                logType = DockerLogType::Time;
            } else {
                return false;
            }
            ++logTypeCnt;
            // skip '\"'
            if (buffer[idx] != '\"') {
                return false;
            }
            ++idx;
            if (idx == size) {
                return false;
            }
            // skip ' '
            while (buffer[idx] == ' ') {
                ++idx;
                if (idx == size) {
                    return false;
                }
            }
            // skip ':'
            if (buffer[idx] != ':') {
                return false;
            }
            ++idx;
            if (idx == size) {
                return false;
            }
            // skip ' '
            while (buffer[idx] == ' ') {
                ++idx;
                if (idx == size) {
                    return false;
                }
            }

            // skip '"'
            if (buffer[idx] != '\"') {
                return false;
            }
            ++idx;
            if (idx == size) {
                return false;
            }
            beginIdx = idx;
            char* valueBegion = buffer + beginIdx;
            size_t begin = 0;
            while (buffer[idx] != '\"') {
                if (idx == size) {
                    return false;
                }
                if (buffer[idx] == '\\') {
                    switch (logType) {
                        case DockerLogType::Log:
                            break;
                        case DockerLogType::Stream:
                        case DockerLogType::Time:
                            return false;
                    }
                    ++idx; // skip escape char
                    if (idx == size) {
                        return false;
                    }
                    auto it = escapeMapping.find(buffer[idx]);
                    if (it != escapeMapping.end()) {
                        buffer[beginIdx++] = it->second;
                    } else if (idx + 4 < size && buffer[idx] == 'u') {
                        std::string hex;
                        hex.append(buffer + idx + 1, 4);
                        char16_t unicodeChar = hexToChar(hex);
                        std::string utf8 = utf16ToUtf8(unicodeChar);
                        strcpy(buffer + beginIdx, utf8.c_str());
                        beginIdx += utf8.size();
                        idx += 4;
                    } else {
                        buffer[beginIdx++] = '\\';
                        buffer[beginIdx++] = buffer[idx];
                    }
                } else {
                    buffer[beginIdx++] = buffer[idx];
                }
                ++idx;
            }
            ++idx; // skip '\"'
            if (idx == size) {
                return false;
            }
            // skip ' '
            while (buffer[idx] == ' ') {
                ++idx;
                if (idx == size) {
                    return false;
                }
            }
            // skip ','
            if (logTypeCnt < 3) {
                if (buffer[idx] != ',') {
                    return false;
                } else {
                    ++idx;
                }
            }
            if (idx == size) {
                return false;
            }
            // skip ' '
            while (buffer[idx] == ' ') {
                ++idx;
                if (idx == size) {
                    return false;
                }
            }
            switch (logType) {
                case DockerLogType::Log:
                    dockerLog.log = StringView(valueBegion, beginIdx - (valueBegion - buffer));
                    break;
                case DockerLogType::Stream:
                    dockerLog.stream = StringView(valueBegion, beginIdx - (valueBegion - buffer));
                    break;
                case DockerLogType::Time:
                    dockerLog.time = StringView(valueBegion, beginIdx - (valueBegion - buffer));
                    break;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool ProcessorParseContainerLogNative::ParseDockerJsonLogLine(LogEvent& sourceEvent, std::string& errorMsg) {
    StringView buffer = sourceEvent.GetContent(mSourceKey);

    bool parseSuccess = true;

    DockerLog entry;
    StringView timeValue, sourceValue, content;

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
