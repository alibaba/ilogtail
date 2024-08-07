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

#include "Constants.h"
#include "common/StringTools.h"
#include "logger/Logger.h"
#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"

using namespace std;

namespace logtail {

inline bool StringViewToDouble(const StringView& sv, double& value) {
    const char* str = sv.data();
    char* end = nullptr;
    errno = 0;
    value = std::strtod(str, &end);
    if (end == str) {
        return false;
    }
    if (errno == ERANGE) {
        return false;
    }
    return true;
}

inline bool IsWhitespace(char c) {
    // Prometheus treats ' ' and '\t' as whitespace
    // according to
    // https://github.com/prometheus/docs/blob/master/content/docs/instrumenting/exposition_formats.md#text-format-details
    return c == ' ' || c == '\t';
}

bool IsValidMetric(const StringView& line) {
    for (auto c : line) {
        if (IsWhitespace(c)) {
            continue;
        }
        if (c == '#') {
            return false;
        }
        return true;
    }
    return false;
}

std::vector<StringView> SplitStringView(const std::string& s, char delimiter) {
    std::vector<StringView> result;

    size_t start = 0;
    size_t end = 0;

    while ((end = s.find(delimiter, start)) != std::string::npos) {
        result.emplace_back(s.data() + start, end - start);
        start = end + 1;
    }
    if (start < s.size()) {
        result.emplace_back(s.data() + start, s.size() - start);
    }

    return result;
}


PipelineEventGroup TextParser::Parse(const string& content, uint64_t defaultNanoTs) {
    auto eGroup = PipelineEventGroup(make_shared<SourceBuffer>());

    for (const auto& line : SplitStringView(content, '\n')) {
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
        mNanoTimestamp = defaultNanoTs;
    }

    while (mState != TextState::Done && mState != TextState::Error) {
        char currentChar = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
        switch (mState) {
            case TextState::Start:
                HandleStart(currentChar, metricEvent);
                break;
            case TextState::MetricName:
                HandleMetricName(currentChar, metricEvent);
                break;
            case TextState::OpenBrace:
                HandleOpenBrace(currentChar, metricEvent);
                break;
            case TextState::LabelName:
                HandleLabelName(currentChar, metricEvent);
                break;
            case TextState::EqualSign:
                HandleEqualSign(currentChar, metricEvent);
                break;
            case TextState::LabelValue:
                HandleLabelValue(currentChar, metricEvent);
                break;
            case TextState::CommaOrCloseBrace:
                HandleCommaOrCloseBrace(currentChar, metricEvent);
                break;
            case TextState::SampleValue:
                HandleSampleValue(currentChar, metricEvent);
                break;
            case TextState::Timestamp:
                HandleTimestamp(currentChar, metricEvent);
                break;
            default:
                HandleError("unknown state");
                break;
        }
    }

    if (mState == TextState::Done) {
        return true;
    }

    return false;
}

void TextParser::HandleStart(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (IsWhitespace(c)) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    if (std::isalpha(c) || c == '-' || c == '_' || c == ':') {
        ++mTokenLength;
        NextState(TextState::MetricName);
    } else {
        HandleError("expected metric name");
    }
}

void TextParser::HandleMetricName(char c, MetricEvent& metricEvent) {
    if (std::isalpha(c) || c == '-' || c == '_' || c == ':' || std::isdigit(c)) {
        ++mTokenLength;
    } else if (c == '{' || IsWhitespace(c)) {
        metricEvent.SetName(mLine.substr(mPos - mTokenLength - 1, mTokenLength).to_string());
        mTokenLength = 0;
        // need to solve these case, but don't point mPos to Value
        // {Space}* OpenBrace
        // {Space}+ Value
        while (IsWhitespace(c) && mPos < mLine.size() && std::isspace(mLine[mPos])) {
            c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
        }
        if (IsWhitespace(c)) {
            // Space OpenBrace
            if (mLine[mPos] == '{') {
                mPos++;
                NextState(TextState::OpenBrace);
            } else {
                // Space Value
                NextState(TextState::SampleValue);
            }
        } else if (c == '{') {
            // OpenBrace
            NextState(TextState::OpenBrace);
        } else {
            // Value
            HandleError("invalid character in metric name");
        }

    } else {
        HandleError("invalid character in metric name");
    }
}

void TextParser::HandleOpenBrace(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (IsWhitespace(c)) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    if (std::isalpha(c) || c == '-' || c == '_' || c == ':') {
        ++mTokenLength;
        NextState(TextState::LabelName);
    } else if (c == '}') {
        SkipSpaceIfHasNext();
        NextState(TextState::SampleValue);
    } else {
        HandleError("expected label name after '{'");
    }
}

void TextParser::HandleLabelName(char c, MetricEvent&) {
    if (std::isalpha(c) || c == '-' || c == '_' || c == ':' || std::isdigit(c)) {
        ++mTokenLength;
    } else if (c == '=' || IsWhitespace(c)) {
        mLabelName = mLine.substr(mPos - mTokenLength - 1, mTokenLength);
        mTokenLength = 0;
        // Ignore subsequent spaces
        while (IsWhitespace(c)) {
            c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
        }
        if (c != '=') {
            HandleError("expected '=' after label name");
            return;
        }
        NextState(TextState::EqualSign);
    } else {
        HandleError("invalid character in label name");
    }
}

void TextParser::HandleEqualSign(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (IsWhitespace(c)) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    if (c == '"') {
        NextState(TextState::LabelValue);
    } else {
        HandleError("expected '\"' after '='");
    }
}

void TextParser::HandleLabelValue(char c, MetricEvent& metricEvent) {
    if (c == '"') {
        metricEvent.SetTag(mLabelName, mLine.substr(mPos - mTokenLength - 1, mTokenLength));
        mTokenLength = 0;
        NextState(TextState::CommaOrCloseBrace);
    } else if (c == '\0') {
        HandleError("unexpected end of input in label value");
    } else {
        ++mTokenLength;
    }
}

void TextParser::HandleCommaOrCloseBrace(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (IsWhitespace(c)) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    if (c == ',') {
        NextState(TextState::OpenBrace);
    } else if (c == '}') {
        SkipSpaceIfHasNext();
        NextState(TextState::SampleValue);
    } else {
        HandleError("expected ',' or '}' after label value");
    }
}

void TextParser::HandleSampleValue(char c, MetricEvent& metricEvent) {
    if (IsWhitespace(c)) {
        auto tmpSampleValue = mLine.substr(mPos - mTokenLength - 1, mTokenLength);

        if (!StringViewToDouble(tmpSampleValue, mSampleValue)) {
            HandleError("invalid sample value");
            mTokenLength = 0;
            return;
        }
        metricEvent.SetValue<UntypedSingleValue>(mSampleValue);
        mTokenLength = 0;

        SkipSpaceIfHasNext();
        NextState(TextState::Timestamp);
    } else if (c == '\0') {
        auto tmpSampleValue = mLine.substr(mPos - mTokenLength, mTokenLength);
        if (!StringViewToDouble(tmpSampleValue, mSampleValue)) {
            HandleError("invalid sample value");
            mTokenLength = 0;
            return;
        }
        metricEvent.SetValue<UntypedSingleValue>(mSampleValue);
        mTokenLength = 0;

        time_t timestamp = mNanoTimestamp / 1000000000;
        auto ns = mNanoTimestamp % 1000000000;
        metricEvent.SetTimestamp(timestamp, ns);
        NextState(TextState::Done);
    } else {
        ++mTokenLength;
    }
}

void TextParser::HandleTimestamp(char c, MetricEvent& metricEvent) {
    if (std::isdigit(c)) {
        ++mTokenLength;
    } else if (IsWhitespace(c) || c == '\0') {
        if (mTokenLength) {
            auto tmpTimestamp = mLine.substr(mPos - mTokenLength - (IsWhitespace(c) ? 1 : 0), mTokenLength);
            auto [ptr, ec]
                = std::from_chars(tmpTimestamp.data(), tmpTimestamp.data() + tmpTimestamp.size(), mNanoTimestamp);
            if (ec != std::errc()) {
                HandleError("invalid timestamp");
                mTokenLength = 0;
                return;
            }
            mNanoTimestamp *= 1000000;
            time_t timestamp = mNanoTimestamp / 1000000000;
            auto ns = mNanoTimestamp % 1000000000;
            metricEvent.SetTimestamp(timestamp, ns);
            mTokenLength = 0;
        }
        NextState(TextState::Done);
    } else {
        HandleError("invalid character in timestamp");
    }
}

void TextParser::HandleError(const string& errMsg) {
    cout << errMsg << mLine << endl;
    LOG_WARNING(sLogger, ("text parser error parsing line", mLine.to_string() + errMsg));
    mState = TextState::Error;
}

void TextParser::SkipSpaceIfHasNext() {
    // before entering this state, we should clear all space characters
    while (mPos < mLine.length() && std::isspace(mLine[mPos])) {
        mPos++;
        // if next char is not space, we should stop
        // because the main loop must read the next char
        if (mPos < mLine.length() && !std::isspace(mLine[mPos])) {
            break;
        }
    }
}

} // namespace logtail
