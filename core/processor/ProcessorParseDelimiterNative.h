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

#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"
#include "parser/DelimiterModeFsmParser.h"

namespace logtail {

class ProcessorParseDelimiterNative : public Processor {
public:
    static const std::string sName;

    enum class Method { extend, keep, discard };
    // 源字段名。
    std::string mSourceKey;
    // 分隔符
    std::string mSeparator;
    // 引用符
    char mQuote = '"';
    // 提取的字段列表。
    std::vector<std::string> mKeys;
    // 是否允许提取的字段数量小于Keys的数量。若不允许，则此情景会被视为解析失败。
    bool mAllowingShortenedFields = true;
    // 当提取的字段数量大于Keys的数量时的行为。可选值包括：
    // ●
    // extend：保留多余的字段，且每个多余的字段都作为单独的一个字段加入日志，多余字段的字段名为__column$i__，其中$i代表额外字段序号，从0开始计数。
    // ● keep：保留多余的字段，但将多余内容作为一个整体字段加入日志，字段名为__column0__.
    // ● discard：丢弃多余的字段。
    std::string mOverflowedFieldsTreatment = "extend";
    // 当解析失败时，是否保留源字段。
    bool mKeepingSourceWhenParseFail = true;
    // 当解析成功时，是否保留源字段。
    bool mKeepingSourceWhenParseSucceed = false;
    // 当源字段被保留时，用于存储源字段的字段名。若不填，默认不改名。
    std::string mRenamedSourceKey = "";

    bool mCopingRawLog = false;
    char mSeparatorChar;
    bool mSourceKeyOverwritten = false;
    bool mRawLogTagOverwritten = false;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    bool ProcessEvent(const StringView& logPath, PipelineEventPtr& e);
    bool SplitString(const char* buffer,
                     int32_t begIdx,
                     int32_t endIdx,
                     std::vector<size_t>& colBegIdxs,
                     std::vector<size_t>& colLens);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);

    DelimiterModeFsmParser* mDelimiterModeFsmParserPtr;
    static const std::string s_mDiscardedFieldKey;

    int* mLogGroupSize = nullptr;
    int* mParseFailures = nullptr;

    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcDiscardRecordsTotal;
    CounterPtr mProcParseErrorTotal;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseDelimiterNativeUnittest;
#endif
};

} // namespace logtail