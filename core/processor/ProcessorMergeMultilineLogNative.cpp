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

#include "processor/ProcessorMergeMultilineLogNative.h"

#include <boost/regex.hpp>
#include <string>

#include "app_config/AppConfig.h"
#include "common/Constants.h"
#include "common/ParamExtractor.h"
#include "logger/Logger.h"
#include "models/LogEvent.h"
#include "monitor/MetricConstants.h"
#include "plugin/instance/ProcessorInstance.h"
#include "reader/LogFileReader.h" //SplitState

namespace logtail {

const std::string ProcessorMergeMultilineLogNative::sName = "processor_merge_multiline_log_native";
const std::string ProcessorMergeMultilineLogNative::PartLogFlag = "P";

bool ProcessorMergeMultilineLogNative::Init(const Json::Value& config) {
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

    std::string mergeType = "regex";
    if (!GetOptionalStringParam(config, "MergeType", mergeType, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mergeType,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    } else if (mergeType == "flag") {
        mMergeType = MergeType::BY_FLAG;
    } else if (mergeType == "JSON") {
        mMergeType = MergeType::BY_JSON;
    } else if (mergeType == "regex") {
        if (!mMultiline.Init(config, *mContext, sName)) {
            return false;
        }
    } else {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           "string param MergeType is not valid",
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    mFeedLines = &(GetContext().GetProcessProfile().feedLines);
    mSplitLines = &(GetContext().GetProcessProfile().splitLines);

    return true;
}

void ProcessorMergeMultilineLogNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    if (mMergeType == MergeType::BY_REGEX) {
        MergeLogsByRegex(logGroup);
    } else if (mMergeType == MergeType::BY_FLAG) {
        MergeLogsByFlag(logGroup);
    }
    *mSplitLines = logGroup.GetEvents().size();
    return;
}

bool ProcessorMergeMultilineLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

void ProcessorMergeMultilineLogNative::MergeLogsByFlag(PipelineEventGroup& logGroup) {
    std::vector<size_t> logEventIndex;
    // 遍历event，对每个event进行合并
    auto& events = logGroup.MutableEvents();
    std::vector<LogEvent*> logEvents;
    bool isPartialLog = false;
    size_t beginIndex = 0;
    for (size_t endIndex = 0; endIndex < events.size(); ++endIndex) {
        logEvents.emplace_back(&events[endIndex].Cast<LogEvent>());
        LogEvent* sourceEvent = logEvents[logEvents.size() - 1];
        if (sourceEvent->GetContents().empty()) {
            continue;
        }

        // case: p p p ... p(lastIndex) notP(endIndex)
        if (isPartialLog && !sourceEvent->HasContent(PartLogFlag)) {
            MergeEvents(logEvents, beginIndex, endIndex, logEventIndex, false, false);
            isPartialLog = false;
            continue;
        }

        // case: notP(lastIndex) notP(endIndex)
        if (!isPartialLog && !sourceEvent->HasContent(PartLogFlag)) {
            MergeEvents(logEvents, endIndex, endIndex, logEventIndex, false, false);
            continue;
        }

        // case: F(lastIndex) p(endIndex)
        if (!isPartialLog) {
            isPartialLog = true;
            beginIndex = endIndex;
            auto& contents = sourceEvent->MutableContents();
            contents.erase(PartLogFlag);
        }
    }
    if (isPartialLog) {
        MergeEvents(logEvents, beginIndex, events.size() - 1, logEventIndex, false, false);
    }

    int begin = 0;
    for (size_t i = 0; i < logEventIndex.size(); ++i) {
        events[begin++] = events[logEventIndex[i]];
    }
    events.resize(begin);
}

void ProcessorMergeMultilineLogNative::MergeLogsByRegex(PipelineEventGroup& logGroup) {
    std::vector<size_t> logEventIndex;
    std::vector<size_t> discardLogEventIndex;

    std::vector<LogEvent*> logEvents;
    bool splitSuccess = LogSplit(logGroup, logEvents, logEventIndex, discardLogEventIndex);

    if (AppConfig::GetInstance()->IsLogParseAlarmValid() && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
        const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
        if (!splitSuccess) { // warning if unsplittable
            // 获取1024k的日志内容
            int logSize = 0;
            StringView dataValue;
            const char* begin = nullptr;
            for (const PipelineEventPtr& e : logGroup.GetEvents()) {
                if (logSize > 1024) {
                    logSize = 1024;
                    break;
                }
                const LogEvent& logEvent = e.Cast<LogEvent>();
                if (begin == nullptr) {
                    begin = logEvent.GetContent(mSourceKey).data();
                }
                logSize += logEvent.GetContent(mSourceKey).size();
            }
            if (begin != nullptr) {
                dataValue = StringView(begin, logSize);
                GetContext().GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                                  "split log lines fail, please check log_begin_regex, file:"
                                                      + logPath.to_string()
                                                      + ", logs:" + dataValue.substr(0, 1024).to_string(),
                                                  GetContext().GetProjectName(),
                                                  GetContext().GetLogstoreName(),
                                                  GetContext().GetRegion());
                LOG_ERROR(
                    GetContext().GetLogger(),
                    ("split log lines fail", "please check log_begin_regex")("file_name", logPath)(
                        "log bytes", dataValue.size() + 1)("first 1KB log", dataValue.substr(0, 1024).to_string()));
            }
        }
        for (const auto& discardData : discardLogEventIndex) { // warning if data loss
            const LogEvent* logEvent = logEvents[discardData];
            auto& sourceVal = logEvent->GetContent(mSourceKey);

            GetContext().GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                              "split log lines discard data, file:" + logPath.to_string()
                                                  + ", logs:" + sourceVal.substr(0, 1024).to_string(),
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
            LOG_WARNING(GetContext().GetLogger(),
                        ("split log lines discard data", "please check log_begin_regex")("file_name", logPath)(
                            "log bytes", sourceVal.size() + 1)("first 1KB log", sourceVal.substr(0, 1024).to_string()));
        }
    }


    int begin = 0;
    auto& events = logGroup.MutableEvents();
    for (size_t i = 0; i < logEventIndex.size(); ++i) {
        events[begin++] = events[logEventIndex[i]];
    }
    events.resize(begin);
}

bool ProcessorMergeMultilineLogNative::LogSplit(PipelineEventGroup& logGroup,
                                                std::vector<LogEvent*>& logEvents,
                                                std::vector<size_t>& logEventIndex,
                                                std::vector<size_t>& discardLogEventIndex) {
    /*
               | -------------- | -------- \n
        multiBeginIndex        curIndex

        multiBeginIndex: used to cache current parsing log. Clear when starting the next log.
        curIndex: current line

        Supported regex combination:
        1. begin
        2. begin + continue
        3. begin + end
        4. continue + end
        5. end
    */
    size_t multiBeginIndex = 0;
    bool splitSuccess = false;
    std::string exception;
    SplitState state = SPLIT_UNMATCH;

    // 遍历event，对每个event进行合并
    auto& events = logGroup.MutableEvents();
    for (size_t curIndex = 0; curIndex < events.size(); ++curIndex) {
        logEvents.emplace_back(&events[curIndex].Cast<LogEvent>());
        const LogEvent* sourceEvent = logEvents[logEvents.size() - 1];
        if (sourceEvent->GetContents().empty()) {
            continue;
        }
        exception.clear();
        StringView sourceVal = sourceEvent->GetContent(mSourceKey);

        switch (state) {
            case SPLIT_UNMATCH:
                // 行首不为空
                if (mMultiline.GetStartPatternReg() != nullptr) {
                    // 匹配到了行首
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        // 如果当前行是新的日志的开始，那么清空之前的缓存
                        if (curIndex != multiBeginIndex) {
                            splitSuccess = true;
                            // 把 multiBeginIndex 到 curIndex-1 的日志合并到multiBeginIndex中
                            MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex, true);

                            multiBeginIndex = curIndex;
                        }
                        state = SPLIT_BEGIN;
                        break;
                    }
                    // 没有匹配到行首
                    HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                    break;
                }
                // ContinuePatternReg可以匹配0次或多次，如果不匹配，请继续尝试EndPatternReg
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    // 存在continuePattern，且当前行匹配上，切换到continue状态
                    state = SPLIT_CONTINUE;
                    break;
                }
                if (mMultiline.GetEndPatternReg() != nullptr
                    && BoostRegexMatch(sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                    // EndPatternReg存在，且当前行匹配上
                    // 把 multiBeginIndex 到 curIndex 的日志合并到multiBeginIndex中
                    MergeEvents(logEvents, multiBeginIndex, curIndex, logEventIndex);
                    splitSuccess = true;
                    multiBeginIndex = curIndex + 1;
                    break;
                }
                // 没有匹配到任何正则
                HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                break;

            case SPLIT_BEGIN:
                // ContinuePatternReg可以匹配任意次，如果不匹配则继续尝试其他。
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    // ContinuePatternReg存在，且当前行匹配上，切换到continue状态
                    state = SPLIT_CONTINUE;
                    break;
                }

