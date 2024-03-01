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
#include "common/ParamExtractor.h"
#include "logger/Logger.h"
#include "models/LogEvent.h"
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

    std::string mergeType;
    if (!GetMandatoryStringParam(config, "MergeType", mergeType, errorMsg)) {
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
    if (e.Is<LogEvent>()) {
        return true;
    }
    LOG_ERROR(GetContext().GetLogger(),
              ("current event not support",
               "")("project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName()));
    return false;
}

void ProcessorMergeMultilineLogNative::MergeLogsByFlag(PipelineEventGroup& logGroup) {
    auto& sourceEvents = logGroup.MutableEvents();
    size_t size = 0;
    std::vector<LogEvent*> events;
    bool isPartialLog = false;
    size_t begin = 0;
    for (size_t cur = 0; cur < sourceEvents.size(); ++cur) {
        if (!IsSupportedEvent(sourceEvents[cur])) {
            for (size_t i = begin; i < sourceEvents.size(); ++i) {
                sourceEvents[size++] = std::move(sourceEvents[i]);
            }
            sourceEvents.resize(size);
            return;
        }
        LogEvent* sourceEvent = &sourceEvents[cur].Cast<LogEvent>();
        if (sourceEvent->GetContents().empty()) {
            continue;
        }
        events.emplace_back(sourceEvent);
        if (isPartialLog) {
            // case: p p p ... p(last) notP(cur)
            if (!sourceEvent->HasContent(PartLogFlag)) {
                MergeEvents(events, false);
                sourceEvents[size++] = std::move(sourceEvents[begin]);
                begin = cur + 1;
                isPartialLog = false;
            }
        } else {
            if (sourceEvent->HasContent(PartLogFlag)) {
                auto& contents = sourceEvent->MutableContents();
                contents.erase(PartLogFlag);
                isPartialLog = true;
            } else {
                MergeEvents(events, false);
                sourceEvents[size++] = std::move(sourceEvents[begin]);
                begin = cur + 1;
            }
        }
    }
    if (isPartialLog) {
        MergeEvents(events, false);
        sourceEvents[size++] = std::move(sourceEvents[begin]);
    }
    sourceEvents.resize(size);
}

void ProcessorMergeMultilineLogNative::MergeLogsByRegex(PipelineEventGroup& logGroup) {
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
    std::string exception;
    SplitState state = SPLIT_UNMATCH;
    std::vector<LogEvent*> mergeEvents;
    size_t newEventsSize = 0;
    const StringView logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);

    // 遍历event，对每个event进行合并
    auto& sourceEvents = logGroup.MutableEvents();
    for (size_t curIndex = 0; curIndex < sourceEvents.size(); ++curIndex) {
        if (!IsSupportedEvent(sourceEvents[curIndex])) {
            for (size_t i = multiBeginIndex; i < sourceEvents.size(); ++i) {
                sourceEvents[newEventsSize++] = std::move(sourceEvents[i]);
            }
            sourceEvents.resize(newEventsSize);
            return;
        }
        LogEvent* sourceEvent = &sourceEvents[curIndex].Cast<LogEvent>();
        // 这个evnet没有内容 不需要处理
        if (sourceEvent->GetContents().empty()) {
            continue;
        }
        exception.clear();
        // State machine with three states (SPLIT_UNMATCH, SPLIT_BEGIN, SPLIT_CONTINUE)
        StringView sourceVal = sourceEvent->GetContent(mSourceKey);

        mergeEvents.emplace_back(sourceEvent);
        switch (state) {
            case SPLIT_UNMATCH:
                // case: begin or begin + continue or begin + end
                if (mMultiline.GetStartPatternReg() != nullptr) {
                    // 匹配到了行首
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        state = SPLIT_BEGIN;
                        break;
                    }
                    // 没有匹配到行首
                    multiBeginIndex = curIndex;
                    HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                    mergeEvents.clear();
                    break;
                }
                // case: continue + end
                // ContinuePatternReg可以匹配0次或多次，如果不匹配，继续尝试EndPatternReg
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    // 存在continuePattern，且当前行匹配上，切换到continue状态
                    state = SPLIT_CONTINUE;
                    break;
                }
                // case: end or continue + end
                if (mMultiline.GetEndPatternReg() != nullptr
                    && BoostRegexMatch(sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                    // case: unmatch end
                    // 把 multiBeginIndex 到 curIndex 的日志合并到multiBeginIndex中
                    MergeEvents(mergeEvents, true);
                    sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);
                    multiBeginIndex = curIndex + 1;
                    break;
                }
                // Cannot determine where log is unmatched here where there is only EndPatternReg
                if (mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
                    && mMultiline.GetEndPatternReg() != nullptr) {
                    break;
                }

                // 当前行没有匹配到任何正则
                multiBeginIndex = curIndex;
                HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                mergeEvents.clear();
                break;

            case SPLIT_BEGIN:
                // ContinuePatternReg可以匹配0次或多次，如果没有匹配，继续尝试其他。
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    // case: begin continue
                    state = SPLIT_CONTINUE;
                    break;
                }
                // case: begin end
                // 把multiBeginIndex到curIndex的日志合并到multiBeginIndex中
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        MergeEvents(mergeEvents, true);
                        sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);

                        multiBeginIndex = curIndex + 1;
                        state = SPLIT_UNMATCH;
                    }
                    // for case: begin unmatch ... unmatch end
                    // so logs cannot be handled as unmatch even if not match LogEngReg
                } else if (mMultiline.GetStartPatternReg() != nullptr) {
                    // case: begin or begin continue
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        if (multiBeginIndex != curIndex) {
                            mergeEvents.pop_back();
                            MergeEvents(mergeEvents, true);
                            mergeEvents.emplace_back(sourceEvent);
                            sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);

                            multiBeginIndex = curIndex;
                        }
                    } else if (mMultiline.GetContinuePatternReg() != nullptr) {
                        // case: begin+continue, but we meet unmatch log here
                        mergeEvents.pop_back();
                        MergeEvents(mergeEvents, true);
                        sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);
                        multiBeginIndex = curIndex;
                        HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                        mergeEvents.clear();
                        state = SPLIT_UNMATCH;
                    }
                    // else case: begin+end or begin, we should keep unmatch log in the cache
                }
                break;

            case SPLIT_CONTINUE:
                // ContinuePatternReg可以匹配0次或多次，如果没有匹配，继续尝试其他。
                if (mMultiline.GetContinuePatternReg() != nullptr
                    && BoostRegexMatch(
                        sourceVal.data(), sourceVal.size(), *mMultiline.GetContinuePatternReg(), exception)) {
                    break;
                }
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        // case: continue end
                        // 把multiBeginIndex到curIndex的日志合并到multiBeginIndex中

                        MergeEvents(mergeEvents, true);
                        sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);

                        multiBeginIndex = curIndex + 1;
                        state = SPLIT_UNMATCH;
                    } else {
                        // 当前行不是END, multiBeginIndex 到 curIndex 为 无效多行日志
                        HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                        mergeEvents.clear();
                        state = SPLIT_UNMATCH;
                    }
                } else if (mMultiline.GetStartPatternReg() != nullptr) {
                    // case start continue
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        // 当前行为START
                        // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                        mergeEvents.pop_back();
                        MergeEvents(mergeEvents, true);
                        mergeEvents.emplace_back(sourceEvent);
                        sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);

                        multiBeginIndex = curIndex;
                        state = SPLIT_BEGIN;
                    } else {
                        mergeEvents.pop_back();
                        MergeEvents(mergeEvents, true);
                        sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);
                        multiBeginIndex = curIndex;
                        HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                        mergeEvents.clear();
                        state = SPLIT_UNMATCH;
                    }
                } else {
                    // EndPatternReg不存在，StartPattern不存在，ContinuePatternReg没匹配上(理论上不支持这种正则配置)
                    // 把multiBeginIndex到curIndex-1的日志合并到multiBeginIndex中
                    mergeEvents.pop_back();
                    MergeEvents(mergeEvents, true);
                    sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);
                    multiBeginIndex = curIndex;
                    HandleUnmatchLogs(sourceEvents, multiBeginIndex, curIndex, newEventsSize, logPath);
                    mergeEvents.clear();
                    state = SPLIT_UNMATCH;
                }
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
    if (multiBeginIndex < sourceEvents.size()) {
        if (mMultiline.GetStartPatternReg() != NULL && mMultiline.GetEndPatternReg() == NULL) {
            // If logs is unmatched, they have been handled immediately. So logs must be matched here.
            MergeEvents(mergeEvents, true);
            sourceEvents[newEventsSize++] = std::move(sourceEvents[multiBeginIndex]);
        } else if (mMultiline.GetStartPatternReg() == NULL && mMultiline.GetContinuePatternReg() == NULL
                   && mMultiline.GetEndPatternReg() != NULL) {
            // If there is still logs in cache, it means that there is no end line. We can handle them as unmatched.
            HandleUnmatchLogs(sourceEvents, multiBeginIndex, sourceEvents.size() - 1, newEventsSize, logPath, true);
            mergeEvents.clear();
        } else {
            HandleUnmatchLogs(sourceEvents, multiBeginIndex, sourceEvents.size() - 1, newEventsSize, logPath, true);
            mergeEvents.clear();
        }
    }
    sourceEvents.resize(newEventsSize);
}

