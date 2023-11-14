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

#include <cstdint>

#include "common/TimeUtil.h"
#include "plugin/interface/Processor.h"

namespace logtail {
class ProcessorParseTimestampNative : public Processor {
public:
    static const std::string sName;

    // 源字段名。
    std::string mSourceKey;
    // 日志时间格式。 %Y/%m/%d %H:%M:%S
    std::string mSourceFormat;
    // 日志时间所属时区。格式为GMT+HH:MM（东区）或GMT-HH:MM（西区）。
    std::string mSourceTimezone = "";
    int mSourceYear = -1;
    TimeStampUnit mPreciseTimestampUnit;
    std::string mPreciseTimestampKey = "ms";

    int mLogTimeZoneOffsetSecond = 0;
    PreciseTimestampConfig mLegacyPreciseTimestampConfig;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    bool ParseTimeZoneOffsetSecond(const std::string& logTZ, int& logTZSecond);
    /// @return false if data need to be discarded
    bool ProcessEvent(StringView logPath, PipelineEventPtr& e, LogtailTime& logTime, StringView& timeStrCache);
    /// @return false if parse time failed
    bool ParseLogTime(const StringView& curTimeStr, // str to parse
                      const StringView& logPath,
                      LogtailTime& logTime,
                      uint64_t& preciseTimestamp,
                      StringView& timeStr // cache
    );
    bool IsPrefixString(const StringView& all, const StringView& prefix);

    int* mParseTimeFailures = nullptr;
    int* mHistoryFailures = nullptr;
    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcDiscardRecordsTotal;
    CounterPtr mProcParseErrorTotal;
    CounterPtr mProcHistoryFailureTotal;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseTimestampNativeUnittest;
    friend class ProcessorParseLogTimeUnittest;
#endif
};
} // namespace logtail