                // EndPatternReg存在，且当前行匹配上
                // 把multiBeginIndex到curIndex的日志合并到multiBeginIndex中
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        MergeEvents(logEvents, multiBeginIndex, curIndex, logEventIndex);
                        splitSuccess = true;
                        multiBeginIndex = curIndex + 1;
                        state = SPLIT_UNMATCH;
                    }
                    // for case: begin unmatch ... unmatch end
                    // so logs cannot be handled as unmatch even if not match LogEngReg
                    break;
                }

                if (mMultiline.GetStartPatternReg() != nullptr) {
                    splitSuccess = true;
                    // StartPattern存在，且当前行匹配上
                    // 如果multiBeginIndex不是当前行
                    // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        if (multiBeginIndex != curIndex) {
                            MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex);
                            multiBeginIndex = curIndex;
                        }
                    } else if (mMultiline.GetContinuePatternReg() != nullptr) {
                        // case: begin+continue, but we meet unmatch log here
                        // 当前行ContinuePatternReg和EndPatternReg都匹配不上
                        // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                        MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex);
                        multiBeginIndex = curIndex;
                        HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                        state = SPLIT_UNMATCH;
                    }
                    // else case: begin+end or begin, we should keep unmatch log in the cache
                    break;
                }
                break;

            case SPLIT_CONTINUE:
                // ContinuePatternReg可以匹配任意次，如果不匹配则继续尝试其他。
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    break;
                }

                // EndPatternReg存在
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        // 当前行为END
                        //  **** CONTINUE + END ****
                        // 把multiBeginIndex到curIndex的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(logEvents, multiBeginIndex, curIndex, logEventIndex);
                        multiBeginIndex = curIndex + 1;
                        state = SPLIT_UNMATCH;
                    } else {
                        // 当前行不是END, multiBeginIndex 到 curIndex 为 无效多行日志
                        HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
                }

                if (mMultiline.GetStartPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        // 当前行为START
                        // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex);
                        multiBeginIndex = curIndex;
                        state = SPLIT_BEGIN;
                    } else {
                        // 当前行为START
                        // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex);
                        multiBeginIndex = curIndex;
                        HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
                }

                // EndPatternReg不存在，StartPattern不存在，ContinuePatternReg没匹配上(理论上不支持这种正则配置)
                // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                splitSuccess = true;
                MergeEvents(logEvents, multiBeginIndex, curIndex - 1, logEventIndex);
                multiBeginIndex = curIndex;
                HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                state = SPLIT_UNMATCH;
                break;
        }
        if (!exception.empty()) {
            if (AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);
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
    // We should clear the log from `multiBeginIndex` to `size`.
    if (multiBeginIndex < events.size()) {
        if (mMultiline.GetStartPatternReg() != NULL && mMultiline.GetEndPatternReg() == NULL) {
            splitSuccess = true;
            // If logs is unmatched, they have been handled immediately. So logs must be matched here.
            MergeEvents(logEvents, multiBeginIndex, events.size() - 1, logEventIndex);
        } else if (mMultiline.GetStartPatternReg() == NULL && mMultiline.GetContinuePatternReg() == NULL
                   && mMultiline.GetEndPatternReg() != NULL) {
            // If there is still logs in cache, it means that there is no end line. We can handle them as unmatched.
            HandleUnmatchLogs(events, multiBeginIndex, events.size() - 1, logEventIndex, discardLogEventIndex, true);
        } else {
            HandleUnmatchLogs(events, multiBeginIndex, events.size() - 1, logEventIndex, discardLogEventIndex, true);
        }
    }
    return splitSuccess;
}

