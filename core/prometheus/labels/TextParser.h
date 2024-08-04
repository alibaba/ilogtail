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

#include <sstream>
#include <string>

#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"

namespace logtail {

enum class TextState {
    Start,
    MetricName,
    OpenBrace,
    LabelName,
    EqualSign,
    LabelValue,
    CommaOrCloseBrace,
    SampleValue,
    Timestamp,
    Done,
    Error
};

enum class TextEvent { None, Character, Digit, Equal, Quote, Comma, OpenBrace, CloseBrace, Space, EndOfInput, Invalid };

TextEvent ClassifyChar(char c);

class TextParser {
public:
    TextParser() = default;

    PipelineEventGroup Parse(const std::string& content, uint64_t defaultNanoTs);

    bool ParseLine(const std::string& line, uint64_t defaultNanoTs, MetricEvent& metricEvent);

private:
    void NextState(TextState newState) { mState = newState; }
    void HandleError(const std::string& errMsg);

    void HandleStart(char c, MetricEvent& metricEvent);
    void HandleMetricName(char c, MetricEvent& metricEvent);
    void HandleOpenBrace(char c, MetricEvent& metricEvent);
    void HandleLabelName(char c, MetricEvent& metricEvent);
    void HandleEqualSign(char c, MetricEvent& metricEvent);
    void HandleLabelValue(char c, MetricEvent& metricEvent);
    void HandleCommaOrCloseBrace(char c, MetricEvent& metricEvent);
    void HandleSampleValue(char c, MetricEvent& metricEvent);
    void HandleTimestamp(char c, MetricEvent& metricEvent);
    void HandleSpace(char c, MetricEvent& metricEvent);

    void SkipSpaceIfHasNext();

    TextState mState{TextState::Start};
    std::string mLine;
    std::size_t mPos{0};

    std::string mMetricName;
    std::string mLabelName;
    std::string mLabelValue;
    double mSampleValue{0.0};
    uint64_t mNanoTimestamp{0};
    std::ostringstream mToken;


#ifdef APSARA_UNIT_TEST_MAIN
    friend class TextParserUnittest;
#endif
};

} // namespace logtail
