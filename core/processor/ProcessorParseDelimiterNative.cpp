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

#include "processor/ProcessorParseDelimiterNative.h"
#include "common/Constants.h"
#include "models/LogEvent.h"
#include "parser/LogParser.h"
#include "plugin/ProcessorInstance.h"


namespace logtail {

const std::string ProcessorParseDelimiterNative::s_mDiscardedFieldKey = "_";

bool ProcessorParseDelimiterNative::Init(const ComponentConfig& componentConfig) {
    mSourceKey = DEFAULT_CONTENT_KEY;
    mSeparator = componentConfig.GetConfig().mSeparator;
    mColumnKeys = componentConfig.GetConfig().mColumnKeys;
    mExtractPartialFields = componentConfig.GetConfig().mAdvancedConfig.mExtractPartialFields;
    mAutoExtend = componentConfig.GetConfig().mAutoExtend;
    mAcceptNoEnoughKeys = componentConfig.GetConfig().mAcceptNoEnoughKeys;
    mDiscardUnmatch = componentConfig.GetConfig().mDiscardUnmatch;
    mUploadRawLog = componentConfig.GetConfig().mUploadRawLog;
    mQuote = componentConfig.GetConfig().mQuote;
    if (!mSeparator.empty())
        mSeparatorChar = mSeparator.data()[0];
    else {
        // This should never happened.
        mSeparatorChar = '\t';
    }
    mDelimiterModeFsmParserPtr = new DelimiterModeFsmParser(mQuote, mSeparatorChar);
    mParseFailures = &(GetContext().GetProcessProfile().parseFailures);
    mLogGroupSize = &(GetContext().GetProcessProfile().logGroupSize);
    SetMetricsRecordRef(Name(), componentConfig.GetId());
    mProcParseInSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_IN_SIZE_BYTES);
    mProcParseOutSizeBytes = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_OUT_SIZE_BYTES);
    mProcDiscardRecordsTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_DISCARD_RECORDS_TOTAL);
    mProcParseErrorTotal = GetMetricsRecordRef().CreateCounter(METRIC_PROC_PARSE_ERROR_TOTAL);
    return true;
}

void ProcessorParseDelimiterNative::Process(PipelineEventGroup& logGroup) {
    if (logGroup.GetEvents().empty()) {
        return;
    }
    const StringView& logPath = logGroup.GetMetadata(EVENT_META_LOG_FILE_PATH_RESOLVED);
    EventsContainer& events = logGroup.MutableEvents();
    // works good normally. poor performance if most data need to be discarded.
    for (auto it = events.begin(); it != events.end();) {
        if (ProcessEvent(logPath, *it)) {
            ++it;
        } else {
            it = events.erase(it);
        }
    }
    return;
}

