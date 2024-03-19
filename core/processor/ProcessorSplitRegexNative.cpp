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

#include "processor/ProcessorSplitRegexNative.h"

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

const std::string ProcessorSplitRegexNative::sName = "processor_split_regex_native";

bool ProcessorSplitRegexNative::Init(const Json::Value& config) {
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

    mFeedLines = &(GetContext().GetProcessProfile().feedLines);
    mSplitLines = &(GetContext().GetProcessProfile().splitLines);

    mProcSplittedEventsCnt
        = GetMetricsRecordRef().CreateCounter(METRIC_PROC_SPLIT_MULTILINE_LOG_SPLITTED_RECORDS_TOTAL);
    mProcUnmatchedEventsCnt
        = GetMetricsRecordRef().CreateCounter(METRIC_PROC_SPLIT_MULTILINE_LOG_UNMATCHED_RECORDS_TOTAL);
    return true;
}

void ProcessorSplitRegexNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    EventsContainer newEvents;
    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    for (PipelineEventPtr& e : logGroup.MutableEvents()) {
        ProcessEvent(logGroup, logPath, std::move(e), newEvents);
    }
    *mSplitLines = newEvents.size();
    mProcSplittedEventsCnt->Add(newEvents.size());
    logGroup.SwapEvents(newEvents);

    return;
}

bool ProcessorSplitRegexNative::IsSupportedEvent(const PipelineEventPtr& e) const {
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

void ProcessorSplitRegexNative::ProcessEvent(PipelineEventGroup& logGroup,
                                             const StringView& logPath,
                                             PipelineEventPtr&& e,
                                             EventsContainer& newEvents) {
    if (!IsSupportedEvent(e)) {
        newEvents.emplace_back(std::move(e));
        return;
    }
    const LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        newEvents.emplace_back(std::move(e));
        return;
    }
    StringView sourceVal = sourceEvent.GetContent(mSourceKey);
    std::vector<StringView> logIndex; // all splitted logs
    std::vector<StringView> discardIndex; // used to send warning
    int feedLines = 0;
    bool splitSuccess = LogSplit(sourceVal.data(), sourceVal.size(), feedLines, logIndex, discardIndex, logPath);
    *mFeedLines += feedLines;
    mProcUnmatchedEventsCnt->Add(discardIndex.size());

    if (AppConfig::GetInstance()->IsLogParseAlarmValid() && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
        if (!splitSuccess) { // warning if unsplittable
            GetContext().GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                              "split log lines fail, please check log_begin_regex, file:"
                                                  + logPath.to_string()
                                                  + ", logs:" + sourceVal.substr(0, 1024).to_string(),
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
            LOG_ERROR(GetContext().GetLogger(),
                      ("split log lines fail", "please check log_begin_regex")("file_name", logPath)(
                          "log bytes", sourceVal.size() + 1)("first 1KB log", sourceVal.substr(0, 1024).to_string()));
        }
        for (auto& discardData : discardIndex) { // warning if data loss
            GetContext().GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                              "split log lines discard data, file:" + logPath.to_string()
                                                  + ", logs:" + discardData.substr(0, 1024).to_string(),
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
            LOG_WARNING(
                GetContext().GetLogger(),
                ("split log lines discard data", "please check log_begin_regex")("file_name", logPath)(
                    "log bytes", sourceVal.size() + 1)("first 1KB log", discardData.substr(0, 1024).to_string()));
        }
    }
    if (logIndex.size() == 0) {
        return;
    }
    long sourceoffset = 0L;
    if (sourceEvent.HasContent(LOG_RESERVED_KEY_FILE_OFFSET)) {
        sourceoffset = atol(sourceEvent.GetContent(LOG_RESERVED_KEY_FILE_OFFSET).data()); // use safer method
    }
    StringBuffer splitKey = logGroup.GetSourceBuffer()->CopyString(mSourceKey);
    for (auto& content : logIndex) {
        std::unique_ptr<LogEvent> targetEvent = logGroup.CreateLogEvent();
        targetEvent->SetTimestamp(
            sourceEvent.GetTimestamp(),
            sourceEvent.GetTimestampNanosecond()); // it is easy to forget other fields, better solution?
        targetEvent->SetContentNoCopy(StringView(splitKey.data, splitKey.size), content);
        if (mAppendingLogPositionMeta) {
            auto const offset = sourceoffset + (content.data() - sourceVal.data());
            StringBuffer offsetStr = logGroup.GetSourceBuffer()->CopyString(std::to_string(offset));
            targetEvent->SetContentNoCopy(LOG_RESERVED_KEY_FILE_OFFSET, StringView(offsetStr.data, offsetStr.size));
        }
        if (sourceEvent.GetContents().size() > 1) { // copy other fields
            for (auto& kv : sourceEvent.GetContents()) {
                if (kv.first != mSourceKey && kv.first != LOG_RESERVED_KEY_FILE_OFFSET) {
                    targetEvent->SetContentNoCopy(kv.first, kv.second);
                }
            }
        }
        newEvents.emplace_back(std::move(targetEvent));
    }
}

