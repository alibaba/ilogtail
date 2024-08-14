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

#include "prometheus/labels/TextParser.h"

#include <boost/algorithm/string.hpp>
#include <charconv>
#include <cmath>
#include <string>

#include "common/StringTools.h"
#include "logger/Logger.h"
#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"
#include "models/StringView.h"
#include "prometheus/Constants.h"
#include "prometheus/Utils.h"

using namespace std;

namespace logtail {

PipelineEventGroup TextParser::Parse(const string& content, uint64_t defaultNanoTs) {
    auto eGroup = PipelineEventGroup(make_shared<SourceBuffer>());
    vector<StringView> lines;
    lines.reserve(content.size() / 100);
    SplitStringView(content, '\n', lines);
    for (const auto& line : lines) {
        if (!IsValidMetric(line)) {
            continue;
        }
        auto metricEvent = eGroup.CreateMetricEvent();
        if (ParseLine(line, defaultNanoTs, *metricEvent)) {
            eGroup.MutableEvents().emplace_back(std::move(metricEvent));
        }
    }

    return eGroup;
}

PipelineEventGroup TextParser::BuildLogGroup(const string& content, uint64_t defaultNanoTs) {
    PipelineEventGroup eGroup(std::make_shared<SourceBuffer>());

    for (const auto& line : SplitString(content, "\n")) {
        if (!IsValidMetric(line)) {
            continue;
        }
        auto* logEvent = eGroup.AddLogEvent();
        logEvent->SetContent(prometheus::PROMETHEUS, line);
        logEvent->SetTimestamp(defaultNanoTs / 1000000000, defaultNanoTs % 1000000000);
    }

    return eGroup;
}

bool TextParser::ParseLine(StringView line, uint64_t defaultNanoTs, MetricEvent& metricEvent) {
    mLine = line;
    mPos = 0;
    mState = TextState::Start;
    mLabelName.clear();
    mTokenLength = 0;
    mNoEscapes = line.find('\\') == StringView::npos;
    if (defaultNanoTs > 0) {
        mTimestamp = defaultNanoTs / 1000000000;
        mNanoTimestamp = defaultNanoTs % 1000000000;
    }

    HandleStart(metricEvent);

    if (mState == TextState::Done) {
        return true;
    }

    return false;
}

void TextParser::HandleStart(MetricEvent& metricEvent) {
    SkipLeadingWhitespace();
    auto c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    if (std::isalpha(c) || c == '_' || c == ':') {
        HandleMetricName(metricEvent);
    } else {
        HandleError("expected metric name");
    }
}

void TextParser::HandleMetricName(MetricEvent& metricEvent) {
    char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    while (std::isalpha(c) || c == '_' || c == ':' || std::isdigit(c)) {
        ++mTokenLength;
        ++mPos;
        c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    }
    metricEvent.SetNameNoCopy(mLine.substr(mPos - mTokenLength, mTokenLength));
    mTokenLength = 0;
    SkipLeadingWhitespace();
    if (mPos < mLine.size()) {
        if (mLine[mPos] == '{') {
            ++mPos;
            SkipLeadingWhitespace();
            HandleLabelName(metricEvent);
        } else {
            HandleSampleValue(metricEvent);
        }
    } else {
        HandleError("error end of metric name");
    }
}

void TextParser::HandleLabelName(MetricEvent& metricEvent) {
    char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    if (std::isalpha(c) || c == '_') {
        while (std::isalpha(c) || c == '_' || c == ':' || std::isdigit(c)) {
            ++mTokenLength;
            ++mPos;
            c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
        }
        mLabelName = mLine.substr(mPos - mTokenLength, mTokenLength);
        mTokenLength = 0;
        SkipLeadingWhitespace();
        if (mPos == mLine.size() || mLine[mPos] != '=') {
            HandleError("expected '=' after label name");
            return;
        }
        ++mPos;
        SkipLeadingWhitespace();
        HandleEqualSign(metricEvent);
    } else if (c == '}') {
        ++mPos;
        SkipLeadingWhitespace();
        HandleSampleValue(metricEvent);
    } else {
        HandleError("invalid character in label name");
    }
}

void TextParser::HandleEqualSign(MetricEvent& metricEvent) {
    if (mPos < mLine.size() && mLine[mPos] == '"') {
        ++mPos;
        HandleLabelValue(metricEvent);
    } else {
        HandleError("expected '\"' after '='");
    }
}

void TextParser::HandleLabelValue(MetricEvent& metricEvent) {
    // left quote has been consumed
    if (mNoEscapes) {
        // Fast path - the line has no escape chars
        while (mPos < mLine.size() && mLine[mPos] != '"') {
            ++mPos;
            ++mTokenLength;
        }
        if (mPos == mLine.size()) {
            HandleError("unexpected end of input in label value");
        }
        metricEvent.SetTagNoCopy(mLabelName, mLine.substr(mPos - mTokenLength, mTokenLength));

    } else {
        // Slow path - the line contains escape chars
        auto lPos = mPos;
        while (mPos < mLine.size()) {
            if (mLine[mPos] == '"') {
                int lPos = mPos - 1;
                while (lPos >= 0 && mLine[lPos] == '\\') {
                    --lPos;
                }
                if ((mPos - lPos) % 2 == 1) {
                    break;
                }
            }
            ++mPos;
        }
        if (mPos == mLine.size()) {
            HandleError("unexpected end of input in label value");
        } else {
            string labelValue;
            while (lPos < mPos) {
                if (mLine[lPos] == '\\') {
                    if (lPos + 1 < mPos) {
                        switch (mLine[lPos + 1]) {
                            case '\\':
                            case '\"':
                            case 'n':
                                labelValue.push_back(mLine[lPos + 1]);
                                break;
                            default:
                                labelValue.push_back('\\');
                                labelValue.push_back(mLine[lPos + 1]);
                                break;
                        }
                        lPos += 2;
                    } else {
                        labelValue.push_back('\\');
                        lPos += 1;
                    }
                } else {
                    labelValue.push_back(mLine[lPos]);
                    lPos += 1;
                }
            }
            metricEvent.SetTag(mLabelName.to_string(), labelValue);
        }
    }
    mTokenLength = 0;
    ++mPos;
    SkipLeadingWhitespace();
    if (mPos < mLine.size() && (mLine[mPos] == ',' || mLine[mPos] == '}')) {
        HandleCommaOrCloseBrace(metricEvent);
    } else {
        HandleError("unexpected end of input in label value");
    }
}

void TextParser::HandleCommaOrCloseBrace(MetricEvent& metricEvent) {
    char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    if (c == ',') {
        ++mPos;
        SkipLeadingWhitespace();
        HandleLabelName(metricEvent);
    } else if (c == '}') {
        ++mPos;
        SkipLeadingWhitespace();
        HandleSampleValue(metricEvent);
    } else {
        HandleError("expected ',' or '}' after label value");
    }
}

void TextParser::HandleSampleValue(MetricEvent& metricEvent) {
    while (mPos < mLine.size() && !(mLine[mPos] == ' ' || mLine[mPos] == '\t')) {
        ++mPos;
        ++mTokenLength;
    }

    auto tmpSampleValue = mLine.substr(mPos - mTokenLength, mTokenLength);

    if (!StringViewToDouble(tmpSampleValue, mSampleValue)) {
        HandleError("invalid sample value");
        mTokenLength = 0;
        return;
    }
    metricEvent.SetValue<UntypedSingleValue>(mSampleValue);
    mTokenLength = 0;
    SkipLeadingWhitespace();
    if (mPos == mLine.size()) {
        metricEvent.SetTimestamp(mTimestamp, mNanoTimestamp);
        mState = TextState::Done;
    } else {
        HandleTimestamp(metricEvent);
    }
}

void TextParser::HandleTimestamp(MetricEvent& metricEvent) {
    // '#' is for exemplars
    while (mPos < mLine.size() && mLine[mPos] != ' ' && mLine[mPos] != '\t' && mLine[mPos] != '#') {
        ++mPos;
        ++mTokenLength;
    }
    auto tmpTimestamp = mLine.substr(mPos - mTokenLength, mTokenLength);
    if (tmpTimestamp.size() == 0) {
        mState = TextState::Done;
        return;
    }
    uint64_t nanoTimestamp = 0;
    auto [ptr, ec] = std::from_chars(tmpTimestamp.data(), tmpTimestamp.data() + tmpTimestamp.size(), nanoTimestamp);
    if (ec != std::errc()) {
        HandleError("invalid timestamp");
        mTokenLength = 0;
        return;
    }
    time_t timestamp = nanoTimestamp / 1000;
    auto ns = (nanoTimestamp % 1000) * 1000000;
    metricEvent.SetTimestamp(timestamp, ns);
    mTokenLength = 0;

    mState = TextState::Done;
}

void TextParser::HandleError(const string& errMsg) {
    cout << errMsg << mLine << endl;
    LOG_WARNING(sLogger, ("text parser error parsing line", mLine.to_string() + errMsg));
    mState = TextState::Error;
}

inline void TextParser::SkipLeadingWhitespace() {
    while (mPos < mLine.length() && (mLine[mPos] == ' ' || mLine[mPos] == '\t')) {
        mPos++;
    }
}

} // namespace logtail
