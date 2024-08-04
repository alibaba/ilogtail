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
#include <cmath>
#include <sstream>
#include <string>

#include "common/StringTools.h"
#include "logger/Logger.h"
#include "models/MetricEvent.h"
#include "models/PipelineEventGroup.h"

using namespace std;

namespace logtail {

TextEvent ClassifyChar(char c) {
    if (std::isalpha(c) || c == '_' || c == ':' || c == '-')
        return TextEvent::Character;
    if (std::isdigit(c))
        return TextEvent::Digit;
    if (c == '=')
        return TextEvent::Equal;
    if (c == '"')
        return TextEvent::Quote;
    if (c == ',')
        return TextEvent::Comma;
    if (c == '{')
        return TextEvent::OpenBrace;
    if (c == '}')
        return TextEvent::CloseBrace;
    if (std::isspace(c))
        return TextEvent::Space;
    if (c == '\0')
        return TextEvent::EndOfInput;
    return TextEvent::Invalid;
}

PipelineEventGroup TextParser::Parse(const string& content, uint64_t defaultNanoTs) {
    auto eGroup = PipelineEventGroup(make_shared<SourceBuffer>());

    for (const auto& line : SplitString(content, "\r\n")) {
        auto newLine = TrimString(line);
        if (newLine.empty() || newLine[0] == '#') {
            continue;
        }
        auto metricEvent = eGroup.CreateMetricEvent();
        if (ParseLine(newLine, defaultNanoTs, *metricEvent)) {
            eGroup.MutableEvents().emplace_back(std::move(metricEvent));
        }
    }

    return eGroup;
}

bool TextParser::ParseLine(const string& line, uint64_t defaultNanoTs, MetricEvent& metricEvent) {
    mLine = line;
    mPos = 0;
    mState = TextState::Start;
    mMetricName.clear();
    mLabelName.clear();
    mLabelValue.clear();
    mToken.str("");
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
    while (ClassifyChar(c) == TextEvent::Space) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    if (ClassifyChar(c) == TextEvent::Character) {
        mToken << c;
        NextState(TextState::MetricName);
    } else {
        HandleError("expected metric name");
    }
}

void TextParser::HandleMetricName(char c, MetricEvent& metricEvent) {
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Character || event == TextEvent::Digit || c == '_') {
        mToken << c;
    } else if (event == TextEvent::OpenBrace || event == TextEvent::Space) {
        mMetricName = mToken.str();
        metricEvent.SetName(mMetricName);
        mToken.str("");
        // Ignore subsequent spaces
        while (ClassifyChar(c) == TextEvent::Space) {
            c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
        }
        if (ClassifyChar(c) == TextEvent::OpenBrace) {
            NextState(TextState::OpenBrace);
        } else {
            NextState(TextState::SampleValue);
        }
    } else {
        HandleError("Invalid character in metric name");
    }
}

void TextParser::HandleOpenBrace(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (ClassifyChar(c) == TextEvent::Space) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Character) {
        mToken << c;
        NextState(TextState::LabelName);
    } else if (event == TextEvent::CloseBrace) {
        SkipSpaceIfHasNext();
        NextState(TextState::SampleValue);
    } else {
        HandleError("expected label name after '{'");
    }
}

void TextParser::HandleLabelName(char c, MetricEvent&) {
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Character || event == TextEvent::Digit || c == '_') {
        mToken << c;
    } else if (event == TextEvent::Equal || event == TextEvent::Space) {
        mLabelName = mToken.str();
        mToken.str("");
        // Ignore subsequent spaces
        while (ClassifyChar(c) == TextEvent::Space) {
            c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
        }
        if (ClassifyChar(c) != TextEvent::Equal) {
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
    while (ClassifyChar(c) == TextEvent::Space) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Quote) {
        NextState(TextState::LabelValue);
    } else {
        HandleError("expected '\"' after '='");
    }
}

void TextParser::HandleLabelValue(char c, MetricEvent& metricEvent) {
    if (ClassifyChar(c) == TextEvent::Quote) {
        mLabelValue = mToken.str();
        metricEvent.SetTag(mLabelName, mLabelValue);
        mToken.str("");
        NextState(TextState::CommaOrCloseBrace);
    } else if (c == '\0') {
        HandleError("unexpected end of input in label value");
    } else {
        mToken << c;
    }
}

void TextParser::HandleCommaOrCloseBrace(char c, MetricEvent&) {
    // Ignore subsequent spaces
    while (ClassifyChar(c) == TextEvent::Space) {
        c = (mPos < mLine.size()) ? mLine[mPos++] : '\0';
    }
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Comma) {
        NextState(TextState::OpenBrace);
    } else if (event == TextEvent::CloseBrace) {
        SkipSpaceIfHasNext();
        NextState(TextState::SampleValue);
    } else {
        HandleError("expected ',' or '}' after label value");
    }
}

void TextParser::HandleSampleValue(char c, MetricEvent& metricEvent) {
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Digit || c == '.' || c == '-' || c == '+' || c == 'e' || c == 'E') {
        mToken << c;
    } else if (event == TextEvent::Space || event == TextEvent::EndOfInput) {
        try {
            mSampleValue = stod(mToken.str());
        } catch (...) {
            HandleError("invalid sample value");
            mToken.str("");
            return;
        }
        metricEvent.SetValue<UntypedSingleValue>(mSampleValue);
        mToken.str("");

        if (ClassifyChar(c) == TextEvent::Space) {
            SkipSpaceIfHasNext();
            NextState(TextState::Timestamp);
        } else {
            time_t timestamp = mNanoTimestamp / 1000000000;
            auto ns = mNanoTimestamp % 1000000000;
            metricEvent.SetTimestamp(timestamp, ns);
            NextState(TextState::Done);
        }
    } else {
        HandleError("invalid character in sample value");
    }
}

void TextParser::HandleTimestamp(char c, MetricEvent& metricEvent) {
    TextEvent event = ClassifyChar(c);
    if (event == TextEvent::Digit) {
        mToken << c;
    } else if (event == TextEvent::Space || event == TextEvent::EndOfInput) {
        if (!mToken.str().empty()) {
            try {
                mNanoTimestamp = stoull(mToken.str()) * 1000000;
            } catch (...) {
                HandleError("invalid timestamp");
                mToken.str("");
                return;
            }
            time_t timestamp = mNanoTimestamp / 1000000000;
            auto ns = mNanoTimestamp % 1000000000;
            metricEvent.SetTimestamp(timestamp, ns);
            mToken.str("");
        }
        NextState(TextState::Done);
    } else {
        HandleError("invalid character in timestamp");
    }
}

void TextParser::HandleError(const string& errMsg) {
    LOG_WARNING(sLogger, ("text parser error parsing line", mLine + errMsg));
    mState = TextState::Error;
}

void TextParser::SkipSpaceIfHasNext() {
    // before entering this state, we should clear all space characters
    while (mPos < mLine.length() && ClassifyChar(mLine[mPos]) == TextEvent::Space) {
        mPos++;
        // if next char is not space, we should stop
        // because the main loop must read the next char
        if (mPos < mLine.length() && ClassifyChar(mLine[mPos]) != TextEvent::Space) {
            break;
        }
    }
}

} // namespace logtail
