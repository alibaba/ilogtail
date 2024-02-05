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

#include "processor/ProcessorSplitNative.h"

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

const std::string ProcessorSplitNative::sName = "processor_split_regex_native";

bool ProcessorSplitNative::Init(const Json::Value& config) {
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

    return true;
}

void ProcessorSplitNative::Process(PipelineEventGroup& logGroup) {
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

bool ProcessorSplitNative::IsSupportedEvent(const PipelineEventPtr& e) const {
    return e.Is<LogEvent>();
}

void ProcessorSplitNative::ProcessEvent(PipelineEventGroup& logGroup,
                                        const StringView& logPath,
                                        const PipelineEventPtr& e,
                                        EventsContainer& newEvents) {
    if (!IsSupportedEvent(e)) {
        newEvents.emplace_back(e);
        return;
    }
    const LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        newEvents.emplace_back(e);
        return;
    }
    StringView sourceVal = sourceEvent.GetContent(mSourceKey);
    std::vector<StringView> logIndex; // all splitted logs
    int feedLines = 0;
    LogSplit(sourceVal.data(), sourceVal.size(), feedLines, logIndex, logPath);
    *mFeedLines += feedLines;

    if (logIndex.size() == 0) {
        return;
    }
    long sourceoffset = 0L;
    if (sourceEvent.HasContent(LOG_RESERVED_KEY_FILE_OFFSET)) {
        sourceoffset = atol(sourceEvent.GetContent(LOG_RESERVED_KEY_FILE_OFFSET).data()); // use safer method
    }
    StringBuffer splitKey = logGroup.GetSourceBuffer()->CopyString(mSourceKey);
    for (auto& content : logIndex) {
        std::unique_ptr<LogEvent> targetEvent = LogEvent::CreateEvent(logGroup.GetSourceBuffer());
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

void ProcessorSplitNative::LogSplit(const char* buffer,
                                    int32_t size,
                                    int32_t& lineFeed,
                                    std::vector<StringView>& logIndex,
                                    const StringView& logPath) {
    int multiBeginIndex = 0;
    int begIndex = 0;
    int endIndex = 0;
    while (endIndex <= size) {
        if (endIndex == size || buffer[endIndex] == '\n') {
            lineFeed++;
            // Single line log
            logIndex.emplace_back(buffer + begIndex, endIndex - begIndex);
            multiBeginIndex = endIndex + 1;
            begIndex = endIndex + 1;
        }
        endIndex++;
    }
    // We should clear the log from `multiBeginIndex` to `size`.
    if (multiBeginIndex < size) {
        logIndex.emplace_back(buffer + multiBeginIndex, size - multiBeginIndex);
    }
}

} // namespace logtail
