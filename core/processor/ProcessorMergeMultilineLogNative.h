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
    enum class MergeType { BY_REGEX, BY_FLAG, BY_JSON };

    static const std::string PartLogFlag;
    static const std::string sName;

    std::string mSourceKey = DEFAULT_CONTENT_KEY;
    MergeType mMergeType = MergeType::BY_REGEX;
    MultilineOptions mMultiline;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    void MergeLogsByRegex(PipelineEventGroup& logGroup);
    void MergeLogsByFlag(PipelineEventGroup& logGroup);
    void MergeLogsByJSON(PipelineEventGroup& logGroup);

    bool LogSplit(PipelineEventGroup& logGroup,
                  std::vector<LogEvent*>& logEvents,
                  std::vector<size_t>& logEventIndex,
                  std::vector<size_t>& discardLogEventIndex);
    void HandleUnmatchLogs(const logtail::EventsContainer& events,
                           size_t& multiBeginIndex,
                           size_t endIndex,
                           std::vector<size_t>& logEventIndex,
                           std::vector<size_t>& discardLogEventIndex,
                           bool mustHandleLogs = false);
    void MergeEvents(std::vector<LogEvent*> &logEvents,
                     size_t beginIndex,
                     size_t endIndex,
                     std::vector<size_t>& logEventIndex,
                     bool update = false,
                     bool insertLineBreak = true);

    int* mFeedLines = nullptr;
    int* mSplitLines = nullptr;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorMergeMultilineLogNativeUnittest;
    friend class ProcessorMergeMultilineLogDisacardUnmatchUnittest;
    friend class ProcessorMergeMultilineLogKeepUnmatchUnittest;
#endif
};

} // namespace logtail
