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
#include "app_config/AppConfig.h"
#include "common/LogGroupContext.h"

namespace logtail {

// BaseFilterNode
enum FilterOperator { NOT_OPERATOR, AND_OPERATOR, OR_OPERATOR };

enum FilterNodeType { VALUE_NODE, OPERATOR_NODE };

enum FilterNodeFunctionType { REGEX_FUNCTION };

class BaseFilterNode {
public:
    explicit BaseFilterNode(FilterNodeType nodeType) : nodeType(nodeType) {}
    virtual ~BaseFilterNode() {}

public:
    virtual bool Match(const sls_logs::Log& log, const LogGroupContext& context) { return true; }
    virtual bool Match(const LogContents& contents, const PipelineContext& mContext) { return true; }

public:
    FilterNodeType GetNodeType() const { return nodeType; }

private:
    FilterNodeType nodeType;
};

typedef std::shared_ptr<BaseFilterNode> BaseFilterNodePtr;

// BinaryFilterOperatorNode
class BinaryFilterOperatorNode : public BaseFilterNode {
public:
    BinaryFilterOperatorNode(FilterOperator op, BaseFilterNodePtr left, BaseFilterNodePtr right)
        : BaseFilterNode(OPERATOR_NODE), op(op), left(left), right(right) {}
    virtual ~BinaryFilterOperatorNode(){};

public:
    virtual bool Match(const sls_logs::Log& log, const LogGroupContext& context) {
        if (BOOST_LIKELY(left && right)) {
            if (op == AND_OPERATOR) {
                return left->Match(log, context) && right->Match(log, context);
            } else if (op == OR_OPERATOR) {
                return left->Match(log, context) || right->Match(log, context);
            }
        }
        return false;
    }

    virtual bool Match(const LogContents& contents, const PipelineContext& mContext) {
        if (BOOST_LIKELY(left && right)) {
            if (op == AND_OPERATOR) {
                return left->Match(contents, mContext) && right->Match(contents, mContext);
            } else if (op == OR_OPERATOR) {
                return left->Match(contents, mContext) || right->Match(contents, mContext);
            }
        }
        return false;
    }

private:
    FilterOperator op;
    BaseFilterNodePtr left;
    BaseFilterNodePtr right;
};

// RegexFilterValueNode
class RegexFilterValueNode : public BaseFilterNode {
public:
    RegexFilterValueNode(const std::string& key, const std::string& exp)
        : BaseFilterNode(VALUE_NODE), key(key), reg(exp) {}

    virtual ~RegexFilterValueNode() {}

public:
    virtual bool Match(const sls_logs::Log& log, const LogGroupContext& context) {
        for (int i = 0; i < log.contents_size(); ++i) {
            const sls_logs::Log_Content& content = log.contents(i);
            if (content.key() != key) {
                continue;
            }

            std::string exception;
            bool result = BoostRegexMatch(content.value().c_str(), content.value().size(), reg, exception);
            if (!result && !exception.empty() && AppConfig::GetInstance()->IsLogParseAlarmValid()) {
                LOG_ERROR(sLogger, ("regex_match in Filter fail", exception));
                if (LogtailAlarm::GetInstance()->IsLowLevelAlarmValid()) {
                    context.SendAlarm(REGEX_MATCH_ALARM, "regex_match in Filter fail:" + exception);
                }
            }
            return result;
        }
        return false;
    }

    virtual bool Match(const LogContents& contents, const PipelineContext& mContext) {
        const auto& content = contents.find(key);
        if (content == contents.end()) {
            return false;
        }

        std::string exception;
        bool result = BoostRegexMatch(content->second.data(), content->second.size(), reg, exception);
        if (!result && !exception.empty() && AppConfig::GetInstance()->IsLogParseAlarmValid()) {
            LOG_ERROR(mContext.GetLogger(), ("regex_match in Filter fail", exception));
            if (mContext.GetAlarm().IsLowLevelAlarmValid()) {
                mContext.GetAlarm().SendAlarm(REGEX_MATCH_ALARM,
                                              "regex_match in Filter fail:" + exception,
                                              mContext.GetProjectName(),
                                              mContext.GetLogstoreName(),
                                              mContext.GetRegion());
            }
        }
        return result;
    }

private:
    std::string key;
    boost::regex reg;
};

// UnaryFilterOperatorNode
class UnaryFilterOperatorNode : public BaseFilterNode {
public:
    UnaryFilterOperatorNode(FilterOperator op, BaseFilterNodePtr child)
        : BaseFilterNode(OPERATOR_NODE), op(op), child(child) {}

    virtual ~UnaryFilterOperatorNode(){};

public:
    virtual bool Match(const sls_logs::Log& log, const LogGroupContext& context) {
        if (BOOST_LIKELY(child.get() != NULL)) {
            return !child->Match(log, context);
        }
        return false;
    }

    virtual bool Match(const LogContents& contents, const PipelineContext& mContext) {
        if (BOOST_LIKELY(child.get() != NULL)) {
            return !child->Match(contents, mContext);
        }
        return false;
    }

private:
    FilterOperator op;
    BaseFilterNodePtr child;
};

static BaseFilterNodePtr ParseExpressionFromJSON(const Json::Value& value);
static bool GetOperatorType(const std::string& type, FilterOperator& op);
static bool GetNodeFuncType(const std::string& type, FilterNodeFunctionType& func);

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

    bool ProcessEvent(PipelineEventPtr& e);

    // 通过ConditionExp过滤日志
    bool FilterExpressionRoot(LogEvent& sourceEvent, const BaseFilterNodePtr& node);

    // 通过FilterRule过滤日志
    bool FilterFilterRule(LogEvent& sourceEvent, const LogFilterRule* filterRule);
    bool IsMatched(const LogContents& contents, const LogFilterRule& rule);

    // 丢弃非UTF8编码的日志
    bool noneUtf8(StringView& strSrc, bool modify);
    bool CheckNoneUtf8(const StringView& strSrc);
    void FilterNoneUtf8(std::string& strSrc);

    CounterPtr mProcFilterErrorTotal;
    CounterPtr mProcFilterRecordsTotal;
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ProcessorFilterNativeUnittest;
#endif
};

} // namespace logtail