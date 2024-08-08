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
    metricEvent.SetName(mLine.substr(mPos - mTokenLength, mTokenLength).to_string());
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
        c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
        if (c != '=') {
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
    char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    if (c == '"') {
        ++mPos;
        HandleLabelValue(metricEvent);
    } else {
        HandleError("expected '\"' after '='");
    }
}

void TextParser::HandleLabelValue(MetricEvent& metricEvent) {
    // left quote has been consumed
    if (mNoEscapes) {
        while (mPos < mLine.size() && mLine[mPos] != '"') {
            ++mPos;
            ++mTokenLength;
        }
        if (mPos == mLine.size()) {
            HandleError("unexpected end of input in label value");
        }
        metricEvent.SetTag(mLabelName, mLine.substr(mPos - mTokenLength, mTokenLength));
        mTokenLength = 0;
        ++mPos;
        SkipLeadingWhitespace();
        char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
        if (c == ',' || c == '}') {
            HandleCommaOrCloseBrace(metricEvent);
        } else {
            HandleError("unexpected end of input in label value");
        }
    } else {
        HandleError("unsupported escape sequence");
        // while (mPos < mLine.size() && mLine[mPos] == '"' && mLine[mPos - 1] != '\\') {
        //     ++mPos;
        // }
        // if (mPos == mLine.size()) {
        //     HandleError("unexpected end of input in label value");
        // }
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
    while (mPos < mLine.size() && !IsWhitespace(mLine[mPos])) {
        ++mPos;
        ++mTokenLength;
    }
    char c = (mPos < mLine.size()) ? mLine[mPos] : '\0';

    auto tmpSampleValue = mLine.substr(mPos - mTokenLength, mTokenLength);

    if (!StringViewToDouble(tmpSampleValue, mSampleValue)) {
        HandleError("invalid sample value");
        mTokenLength = 0;
        return;
    }
    metricEvent.SetValue<UntypedSingleValue>(mSampleValue);
    mTokenLength = 0;
    SkipLeadingWhitespace();
    c = (mPos < mLine.size()) ? mLine[mPos] : '\0';
    if (c == '\0') {
        time_t timestamp = mNanoTimestamp / 1000000000;
        auto ns = mNanoTimestamp % 1000000000;
        metricEvent.SetTimestamp(timestamp, ns);
        mState = TextState::Done;
    } else {
        HandleTimestamp(metricEvent);
    }
}

void TextParser::HandleTimestamp(MetricEvent& metricEvent) {
    while (mPos < mLine.size() && (mLine[mPos] != ' ' && mLine[mPos] != '\t')) {
        ++mPos;
        ++mTokenLength;
    }
    auto tmpTimestamp = mLine.substr(mPos - mTokenLength, mTokenLength);
    auto [ptr, ec] = std::from_chars(tmpTimestamp.data(), tmpTimestamp.data() + tmpTimestamp.size(), mNanoTimestamp);
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

    mState = TextState::Done;
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

void TextParser::SkipLeadingWhitespace() {
    while (mPos < mLine.length() && IsWhitespace(mLine[mPos])) {
        mPos++;
    }
}

} // namespace logtail
