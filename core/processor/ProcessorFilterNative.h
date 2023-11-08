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
#include "processor/BaseFilterNode.h"

namespace logtail {

class ProcessorFilterNative : public Processor {
public:
    static const std::string sName;

    // 日志字段白名单。多个条件之间为“且”的关系，仅当所有条件均满足时，该条日志才会被采集。
    std::unordered_map<std::string, std::string> mInclude;
    // ConditionExp
    BaseFilterNodePtr mConditionExp = nullptr;
    // 忽略非UTF8编码的日志
    bool mDiscardingNonUTF8 = false;

    const std::string& Name() const override { return sName; }
    bool Init(const Json::Value& config) override;
    void Process(PipelineEventGroup& logGroup) override;
    ~ProcessorFilterNative();

protected:
    bool IsSupportedEvent(const PipelineEventPtr& e) const override;

private:
    enum class Mode { BYPASS_MODE, EXPRESSION_MODE, RULE_MODE };
    Mode mFilterMode = Mode::BYPASS_MODE;
    struct LogFilterRule {
        std::vector<std::string> FilterKeys;
        std::vector<boost::regex> FilterRegs;
    };
    std::shared_ptr<LogFilterRule> mFilterRule;

    CounterPtr mProcFilterErrorTotal;
    CounterPtr mProcFilterRecordsTotal;

    bool FilterExpressionRoot(LogEvent& sourceEvent, const BaseFilterNodePtr& node);

    bool FilterFilterRule(LogEvent& sourceEvent, const LogFilterRule* filterRule);
    bool IsMatched(const LogContents& contents, const LogFilterRule& rule);

    bool ProcessEvent(PipelineEventPtr& e);

    bool noneUtf8(StringView& strSrc, bool modify);
    bool CheckNoneUtf8(const StringView& strSrc);
    void FilterNoneUtf8(std::string& strSrc);

    BaseFilterNodePtr ParseExpressionFromJSON(const Json::Value& value);
    static bool GetOperatorType(const std::string& type, FilterOperator& op);
    static bool GetNodeFuncType(const std::string& type, FilterNodeFunctionType& func);
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorFilterNativeUnittest;
#endif
};

} // namespace logtail