void ProcessorMergeMultilineLogNative::MergeEvents(std::vector<LogEvent*>& logEvents, bool insertLineBreak) {
    if (logEvents.size() == 0) {
        return;
    }
    if (logEvents.size() == 1) {
        logEvents.clear();
        return;
    }
    LogEvent* targetEvent = logEvents[0];
    StringView targetValue = targetEvent->GetContent(mSourceKey);
    char* begin = const_cast<char*>(targetValue.data());
    char* end = begin + targetValue.size();

    for (size_t index = 1; index < logEvents.size(); ++index) {
        if (insertLineBreak) {
            *end = '\n';
            ++end;
        }
        StringView curValue = logEvents[index]->GetContent(mSourceKey);
        memmove(end, curValue.data(), curValue.size());
        end += curValue.size();
    }
    targetEvent->SetContentNoCopy(mSourceKey, StringView(begin, end - begin));
    logEvents.clear();
}

void ProcessorMergeMultilineLogNative::HandleUnmatchLogs(std::vector<PipelineEventPtr>& logEvents,
                                                         size_t& multiBeginIndex,
                                                         size_t endIndex,
                                                         size_t& newEventsSize,
                                                         const StringView logPath,
                                                         bool mustHandleLogs) {
    // Cannot determine where log is unmatched here where there is only EndPatternReg
    if (!mustHandleLogs && mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
        && mMultiline.GetEndPatternReg() != nullptr) {
        return;
    }
    if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
        for (size_t i = multiBeginIndex; i <= endIndex; i++ && AppConfig::GetInstance()->IsLogParseAlarmValid()
             && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
            StringView sourceVal = logEvents[i].Cast<LogEvent>().GetContent(mSourceKey);
            GetContext().GetAlarm().SendAlarm(SPLIT_LOG_FAIL_ALARM,
                                              "split log lines discard data, file:" + logPath.to_string()
                                                  + ", logs:" + sourceVal.substr(0, 1024).to_string(),
                                              GetContext().GetProjectName(),
                                              GetContext().GetLogstoreName(),
                                              GetContext().GetRegion());
            LOG_WARNING(
                GetContext().GetLogger(),
                ("split log lines discard data", "please check log_begin_regex")("file_name", logPath.to_string())(
                    "log bytes", sourceVal.size() + 1)("first 1KB log", sourceVal.substr(0, 1024).to_string()));
        }
    } else if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
        for (size_t i = multiBeginIndex; i <= endIndex; i++) {
            logEvents[newEventsSize++] = std::move(logEvents[i]);
        }
    }
    multiBeginIndex = endIndex + 1;
}

} // namespace logtail
