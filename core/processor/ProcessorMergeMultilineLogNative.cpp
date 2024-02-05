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

    if (!mMultiline.Init(config, *mContext, sName)) {
        return false;
    }

    mFeedLines = &(GetContext().GetProcessProfile().feedLines);
    mSplitLines = &(GetContext().GetProcessProfile().splitLines);

    return true;
}

void ProcessorMergeMultilineLogNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty() || !mMultiline.IsMultiline()) {
        return;
    }
    EventsContainer newEvents;
    const StringView& logPath = logGroup.GetMetadata(EventGroupMetaKey::LOG_FILE_PATH_RESOLVED);

    // 遍历event，对每个event进行合并
    ProcessEvents(logGroup, logPath, newEvents);

    *mSplitLines = newEvents.size();
    logGroup.SwapEvents(newEvents);

    return;
}

bool ProcessorMergeMultilineLogNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

void ProcessorMergeMultilineLogNative::ProcessEvents(PipelineEventGroup& logGroup,
                                                     const StringView& logPath,
                                                     EventsContainer& newEvents) {
    std::vector<PipelineEventPtr> logEventIndex;
    std::vector<PipelineEventPtr> discardLogEventIndex;


    bool splitSuccess = LogSplit(logGroup, logEventIndex, discardLogEventIndex, logPath);

    if (AppConfig::GetInstance()->IsLogParseAlarmValid() && LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
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
        for (const PipelineEventPtr& discardData : discardLogEventIndex) { // warning if data loss
            const LogEvent& logEvent = discardData.Cast<LogEvent>();
            auto& sourceVal = logEvent.GetContent(mSourceKey);

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


    if (logEventIndex.size() == 0) {
        return;
    }

    StringBuffer splitKey = logGroup.GetSourceBuffer()->CopyString(mSourceKey);
    for (const PipelineEventPtr& e : logEventIndex) {
        const LogEvent& logEvent = e.Cast<LogEvent>();

        std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(logGroup.GetSourceBuffer());
        targetEvent->SetTimestamp(logEvent.GetTimestamp(), logEvent.GetTimestampNanosecond());
        targetEvent->SetContentNoCopy(StringView(splitKey.data, splitKey.size), logEvent.GetContent(mSourceKey));
        if (logEvent.GetContents().size() > 1) {
            for (auto& kv : logEvent.GetContents()) {
                if (kv.first != mSourceKey) {
                    targetEvent->SetContentNoCopy(kv.first, kv.second);
                }
            }
        }
        newEvents.emplace_back(std::move(targetEvent));
    }
}

bool ProcessorMergeMultilineLogNative::LogSplit(PipelineEventGroup& logGroup,
                                                std::vector<PipelineEventPtr>& logEventIndex,
                                                std::vector<PipelineEventPtr>& discardLogEventIndex,
                                                const StringView& logPath) {
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
    long unsigned int multiBeginIndex = 0;
    bool splitSuccess = false;
    std::string exception;
    SplitState state = SPLIT_UNMATCH;

    // 遍历event，对每个event进行合并
    auto& events = logGroup.MutableEvents();
    for (long unsigned int curIndex = 0; curIndex < events.size(); ++curIndex) {
        const PipelineEventPtr& e = events[curIndex];
        const LogEvent& sourceEvent = e.Cast<LogEvent>();
        exception.clear();
        StringView sourceVal = sourceEvent.GetContent(mSourceKey);

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
                            // 把multiBeginIndex到 curIndex-1 的日志合并到multiBeginIndex中
                            MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex, true);

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
                    // 把multiBeginIndex到i的日志合并到multiBeginIndex中
                    MergeEvents(events, multiBeginIndex, curIndex, logEventIndex);
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
                // 把multiBeginIndex到i的日志合并到multiBeginIndex中
                if (mMultiline.GetEndPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetEndPatternReg(), exception)) {
                        MergeEvents(events, multiBeginIndex, curIndex, logEventIndex);
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
                    // 把multiBeginIndex到i-1的日志合并到multiBeginIndex中
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        if (multiBeginIndex != curIndex) {
                            MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex);
                            multiBeginIndex = curIndex;
                        }
                    } else if (mMultiline.GetContinuePatternReg() != nullptr) {
                        // case: begin+continue, but we meet unmatch log here
                        // 当前行ContinuePatternReg和EndPatternReg都匹配不上
                        // 把multiBeginIndex到i-1的日志合并到multiBeginIndex中
                        MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex);
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
                        // 把multiBeginIndex到i的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(events, multiBeginIndex, curIndex, logEventIndex);
                        multiBeginIndex = curIndex + 1;
                        state = SPLIT_UNMATCH;
                    } else {
                        // 当前行不是END, multiBeginIndex 到 i 为 无效多行日志
                        HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
                }

                if (mMultiline.GetStartPatternReg() != nullptr) {
                    if (BoostRegexMatch(
                            sourceVal.data(), sourceVal.size(), *mMultiline.GetStartPatternReg(), exception)) {
                        // 当前行为START
                        // 把multiBeginIndex到i-1的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex);
                        multiBeginIndex = curIndex;
                        state = SPLIT_BEGIN;
                    } else {
                        // 当前行为START
                        // 把multiBeginIndex到i-1的日志合并到multiBeginIndex中
                        splitSuccess = true;
                        MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex);
                        multiBeginIndex = curIndex;
                        HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                        state = SPLIT_UNMATCH;
                    }
                    break;
                }

                // EndPatternReg不存在，StartPattern不存在，ContinuePatternReg没匹配上
                // 把multiBeginIndex到i-1的日志合并到multiBeginIndex中
                splitSuccess = true;
                MergeEvents(events, multiBeginIndex, curIndex - 1, logEventIndex);
                multiBeginIndex = curIndex;
                HandleUnmatchLogs(events, multiBeginIndex, curIndex, logEventIndex, discardLogEventIndex);
                state = SPLIT_UNMATCH;
                break;
        }
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

    return splitSuccess;
}

