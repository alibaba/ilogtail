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

#include "app_config/AppConfig.h"
#include "common/Constants.h"
#include "reader/LogFileReader.h" //SplitState
#include "models/LogEvent.h"
#include "logger/Logger.h"
#include "plugin/instance/ProcessorInstance.h"
#include "monitor/MetricConstants.h"


namespace logtail {
const std::string ProcessorSplitRegexNative::sName = "processor_split_regex_native";

bool ProcessorSplitRegexNative::Init(const ComponentConfig& componentConfig) {
    const PipelineConfig& config = componentConfig.GetConfig();

    mSplitKey = DEFAULT_CONTENT_KEY;
    mIsMultline = config.IsMultiline();
    SetLogMultilinePolicy(config.mLogBeginReg, config.mLogContinueReg, config.mLogEndReg);
    mDiscardUnmatch = config.mDiscardUnmatch;
    mEnableLogPositionMeta = config.mAdvancedConfig.mEnableLogPositionMeta;
    mFeedLines = &(GetContext().GetProcessProfile().feedLines);
    mSplitLines = &(GetContext().GetProcessProfile().splitLines);
    return true;
}

void ProcessorSplitRegexNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    EventsContainer newEvents;
    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
    for (const PipelineEventPtr& e : logGroup.GetEvents()) {
        ProcessEvent(logGroup, logPath, e, newEvents);
    }
    *mSplitLines = newEvents.size();
    logGroup.SwapEvents(newEvents);

    return;
}

bool ProcessorSplitRegexNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

void ProcessorSplitRegexNative::ProcessEvent(PipelineEventGroup& logGroup,
                                             const StringView& logPath,
                                             const PipelineEventPtr& e,
                                             EventsContainer& newEvents) {
    if (!IsSupportedEvent(e)) {
        newEvents.emplace_back(e);
        return;
    }
    const LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSplitKey)) {
        newEvents.emplace_back(e);
        return;
    }
    StringView sourceVal = sourceEvent.GetContent(mSplitKey);
    std::vector<StringView> logIndex; // all splitted logs
    std::vector<StringView> discardIndex; // used to send warning
    int feedLines = 0;
    bool splitSuccess = LogSplit(sourceVal.data(), sourceVal.size(), feedLines, logIndex, discardIndex, logPath);
    *mFeedLines += feedLines;

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
    StringBuffer splitKey = logGroup.GetSourceBuffer()->CopyString(mSplitKey);
    for (auto& content : logIndex) {
        std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(logGroup.GetSourceBuffer());
        targetEvent->SetTimestamp(sourceEvent.GetTimestamp(), sourceEvent.GetTimestampNanosecond()); // it is easy to forget other fields, better solution?
        targetEvent->SetContentNoCopy(StringView(splitKey.data, splitKey.size), content);
        if (mEnableLogPositionMeta) {
            auto const offset = sourceoffset + (content.data() - sourceVal.data());
            StringBuffer offsetStr = logGroup.GetSourceBuffer()->CopyString(std::to_string(offset));
            targetEvent->SetContentNoCopy(LOG_RESERVED_KEY_FILE_OFFSET, StringView(offsetStr.data, offsetStr.size));
        }
        if (sourceEvent.GetContents().size() > 1) { // copy other fields
            for (auto& kv : sourceEvent.GetContents()) {
                if (kv.first != mSplitKey && kv.first != LOG_RESERVED_KEY_FILE_OFFSET) {
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
        multiBeginIndex      begIndex   endIndex

        multiBeginIndex: used to cache current parsing log. Clear when starting the next log.
        begIndex: the begin index of the current line
        endIndex: the end index of the current line

        Supported regex combination:
        1. begin
        2. begin + continue
        3. begin + end
        4. continue + end
        5. end
    */
    int multiBeginIndex = 0;
    int begIndex = 0;
    int endIndex = 0;
    bool anyMatched = false;
    lineFeed = 0;
    std::string exception;
    SplitState state = SPLIT_UNMATCH;
    while (endIndex <= size) {
        if (endIndex == size || buffer[endIndex] == '\n') {
            lineFeed++;
            exception.clear();
            // State machine with three states (SPLIT_UNMATCH, SPLIT_BEGIN, SPLIT_CONTINUE)
            switch (state) {
                case SPLIT_UNMATCH:
                    if (!mIsMultline) {
                        // Single line log
                        anyMatched = true;
                        logIndex.emplace_back(buffer + begIndex, endIndex - begIndex);
                        multiBeginIndex = endIndex + 1;
                        break;
                    } else if (mLogBeginRegPtr != nullptr) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            // Just clear old cache, task current line as the new cache
                            if (multiBeginIndex != begIndex) {
                                anyMatched = true;
                                logIndex[logIndex.size() - 1] = StringView(logIndex[logIndex.size() - 1].begin(),
                                                                           logIndex[logIndex.size() - 1].length()
                                                                               + begIndex - 1 - multiBeginIndex);
                                multiBeginIndex = begIndex;
                            }
                            state = SPLIT_BEGIN;
                            break;
                        }
                        HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                        break;
                    }
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try mLogEndRegPtr
                    if (mLogContinueRegPtr != nullptr
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        state = SPLIT_CONTINUE;
                        break;
                    }
                    if (mLogEndRegPtr != nullptr
                        && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                        // output logs in cache from multiBeginIndex to endIndex
                        anyMatched = true;
                        logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                        multiBeginIndex = endIndex + 1;
                        break;
                    }
                    HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                    break;

                case SPLIT_BEGIN:
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try others.
                    if (mLogContinueRegPtr != nullptr && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        state = SPLIT_CONTINUE;
                        break;
                    }
                    if (mLogEndRegPtr != nullptr) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                            multiBeginIndex = endIndex + 1;
                            state = SPLIT_UNMATCH;
                        }
                        // for case: begin unmatch end
                        // so logs cannot be handled as unmatch even if not match LogEngReg
                    } else if (mLogBeginRegPtr != nullptr) {
                        anyMatched = true;
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            if (multiBeginIndex != begIndex) {
                                logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                                multiBeginIndex = begIndex;
                            }
                        } else if (mLogContinueRegPtr != nullptr) {
                            // case: begin+continue, but we meet unmatch log here
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                        // else case: begin+end or begin, we should keep unmatch log in the cache
                    }
                    break;

                case SPLIT_CONTINUE:
                    // mLogContinueRegPtr can be matched 0 or multiple times, if not match continue to try others.
                    if (mLogContinueRegPtr != nullptr && BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogContinueRegPtr, exception)) {
                        break;
                    }
                    if (mLogEndRegPtr != nullptr) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogEndRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
                            multiBeginIndex = endIndex + 1;
                            state = SPLIT_UNMATCH;
                        } else {
                            HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                    } else if (mLogBeginRegPtr != nullptr) {
                        if (BoostRegexMatch(buffer + begIndex, endIndex - begIndex, *mLogBeginRegPtr, exception)) {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            state = SPLIT_BEGIN;
                        } else {
                            anyMatched = true;
                            logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                            multiBeginIndex = begIndex;
                            HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                            state = SPLIT_UNMATCH;
                        }
                    } else {
                        anyMatched = true;
                        logIndex.emplace_back(buffer + multiBeginIndex, begIndex - 1 - multiBeginIndex);
                        multiBeginIndex = begIndex;
                        HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
            }
            begIndex = endIndex + 1;
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
        endIndex++;
    }
    // We should clear the log from `multiBeginIndex` to `size`.
    if (multiBeginIndex < size) {
        if (!mIsMultline) {
            logIndex.emplace_back(buffer + multiBeginIndex, size - multiBeginIndex);
        } else {
            endIndex = buffer[size-1] == '\n' ? size -1 : size;
            if (mLogBeginRegPtr != NULL && mLogEndRegPtr == NULL) {
                anyMatched = true;
                // If logs is unmatched, they have been handled immediately. So logs must be matched here.
                logIndex.emplace_back(buffer + multiBeginIndex, endIndex - multiBeginIndex);
            } else if (mLogBeginRegPtr == NULL && mLogContinueRegPtr == NULL && mLogEndRegPtr != NULL) {
                // If there is still logs in cache, it means that there is no end line. We can handle them as unmatched.
                if (mDiscardUnmatch) {
                    for (int i = multiBeginIndex; i <= endIndex; i++) {
                        if (i == endIndex || buffer[i] == '\n') {
                            discardIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                            multiBeginIndex = i + 1;
                        }
                    }
                } else {
                    for (int i = multiBeginIndex; i <= endIndex; i++) {
                        if (i == endIndex || buffer[i] == '\n') {
                            logIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                            multiBeginIndex = i + 1;
                        }
                    }
                }
            } else {
                HandleUnmatchLogs(buffer, multiBeginIndex, endIndex, logIndex, discardIndex);
            }
        }
    }
    return anyMatched;
}