bool ProcessorSplitRegexNative::LogSplit(const char* buffer,
                                         int32_t size,
                                         int32_t& lineFeed,
                                         std::vector<StringView>& logIndex,
                                         std::vector<StringView>& discardIndex,
                                         const StringView& logPath) {
    /*
               | -------------- | -------- \n
        multiStartIndex      startIndex   endIndex

        multiStartIndex: used to cache current parsing log. Clear when starting the next log.
        startIndex: the begin index of the current line
        endIndex: the end index of the current line

        Supported regex combination:
        1. start
        2. start + continue
        3. start + end
        4. continue + end
        5. end
    */
    int multiStartIndex = 0;
    int startIndex = 0;
    int endIndex = 0;
    bool anyMatched = false;
    lineFeed = 0;
    std::string exception;
    bool isPartialLog = false;
    if (mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
        && mMultiline.GetEndPatternReg() != nullptr) {
        // if only end pattern is given, then it will stick to this state
        isPartialLog = true;
    }
    for (; endIndex <= size; endIndex++) {
        if (endIndex == size || buffer[endIndex] == '\n') {
            lineFeed++;
            exception.clear();
            if (!isPartialLog) {
                // it is impossible to enter this state if only end pattern is given
                boost::regex regex;
                if (mMultiline.GetStartPatternReg() != nullptr) {
                    regex = *mMultiline.GetStartPatternReg();
                } else {
                    regex = *mMultiline.GetContinuePatternReg();
                }
                if (BoostRegexMatch(buffer + startIndex, endIndex - startIndex, regex, exception)) {
                    multiStartIndex = startIndex;
                    isPartialLog = true;
                } else if (mMultiline.GetEndPatternReg() != nullptr && mMultiline.GetStartPatternReg() == nullptr
                           && mMultiline.GetContinuePatternReg() != nullptr
                           && BoostRegexMatch(
                               buffer + startIndex, endIndex - startIndex, *mMultiline.GetEndPatternReg(), exception)) {
                    // case: continue + end
                    // output logs in cache from multiStartIndex to endIndex
                    anyMatched = true;
                    logIndex.emplace_back(buffer + multiStartIndex, endIndex - multiStartIndex);
                    multiStartIndex = endIndex + 1;
                } else {
                    HandleUnmatchLogs(buffer, multiStartIndex, endIndex, logIndex, discardIndex);
                    multiStartIndex = endIndex + 1;
                }
            } else {
                // case: start + continue or continue + end
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        buffer + startIndex, endIndex - startIndex, *mMultiline.GetContinuePatternReg(), exception)) {
                    startIndex = endIndex + 1;
                    continue;
                }
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    // case: start + end or continue + end or end
                    if (mMultiline.GetContinuePatternReg() != nullptr) {
                        // current line is not matched against the continue pattern, so the end pattern will decide if
                        // the current log is a match or not
                        if (BoostRegexMatch(buffer + startIndex,
                                            endIndex - startIndex,
                                            *mMultiline.GetEndPatternReg(),
                                            exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiStartIndex, endIndex - multiStartIndex);
                        } else {
                            HandleUnmatchLogs(buffer, multiStartIndex, endIndex, logIndex, discardIndex);
                        }
                        multiStartIndex = endIndex + 1;
                        isPartialLog = false;
                    } else {
                        // case: start + end or end
                        if (BoostRegexMatch(buffer + startIndex,
                                            endIndex - startIndex,
                                            *mMultiline.GetEndPatternReg(),
                                            exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiStartIndex, endIndex - multiStartIndex);
                            multiStartIndex = endIndex + 1;
                            if (mMultiline.GetStartPatternReg() != nullptr) {
                                isPartialLog = false;
                            }
                            // if only end pattern is given, start another log automatically
                        }
                        // no continue pattern given, and the current line in not matched against the end pattern, so
                        // wait for the next line
                    }
                } else {
                    if (mMultiline.GetContinuePatternReg() == nullptr) {
                        // case: start
                        if (BoostRegexMatch(buffer + startIndex,
                                            endIndex - startIndex,
                                            *mMultiline.GetStartPatternReg(),
                                            exception)) {
                            logIndex.emplace_back(buffer + multiStartIndex, startIndex - 1 - multiStartIndex);
                            multiStartIndex = startIndex;
                        }
                    } else {
                        // case: start + continue
                        // continue pattern is given, but current line is not matched against the continue pattern
                        if (!BoostRegexMatch(buffer + startIndex,
                                             endIndex - startIndex,
                                             *mMultiline.GetStartPatternReg(),
                                             exception)) {
                            // when no end pattern is given, the only chance to enter unmatched state is when both start
                            // and continue pattern are given, and the current line is not matched against the start
                            // pattern
                            HandleUnmatchLogs(buffer, startIndex, endIndex, logIndex, discardIndex);
                            multiStartIndex = endIndex + 1;
                            isPartialLog = false;
                        } else {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiStartIndex, startIndex - 1 - multiStartIndex);
                            multiStartIndex = startIndex;
                        }
                    }
                }
            }
            startIndex = endIndex + 1;
            if (!exception.empty()) {
                if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                    if (GetContext().GetAlarm().IsLowLevelAlarmValid()) {
                        LOG_ERROR(GetContext().GetLogger(),
                                  ("regex_match in LogSplit fail, exception", exception)("project",
                                                                                         GetContext().GetProjectName())(
                                      "logstore", GetContext().GetLogstoreName())("file", logPath));
                    }
                    GetContext().GetAlarm().SendAlarm(REGEX_MATCH_ALARM,
                                                      "regex_match in LogSplit fail:" + exception + ", file"
                                                          + logPath.to_string(),
                                                      GetContext().GetProjectName(),
                                                      GetContext().GetLogstoreName(),
                                                      GetContext().GetRegion());
                }
            }
        }
    }
    // when in unmatched state, the unmatched log is handled one by one, so there is no need for additional handle here
    if (isPartialLog && multiStartIndex < size) {
        endIndex = buffer[size - 1] == '\n' ? size - 1 : size;
        if (mMultiline.GetEndPatternReg() == nullptr) {
            anyMatched = true;
            logIndex.emplace_back(buffer + multiStartIndex, endIndex - multiStartIndex);
        } else {
            HandleUnmatchLogs(buffer, multiStartIndex, endIndex, logIndex, discardIndex);
        }
    }
    return anyMatched;
}

void ProcessorSplitRegexNative::HandleUnmatchLogs(const char* buffer,
                                                  int& multiStartIndex,
                                                  int endIndex,
                                                  std::vector<StringView>& logIndex,
                                                  std::vector<StringView>& discardIndex) {
    if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
        for (int i = multiStartIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                discardIndex.emplace_back(buffer + multiStartIndex, i - multiStartIndex);
                multiStartIndex = i + 1;
            }
        }
    } else if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
        for (int i = multiStartIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                logIndex.emplace_back(buffer + multiStartIndex, i - multiStartIndex);
                multiStartIndex = i + 1;
            }
        }
    }
}

} // namespace logtail