void ProcessorMergeMultilineLogNative::MergeEvents(std::vector<LogEvent*>& logEvents,
                                                   size_t beginIndex,
                                                   size_t endIndex,
                                                   std::vector<size_t>& logEventIndex,
                                                   bool updateLast,
                                                   bool insertLineBreak) {
    // 把 beginIndex 到 endIndex 的日志合并到 beginIndex 中
    auto& beginEvent = logEvents[beginIndex];
    StringView beginValue = beginEvent->GetContent(mSourceKey);
    char* data = const_cast<char*>(beginValue.data());

    for (size_t index = beginIndex + 1; index <= endIndex; ++index) {
        if (insertLineBreak) {
            data[beginValue.size()] = '\n';
        }
        const auto curLogEvent = logEvents[index];
        StringView curValue = curLogEvent->GetContent(mSourceKey);
        memmove(insertLineBreak ? data + beginValue.size() + 1 : data + beginValue.size(),
                curValue.data(),
                curValue.size());
        beginValue = StringView(
            data, insertLineBreak ? beginValue.size() + 1 + curValue.size() : beginValue.size() + curValue.size());
    }
    beginEvent->SetContentNoCopy(mSourceKey, beginValue);
    if (updateLast) {
        logEventIndex[logEventIndex.size() - 1] = beginIndex;
    } else {
        logEventIndex.emplace_back(beginIndex);
    }
}

void ProcessorMergeMultilineLogNative::HandleUnmatchLogs(const logtail::EventsContainer& events,
                                                         size_t& multiBeginIndex,
                                                         size_t endIndex,
                                                         std::vector<size_t>& logEventIndex,
                                                         std::vector<size_t>& discardLogEventIndex,
                                                         bool mustHandleLogs) {
    // Cannot determine where log is unmatched here where there is only EndPatternReg
    if (!mustHandleLogs && mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
        && mMultiline.GetEndPatternReg() != nullptr) {
        return;
    }
    if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
        for (size_t i = multiBeginIndex; i <= endIndex; i++) {
            discardLogEventIndex.emplace_back(i);
        }
    } else if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
        for (size_t i = multiBeginIndex; i <= endIndex; i++) {
            logEventIndex.emplace_back(i);
        }
    }
    multiBeginIndex = endIndex + 1;
}

} // namespace logtail