void ProcessorSplitRegexNative::HandleUnmatchLogs(const char* buffer,
                                                  int& multiBeginIndex,
                                                  int endIndex,
                                                  std::vector<StringView>& logIndex,
                                                  std::vector<StringView>& discardIndex) {
    // Cannot determine where log is unmatched here where there is only mLogEndRegPtr
    if (mLogBeginRegPtr == nullptr && mLogContinueRegPtr == nullptr && mLogEndRegPtr != nullptr) {
        return;
    }
    if (mDiscardUnmatch) {
        for (int i = multiBeginIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                discardIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                multiBeginIndex = i + 1;
            }
        }
    } else {
        for (int i = multiBeginIndex; i <= endIndex; i++) {
            if (i == endIndex || buffer[i] == '\n') {
                logIndex.emplace_back(buffer + multiBeginIndex, i - multiBeginIndex);
                multiBeginIndex = i + 1;
            }
        }
    }
}

void ProcessorSplitRegexNative::SetLogMultilinePolicy(const std::string& begReg,
                                                      const std::string& conReg,
                                                      const std::string& endReg) {
    mLogBeginReg = begReg;
    if (mLogBeginRegPtr != nullptr) {
        mLogBeginRegPtr.reset();
    }
    if (begReg.empty() == false && begReg != ".*") {
        mLogBeginRegPtr.reset(new boost::regex(begReg));
    }
    mLogContinueReg = conReg;
    if (mLogContinueRegPtr != nullptr) {
        mLogContinueRegPtr.reset();
    }
    if (conReg.empty() == false && conReg != ".*") {
        mLogContinueRegPtr.reset(new boost::regex(conReg));
    }
    mLogEndReg = endReg;
    if (mLogEndRegPtr != nullptr) {
        mLogEndRegPtr.reset();
    }
    if (endReg.empty() == false && endReg != ".*") {
        mLogEndRegPtr.reset(new boost::regex(endReg));
    }
}

} // namespace logtail