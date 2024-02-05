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

#pragma once

#include <cstdint>
#include <vector>

#include "common/Constants.h"
#include "file_server/MultilineOptions.h"
#include "plugin/interface/Processor.h"
#include "processor/CommonParserOptions.h"

namespace logtail {

class ProcessorMergeMultilineLogNative : public Processor {
public:
    static const std::string sName;

    std::string mSourceKey = DEFAULT_CONTENT_KEY;
    MultilineOptions mMultiline;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    void ProcessEvents(PipelineEventGroup& logGroup, const StringView& logPath, EventsContainer& newEvents);
    bool LogSplit(PipelineEventGroup& logGroup,
                  std::vector<PipelineEventPtr>& logEventIndex,
                  std::vector<PipelineEventPtr>& discardLogEventIndex,
                  const StringView& logPath);
    void HandleUnmatchLogs(const logtail::EventsContainer& events,
                           long unsigned int& multiBeginIndex,
                           long unsigned int endIndex,
                           std::vector<PipelineEventPtr>& logEventIndex,
                           std::vector<PipelineEventPtr>& discardLogEventIndex,
                           bool mustHandleLogs = false);
    void MergeEvents(logtail::EventsContainer& events,
                     long unsigned int beginIndex,
                     long unsigned int endIndex,
                     std::vector<PipelineEventPtr>& logEventIndex,
                     bool update = false);

    int* mFeedLines = nullptr;
    int* mSplitLines = nullptr;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorSplitRegexNativeUnittest;
    friend class ProcessorSplitRegexDisacardUnmatchUnittest;
    friend class ProcessorSplitRegexKeepUnmatchUnittest;
#endif
};

} // namespace logtail