void ProcessorMergeMultilineLogNative::MergeEvents(logtail::EventsContainer& events,
                                                   long unsigned int beginIndex,
                                                   long unsigned int endIndex,
                                                   std::vector<PipelineEventPtr>& logEventIndex,
                                                   bool update) {
    // 把 beginIndex 到 endIndex 的日志合并到 beginIndex 中
    auto& beginEvent = events[beginIndex].Cast<LogEvent>();
    StringView beginValue = beginEvent.GetContent(mSourceKey);

    for (long unsigned int index = beginIndex + 1; index <= endIndex; ++index) {
        const char* data = beginValue.data();
        const_cast<char*>(data)[beginValue.size()] = '\n';
        beginValue = StringView(data, beginValue.size() + 1);

        const auto& curLogEvent = events[index].Cast<LogEvent>();
        StringView curValue = curLogEvent.GetContent(mSourceKey);
        memcpy(const_cast<char*>(beginValue.data() + beginValue.size()), curValue.data(), curValue.size());
        beginValue = StringView(beginValue.data(), beginValue.size() + curValue.size());
    }
    beginEvent.SetContentNoCopy(mSourceKey, beginValue);
    if (update) {
        logEventIndex[logEventIndex.size()-1] = events[beginIndex];
    } else {
        logEventIndex.emplace_back(events[beginIndex]);
    }
}

void ProcessorMergeMultilineLogNative::HandleUnmatchLogs(const logtail::EventsContainer& events,
                                                         long unsigned int& multiBeginIndex,
                                                         long unsigned int endIndex,
                                                         std::vector<PipelineEventPtr>& logEventIndex,
                                                         std::vector<PipelineEventPtr>& discardLogEventIndex) {
    // Cannot determine where log is unmatched here where there is only EndPatternReg
    if (mMultiline.GetStartPatternReg() == nullptr && mMultiline.GetContinuePatternReg() == nullptr
        && mMultiline.GetEndPatternReg() != nullptr) {
        return;
    }
    if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::DISCARD) {
        for (long unsigned int i = multiBeginIndex; i <= endIndex; i++) {
            discardLogEventIndex.emplace_back(events[i]);
        }
    } else if (mMultiline.mUnmatchedContentTreatment == MultilineOptions::UnmatchedContentTreatment::SINGLE_LINE) {
        for (long unsigned int i = multiBeginIndex; i <= endIndex; i++) {
            logEventIndex.emplace_back(events[i]);
        }
    }
    multiBeginIndex = endIndex + 1;
}

} // namespace logtail
