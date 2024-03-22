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

#include "processor/ProcessorSplitMultilineLogStringNative.h"

#include <boost/regex.hpp>
#include <string>

#include "app_config/AppConfig.h"
#include "common/Constants.h"
#include "common/ParamExtractor.h"
#include "logger/Logger.h"
#include "models/LogEvent.h"
#include "monitor/MetricConstants.h"
#include "plugin/instance/ProcessorInstance.h"

namespace logtail {

const std::string ProcessorSplitMultilineLogStringNative::sName = "processor_split_multiline_log_string_native";

bool ProcessorSplitMultilineLogStringNative::Init(const Json::Value& config) {
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

    if (!mMultiline.Init(config, *mContext, sName)) {
        return false;
    }

    // AppendingLogPositionMeta
    if (!GetOptionalBoolParam(config, "AppendingLogPositionMeta", mAppendingLogPositionMeta, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mAppendingLogPositionMeta,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    mSplitLines = &(GetContext().GetProcessProfile().splitLines);

    mProcSplittedEventsCnt
        = GetMetricsRecordRef().CreateCounter(METRIC_PROC_SPLIT_MULTILINE_LOG_SPLITTED_RECORDS_TOTAL);
    mProcUnmatchedEventsCnt
        = GetMetricsRecordRef().CreateCounter(METRIC_PROC_SPLIT_MULTILINE_LOG_UNMATCHED_RECORDS_TOTAL);
    return true;
}

/*
    Presume:
    1. Events must be LogEvent
    2. This is an inner plugin, so the size of log content must equal to 2 (sourceKey, __file_offset__)
    3. The last character of each event must be \0 (set in LogFileReader)
*/
void ProcessorSplitMultilineLogStringNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    EventsContainer newEvents;
    StringView logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    for (PipelineEventPtr& e : logGroup.MutableEvents()) {
        ProcessEvent(logGroup, logPath, std::move(e), newEvents);
    }
    *mSplitLines = newEvents.size();
    mProcSplittedEventsCnt->Add(newEvents.size());
    logGroup.SwapEvents(newEvents);
}

bool ProcessorSplitMultilineLogStringNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    if (e.Is<LogEvent>()) {
        return true;
    }
    LOG_ERROR(
        mContext->GetLogger(),
        ("unexpected error", "some events are not supported")("processor", sName)("config", mContext->GetConfigName()));
    mContext->GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                   "unexpected error: some events are not supported.\tprocessor: " + sName
                                       + "\tconfig: " + mContext->GetConfigName(),
                                   mContext->GetProjectName(),
                                   mContext->GetLogstoreName(),
                                   mContext->GetRegion());
    return false;
}

