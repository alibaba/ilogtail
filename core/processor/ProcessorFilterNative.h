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
#include "processor/Processor.h"
#include <string>
#include <boost/regex.hpp>
#include "LogFilter.h"

namespace logtail {

class ProcessorFilterNative : public Processor {
public:
    static const char* Name() { return "processor_filter_native"; }
    bool Init(const ComponentConfig& componentConfig) override;
    void Process(PipelineEventGroup& logGroup) override;

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) override;

private:
    ~ProcessorFilterNative() {
        std::unordered_map<std::string, LogFilterRule*>::iterator it = mFilters.begin();
        for (; it != mFilters.end(); ++it) {
            delete it->second;
        }
        mFilters.clear();
    }
    std::shared_ptr<LogFilterRule> mFilterRule = nullptr;
    BaseFilterNodePtr mFilterExpressionRoot = nullptr;
    std::unordered_map<std::string, LogFilterRule*> mFilters;
    LogType mLogType;
    bool mDiscardNoneUtf8;

    int* mParseFailures = nullptr;
    CounterPtr mProcParseInSizeBytes;
    CounterPtr mProcParseOutSizeBytes;
    CounterPtr mProcParseErrorTotal;
    CounterPtr mProcDiscardRecordsTotal;

    bool Filter(LogEvent& sourceEvent, const BaseFilterNodePtr& node);
    bool Filter(LogEvent& sourceEvent, const LogFilterRule* filterRule);
    bool Filter(LogEvent& sourceEvent);

    bool ProcessEvent(PipelineEventPtr& e);
    bool IsMatched(const LogContents& contents, const LogFilterRule& rule);
    bool FilterNoneUtf8(const std::string& strSrc, bool isNoneUtf8);
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorFilterNativeUnittest;
#endif
};

} // namespace logtail