/*
 * Copyright 2024 iLogtail Authors
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

#include <string>

#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"

namespace logtail {

enum class TextState { Start, Done, Error };

class TextParser {
public:
    TextParser() = default;

    PipelineEventGroup Parse(const std::string& content, uint64_t defaultNanoTs);
    PipelineEventGroup BuildLogGroup(const std::string& content, uint64_t defaultNanoTs);

    bool ParseLine(StringView line, uint64_t defaultNanoTs, MetricEvent& metricEvent);

private:
    void HandleError(const std::string& errMsg);

    void HandleStart(MetricEvent& metricEvent);
    void HandleMetricName(MetricEvent& metricEvent);
    void HandleOpenBrace(MetricEvent& metricEvent);
    void HandleLabelName(MetricEvent& metricEvent);
    void HandleEqualSign(MetricEvent& metricEvent);
    void HandleLabelValue(MetricEvent& metricEvent);
    void HandleCommaOrCloseBrace(MetricEvent& metricEvent);
    void HandleSampleValue(MetricEvent& metricEvent);
    void HandleTimestamp(MetricEvent& metricEvent);
    void HandleSpace(MetricEvent& metricEvent);

    void SkipSpaceIfHasNext();
    void SkipLeadingWhitespace();

    TextState mState{TextState::Start};
    StringView mLine;
    std::size_t mPos{0};

    StringView mLabelName;
    double mSampleValue{0.0};
    uint64_t mNanoTimestamp{0};
    std::size_t mTokenLength{0};

    bool mNoEscapes = true;


#ifdef APSARA_UNIT_TEST_MAIN
    friend class TextParserUnittest;
#endif
};

} // namespace logtail