void ProcessorSplitMultilineLogStringNative::ProcessEvent(PipelineEventGroup& logGroup,
                                                          StringView logPath,
                                                          PipelineEventPtr&& e,
                                                          EventsContainer& newEvents) {
    if (!IsSupportedEvent(e)) {
        newEvents.emplace_back(std::move(e));
        return;
    }
    const LogEvent& sourceEvent = e.Cast<LogEvent>();
    // This is an inner plugin, so the size of log content must equal to 2 (sourceKey, __file_offset__)
    if (sourceEvent.Size() != 2) {
        newEvents.emplace_back(std::move(e));
        LOG_ERROR(mContext->GetLogger(),
                  ("unexpected error", "size of event content doesn't equal to 2")("processor", sName)(
                      "config", mContext->GetConfigName()));
        mContext->GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                       "unexpected error: size of event content doesn't equal to 2.\tprocessor: " + sName
                                           + "\tconfig: " + mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
        return;
    }
    
    auto sourceIterator = sourceEvent.FindContent(mSourceKey);
    if (sourceIterator == sourceEvent.end()) {
        newEvents.emplace_back(std::move(e));
        LOG_ERROR(mContext->GetLogger(),
                  ("unexpected error", "some events do not have the SourceKey")(
                      "processor", sName)("config", mContext->GetConfigName()));
        mContext->GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                       "unexpected error: some events do not have the sourceKey.\tprocessor: " + sName
                                           + "\tconfig: " + mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
        return;
    }
    StringView sourceVal = sourceIterator->second;

    auto offsetIterator = sourceEvent.FindContent(LOG_RESERVED_KEY_FILE_OFFSET);
    if (offsetIterator == sourceEvent.end()) {
        newEvents.emplace_back(std::move(e));
        LOG_ERROR(mContext->GetLogger(),
                  ("unexpected error", "event do not have key __file_ofset__")("processor", sName)(
                      "config", mContext->GetConfigName()));
        mContext->GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                       "unexpected error: event do not have key __file_ofset__.\tprocessor"
                                           + sName
                                           + "\tconfig: " + mContext->GetConfigName(),
                                       mContext->GetProjectName(),
                                       mContext->GetLogstoreName(),
                                       mContext->GetRegion());
        return;
    }
    uint32_t sourceOffset = atol(offsetIterator->second.data());
    
    StringBuffer sourceKey = logGroup.GetSourceBuffer()->CopyString(mSourceKey);
    const char* multiStartIndex = nullptr;
    std::string exception;
    bool isPartialLog = false;
    if (mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
        && mMultiline.GetEndPatternReg() != nullptr) {
        // if only end pattern is given, then it will stick to this state
        isPartialLog = true;
        multiStartIndex = sourceVal.data();
    }


    size_t begin = 0;
    while (begin < sourceVal.size()) {
        StringView content = GetNextLine(sourceVal, begin);
        if (!isPartialLog) {
            // it is impossible to enter this state if only end pattern is given
            boost::regex regex;
            if (mMultiline.GetStartPatternReg() != nullptr) {
                regex = *mMultiline.GetStartPatternReg();
            } else {
                regex = *mMultiline.GetContinuePatternReg();
            }
            if (BoostRegexMatch(content.data(), content.size(), regex, exception)) {
                multiStartIndex = content.data();
                isPartialLog = true;
            } else if (mMultiline.GetEndPatternReg() != nullptr && mMultiline.GetStartPatternReg() == nullptr
                       && mMultiline.GetContinuePatternReg() != nullptr
                       && BoostRegexMatch(content.data(), content.size(), *mMultiline.GetEndPatternReg(), exception)) {
                // case: continue + end
                CreateNewEvent(content, sourceOffset, sourceKey, sourceEvent, logGroup, newEvents);
                multiStartIndex = content.data() + content.size() + 1;
            } else {
                HandleUnmatchLogs(content, sourceOffset, sourceKey, sourceEvent, logGroup, newEvents, logPath);
            }
        } else {
            // case: start + continue or continue + end
            if (mMultiline.GetContinuePatternReg() != nullptr
                && BoostRegexMatch(content.data(), content.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                begin += content.size() + 1;
                continue;
            }
            if (mMultiline.GetEndPatternReg() != nullptr) {
                // case: start + end or continue + end or end
                if (mMultiline.GetContinuePatternReg() != nullptr) {
                    // current line is not matched against the continue pattern, so the end pattern will decide
                    // if the current log is a match or not
                    if (BoostRegexMatch(content.data(), content.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        CreateNewEvent(StringView(multiStartIndex, content.data() + content.size() - multiStartIndex),
                                       sourceOffset,
                                       sourceKey,
                                       sourceEvent,
                                       logGroup,
                                       newEvents);
                    } else {
                        HandleUnmatchLogs(
                            StringView(multiStartIndex, content.data() + content.size() - multiStartIndex),
                            sourceOffset,
                            sourceKey,
                            sourceEvent,
                            logGroup,
                            newEvents,
                            logPath);
                    }
                    isPartialLog = false;
                } else {
                    // case: start + end or end
                    if (BoostRegexMatch(content.data(), content.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        CreateNewEvent(StringView(multiStartIndex, content.data() + content.size() - multiStartIndex),
                                       sourceOffset,
                                       sourceKey,
                                       sourceEvent,
                                       logGroup,
                                       newEvents);
                        if (mMultiline.GetStartPatternReg() != nullptr) {
                            isPartialLog = false;
                        } else {
                            multiStartIndex = content.data() + content.size() + 1;
                        }
                        // if only end pattern is given, start another log automatically
                    }
                    // no continue pattern given, and the current line in not matched against the end pattern,
                    // so wait for the next line
                }
            } else {
                if (mMultiline.GetContinuePatternReg() == nullptr) {
                    // case: start
                    if (BoostRegexMatch(content.data(), content.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        CreateNewEvent(StringView(multiStartIndex, content.data() - 1 - multiStartIndex),
                                       sourceOffset,
                                       sourceKey,
                                       sourceEvent,
                                       logGroup,
                                       newEvents);
                        multiStartIndex = content.data();
                    }
                } else {
                    // case: start + continue
                    // continue pattern is given, but current line is not matched against the continue pattern
                    CreateNewEvent(StringView(multiStartIndex, content.data() - 1 - multiStartIndex),
                                   sourceOffset,
                                   sourceKey,
                                   sourceEvent,
                                   logGroup,
                                   newEvents);
                    if (!BoostRegexMatch(content.data(), content.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        // when no end pattern is given, the only chance to enter unmatched state is when both
                        // start and continue pattern are given, and the current line is not matched against the
                        // start pattern
                        HandleUnmatchLogs(content, sourceOffset, sourceKey, sourceEvent, logGroup, newEvents, logPath);
                        isPartialLog = false;
                    } else {
                        multiStartIndex = content.data();
                    }
                }
            }
        }
        begin += content.size() + 1;
    }
    // when in unmatched state, the unmatched log is handled one by one, so there is no need for additional handle
    // here
    if (isPartialLog && multiStartIndex - sourceVal.data() < sourceVal.size()) {
        if (mMultiline.GetEndPatternReg() == nullptr) {
            CreateNewEvent(StringView(multiStartIndex, sourceVal.data() + sourceVal.size() - multiStartIndex),
                           sourceOffset,
                           sourceKey,
                           sourceEvent,
                           logGroup,
                           newEvents);
        } else {
            HandleUnmatchLogs(StringView(multiStartIndex, sourceVal.data() + sourceVal.size() - multiStartIndex),
                              sourceOffset,
                              sourceKey,
                              sourceEvent,
                              logGroup,
                              newEvents,
                              logPath);
        }
    }
}

void ProcessorSplitMultilineLogStringNative::CreateNewEvent(const StringView& content,
                                                            long sourceoffset,
                                                            StringBuffer& sourceKey,
                                                            const LogEvent& sourceEvent,
                                                            PipelineEventGroup& logGroup,
                                                            EventsContainer& newEvents) {
    StringView sourceVal = sourceEvent.GetContent(mSourceKey);
    std::unique_ptr<LogEvent> targetEvent = logGroup.CreateLogEvent();
    targetEvent->SetTimestamp(
        sourceEvent.GetTimestamp(),
        sourceEvent.GetTimestampNanosecond()); // it is easy to forget other fields, better solution?
    targetEvent->SetContentNoCopy(StringView(sourceKey.data, sourceKey.size), content);
    if (mAppendingLogPositionMeta) {
        auto const offset = sourceoffset + (content.data() - sourceVal.data());
        StringBuffer offsetStr = logGroup.GetSourceBuffer()->CopyString(std::to_string(offset));
        targetEvent->SetContentNoCopy(LOG_RESERVED_KEY_FILE_OFFSET, StringView(offsetStr.data, offsetStr.size));
    }
    newEvents.emplace_back(std::move(targetEvent));
}

void ProcessorSplitMultilineLogStringNative::HandleUnmatchLogs(const StringView& sourceVal,
                                                               long sourceoffset,
                                                               StringBuffer& sourceKey,
                                                               const LogEvent& sourceEvent,
                                                               PipelineEventGroup& logGroup,
                                                               EventsContainer& newEvents,
                                                               StringView logPath) {
    size_t begin = 0;
    while (begin < sourceVal.size()) {
        StringView content = GetNextLine(sourceVal, begin);
        mProcUnmatchedEventsCnt->Add(1);
        if (!mMultiline.mIgnoringUnmatchWarning && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            LOG_WARNING(
                GetContext().GetLogger(),
                ("unmatched log line", "please check regex")(
                    "action", UnmatchedContentTreatmentToString(mMultiline.mUnmatchedContentTreatment))(
                    "first 1KB", content.substr(0, 1024).to_string())("filepath", logPath.to_string())(
                    "processor", sName)("config", GetContext().GetConfigName())("log bytes", content.size() + 1));
            GetContext().GetAlarm().SendAlarm(
                SPLIT_LOG_FAIL_ALARM,
                "unmatched log line, first 1KB:" + content.substr(0, 1024).to_string() + "\taction: "
                    + UnmatchedContentTreatmentToString(mMultiline.mUnmatchedContentTreatment) + "\tfilepath: "
                    + logPath.to_string() + "\tprocessor: " + sName + "\tconfig: " + GetContext().GetConfigName(),
                GetContext().GetProjectName(),
                GetContext().GetLogstoreName(),
                GetContext().GetRegion());
        }
        if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
            CreateNewEvent(content, sourceoffset, sourceKey, sourceEvent, logGroup, newEvents);
        }
        begin += content.size() + 1;
    }
}

StringView ProcessorSplitMultilineLogStringNative::GetNextLine(StringView log, size_t begin) {
    if (begin >= log.size()) {
        return StringView();
    }

    for (size_t end = begin; end < log.size(); ++end) {
        if (log[end] == '\n') {
            return StringView(log.data() + begin, end - begin);
        }
    }
    return StringView(log.data() + begin, log.size() - begin);
}

} // namespace logtail
