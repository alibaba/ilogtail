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

#include "common/TimeUtil.h"
#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"

namespace logtail {

class ProcessorParseApsaraNative : public Processor {
public:
    static const std::string sName;

    // 源字段名。
    std::string mSourceKey;
    // 日志时间所属时区。格式为GMT+HH:MM（东区）或GMT-HH:MM（西区）。
    std::string mTimezone = "";
    // 当解析失败时，是否保留源字段。
    bool mKeepingSourceWhenParseFail = false;
    // 当解析成功时，是否保留源字段。
    bool mKeepingSourceWhenParseSucceed = false;
    // 当原始字段被保留时，用于存储原始字段的字段名。若不填，默认不改名。
    std::string mRenamedSourceKey = "";
    bool mCopingRawLog = false;
    bool mAdjustingMicroTimezone = false;

    int mLogTimeZoneOffsetSecond = 0;
    bool mSourceKeyOverwritten = false;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    bool
    ProcessEvent(const StringView& logPath, PipelineEventPtr& e, LogtailTime& lastLogTime, StringView& timeStrCache);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);
    time_t
    ApsaraEasyReadLogTimeParser(StringView& buffer, StringView& timeStr, LogtailTime& lastLogTime, int64_t& microTime);
    int32_t GetApsaraLogMicroTime(StringView& buffer);
    bool IsPrefixString(const char* all, const StringView& prefix);
    int32_t ParseApsaraBaseFields(StringView& buffer, LogEvent& sourceEvent);
    bool ParseTimeZoneOffsetSecond(const std::string& logTZ, int& logTZSecond);

    int* mLogGroupSize = nullptr;
    int* mParseFailures = nullptr;
    int* mHistoryFailures = nullptr;
    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcDiscardRecordsTotal;
    CounterPtr mProcParseErrorTotal;
    CounterPtr mProcHistoryFailureTotal;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseApsaraNativeUnittest;
#endif
};

} // namespace logtail
