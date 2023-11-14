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

#include "rapidjson/document.h"
#include "models/LogEvent.h"
#include "plugin/interface/Processor.h"

namespace logtail {

class ProcessorParseJsonNative : public Processor {
public:
    static const std::string sName;
    static const std::string UNMATCH_LOG_KEY;

    // 源字段名。
    std::string mSourceKey;
    // 当解析失败时，是否保留源字段。
    bool mKeepingSourceWhenParseFail = false;
    // 当解析成功时，是否保留源字段。
    bool mKeepingSourceWhenParseSucceed = false;
    // 当原始字段被保留时，用于存储原始字段的字段名。若不填，默认不改名。
    std::string mRenamedSourceKey = "";
    bool mCopingRawLog = false;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    bool mSourceKeyOverwritten = false;
    bool mRawLogTagOverwritten = false;

    int* mParseFailures = nullptr;
    int* mLogGroupSize = nullptr;

    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcDiscardRecordsTotal;
    CounterPtr mProcParseErrorTotal;

    bool JsonLogLineParser(LogEvent& sourceEvent, const StringView& logPath, PipelineEventPtr& e);
    void AddLog(const StringView& key, const StringView& value, LogEvent& targetEvent);
    bool ProcessEvent(const StringView& logPath, PipelineEventPtr& e);
    static std::string RapidjsonValueToString(const rapidjson::Value& value);

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorParseJsonNativeUnittest;
#endif
};

} // namespace logtail