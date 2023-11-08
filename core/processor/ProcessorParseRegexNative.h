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

#include "boost/regex.hpp"
#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"

namespace logtail {
enum ParseLogError {
    PARSE_LOG_REGEX_ERROR,
    PARSE_LOG_FORMAT_ERROR,
    PARSE_LOG_TIMEFORMAT_ERROR,
    PARSE_LOG_HISTORY_ERROR
};
struct UserDefinedFormat {
    boost::regex mReg;
    std::vector<std::string> mKeys;
    bool mIsWholeLineMode;
    UserDefinedFormat(const boost::regex& reg, const std::vector<std::string>& keys, bool isWholeLineMode)
        : mReg(reg), mKeys(keys), mIsWholeLineMode(isWholeLineMode) {}
};

class ProcessorParseRegexNative : public Processor {
public:
    static const std::string sName;
    static const std::string UNMATCH_LOG_KEY;

    // 源字段名。
    std::string mSourceKey;
    // 正则表达式。
    std::string mRegex;
    // 提取的字段列表。
    std::vector<std::string> mKeys;
    // 当解析失败时，是否保留源字段。
    bool mKeepingSourceWhenParseFail = false;
    // 当解析成功时，是否保留源字段。
    bool mKeepingSourceWhenParseSucceed = false;
    // 当源字段被保留时，用于存储源字段的字段名。若不填，默认不改名。
    std::string mRenamedSourceKey = "";
    bool mCopingRawLog = false;


    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    void AddUserDefinedFormat();
    /// @return false if data need to be discarded
    bool ProcessEvent(const StringView& logPath, PipelineEventPtr& e);
    bool WholeLineModeParser(LogEvent& sourceEvent, const std::string& key);
    bool RegexLogLineParser(LogEvent& sourceEvent,
                            const boost::regex& reg,
                            const std::vector<std::string>& keys,
                            const StringView& logPath);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);
    bool mSourceKeyOverwritten = false;
    bool mRawLogTagOverwritten = false;
    std::vector<UserDefinedFormat> mUserDefinedFormat;

    int* mParseFailures = nullptr;
    int* mRegexMatchFailures = nullptr;
    int* mLogGroupSize = nullptr;
    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcDiscardRecordsTotal;
    CounterPtr mProcParseErrorTotal;
    CounterPtr mProcKeyCountNotMatchErrorTotal;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseRegexNativeUnittest;
#endif
};
} // namespace logtail