bool ProcessorParseDelimiterNative::ProcessEvent(const StringView& logPath, PipelineEventPtr& e) {
    if (!IsSupportedEvent(e)) {
        return true;
    }
    LogEvent& sourceEvent = e.Cast<LogEvent>();
    if (!sourceEvent.HasContent(mSourceKey)) {
        return true;
    }
    StringView buffer = sourceEvent.GetContent(mSourceKey);
    mProcParseInSizeBytes->Add(buffer.size());
    int32_t endIdx = buffer.size();
    if (endIdx == 0)
        return true;

    for (int32_t i = endIdx - 1; i >= 0; --i) {
        if (buffer.data()[i] == ' ' || '\r' == buffer.data()[i])
            endIdx = i;
        else
            break;
    }
    int32_t begIdx = 0;
    for (int32_t i = 0; i < endIdx; ++i) {
        if (buffer.data()[i] == ' ')
            begIdx = i + 1;
        else
            break;
    }
    if (begIdx >= endIdx)
        return true;
    size_t reserveSize = mAutoExtend ? (mColumnKeys.size() + 10) : (mColumnKeys.size() + 1);
    std::vector<StringView> columnValues;
    std::vector<size_t> colBegIdxs;
    std::vector<size_t> colLens;
    bool parseSuccess = false;
    size_t parsedColCount = 0;
    bool useQuote = (mSeparator.size() == 1) && (mQuote != mSeparatorChar);
    if (mColumnKeys.size() > 0) {
        if (useQuote) {
            columnValues.reserve(reserveSize);
            parseSuccess = mDelimiterModeFsmParserPtr->ParseDelimiterLine(buffer, begIdx, endIdx, columnValues);
            // handle auto extend
            if (!mAutoExtend && columnValues.size() > mColumnKeys.size()) {
                int requiredLen = 0;
                for (size_t i = mColumnKeys.size(); i < columnValues.size(); ++i) {
                    requiredLen += 1 + columnValues[i].size();
                }
                StringBuffer sb = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(requiredLen);
                char* extraFields = sb.data;
                for (size_t i = mColumnKeys.size(); i < columnValues.size(); ++i) {
                    extraFields[0] = mSeparatorChar;
                    extraFields++;
                    strcpy(extraFields, columnValues[i].data());
                    extraFields += columnValues[i].size();
                }
                // remove extra fields
                columnValues.resize(mColumnKeys.size());
                columnValues.push_back(StringView(sb.data, requiredLen));
            }
            parsedColCount = columnValues.size();
        } else {
            colBegIdxs.reserve(reserveSize);
            colLens.reserve(reserveSize);
            parseSuccess = SplitString(buffer.data(), begIdx, endIdx, colBegIdxs, colLens);
            parsedColCount = colBegIdxs.size();
        }

        if (parseSuccess) {
            if (parsedColCount <= 0 || (!mAcceptNoEnoughKeys && parsedColCount < mColumnKeys.size())) {
                LOG_WARNING(sLogger,
                            ("parse delimiter log fail, keys count unmatch "
                             "columns count, parsed",
                             parsedColCount)("required", mColumnKeys.size())("log", buffer)(
                                "project", GetContext().GetProjectName())("logstore", GetContext().GetLogstoreName())(
                                "file", logPath));
                GetContext().GetAlarm().SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                  std::string("keys count unmatch columns count :")
                                                      + ToString(parsedColCount) + ", required:"
                                                      + ToString(mColumnKeys.size()) + ", logs:" + buffer.to_string(),
                                                  GetContext().GetProjectName(),
                                                  GetContext().GetLogstoreName(),
                                                  GetContext().GetRegion());
                mProcParseErrorTotal->Add(1);
                ++(*mParseFailures);
                parseSuccess = false;
            }
        } else {
            LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                                   std::string("parse delimiter log fail")
                                                       + ", logs:" + buffer.to_string(),
                                                   GetContext().GetProjectName(),
                                                   GetContext().GetLogstoreName(),
                                                   GetContext().GetRegion());
            mProcParseErrorTotal->Add(1);
            ++(*mParseFailures);
            parseSuccess = false;
        }
    } else {
        LogtailAlarm::GetInstance()->SendAlarm(PARSE_LOG_FAIL_ALARM,
                                               "no column keys defined",
                                               GetContext().GetProjectName(),
                                               GetContext().GetLogstoreName(),
                                               GetContext().GetRegion());
        LOG_WARNING(sLogger,
                    ("parse delimiter log fail", "no column keys defined")("project", GetContext().GetProjectName())(
                        "logstore", GetContext().GetLogstoreName())("file", logPath));
        mProcParseErrorTotal->Add(1);
        ++(*mParseFailures);
        parseSuccess = false;
    }

    if (parseSuccess) {
        for (uint32_t idx = 0; idx < parsedColCount; idx++) {
            if (mColumnKeys.size() > idx) {
                if (mExtractPartialFields && mColumnKeys[idx] == s_mDiscardedFieldKey) {
                    continue;
                }
                AddLog(mColumnKeys[idx],
                       useQuote ? columnValues[idx] : StringView(buffer.data() + colBegIdxs[idx], colLens[idx]),
                       sourceEvent);
            } else {
                if (mExtractPartialFields) {
                    continue;
                }
                StringBuffer sb = sourceEvent.GetSourceBuffer()->AllocateStringBuffer(10 + idx / 10);
                std::string key = "__column" + ToString(idx) + "__";
                strcpy(sb.data, key.c_str());
                AddLog(StringView(sb.data, sb.size),
                       useQuote ? columnValues[idx] : StringView(buffer.data() + colBegIdxs[idx], colLens[idx]),
                       sourceEvent);
            }
        }
        sourceEvent.DelContent(mSourceKey);
        return true;
    } else if (!mDiscardUnmatch) {
        AddLog(LogParser::UNMATCH_LOG_KEY, // __raw_log__
               sourceEvent.GetContent(mSourceKey),
               sourceEvent); // legacy behavior, should use sourceKey
        sourceEvent.DelContent(mSourceKey);
        return true;
    }
    mProcDiscardRecordsTotal->Add(1);
    return false;
}

bool ProcessorParseDelimiterNative::SplitString(
    const char* buffer, int32_t begIdx, int32_t endIdx, std::vector<size_t>& colBegIdxs, std::vector<size_t>& colLens) {
    if (endIdx <= begIdx || mSeparator.size() == 0 || mColumnKeys.size() == 0)
        return false;
    size_t size = endIdx - begIdx;
    size_t d_size = mSeparator.size();
    if (d_size == 0 || d_size > size) {
        colBegIdxs.push_back(begIdx);
        colLens.push_back(size);
        return true;
    }
    size_t pos = begIdx;
    size_t top = endIdx - d_size;
    while (pos <= top) {
        const char* pch = strstr(buffer + pos, mSeparator.c_str());
        size_t pos2 = pch == NULL ? endIdx : (pch - buffer);
        if (pos2 != pos) {
            colBegIdxs.push_back(pos);
            colLens.push_back(pos2 - pos);
        } else {
            colBegIdxs.push_back(pos);
            colLens.push_back(0);
        }
        if (pos2 == (size_t)endIdx)
            return true;
        pos = pos2 + d_size;
        if (colLens.size() >= mColumnKeys.size() && !mAutoExtend) {
            colBegIdxs.push_back(pos2);
            colLens.push_back(endIdx - pos2);
            return true;
        }
    }
    if (pos <= (size_t)endIdx) {
        colBegIdxs.push_back(pos);
        colLens.push_back(endIdx - pos);
    }
    return true;
}

void ProcessorParseDelimiterNative::AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent) {
    targetEvent.SetContentNoCopy(key, value);
    *mLogGroupSize += key.size() + value.size() + 5;
    mProcParseOutSizeBytes->Add(key.size() + value.size());
}

bool ProcessorParseDelimiterNative::IsSupportedEvent(const PipelineEventPtr& e) {
    return e.Is<LogEvent>();
}

} // namespace logtail