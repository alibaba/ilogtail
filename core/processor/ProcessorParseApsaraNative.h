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

#include "processor/Processor.h"
#include <string>
#include <boost/regex.hpp>

namespace logtail {

class ProcessorParseApsaraNative : public Processor {
public:
    static const char* Name() { return "processor_parse_apsara_native"; }
    bool Init(const ComponentConfig& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) override;

private:
    bool ProcessEvent(const StringView& logPath, PipelineEventPtr& e, time_t& lastLogTime, StringView& timeStrCache);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);
    time_t ApsaraEasyReadLogTimeParser(StringView& buffer, StringView& timeStr, time_t& lastLogTime, int64_t& microTime);
    int32_t GetApsaraLogMicroTime(StringView& buffer);
    bool IsPrefixString(const char* all, const StringView& prefix);
    int32_t ParseApsaraBaseFields(StringView& buffer, LogEvent& sourceEvent);

    std::string mSourceKey;
    bool mDiscardUnmatch = false;
    bool mUploadRawLog = false;
    bool mAdjustApsaraMicroTimezone = false;
    int mLogTimeZoneOffsetSecond = 0;
    int* mLogGroupSize = nullptr;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseApsaraNativeUnittest;
#endif
};

} // namespace logtail