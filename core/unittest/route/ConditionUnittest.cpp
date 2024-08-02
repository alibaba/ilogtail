// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/JsonUtil.h"
#include "route/Condition.h"
#include "unittest/Unittest.h"

using namespace std;

namespace logtail {

class ConditionUnittest : public testing::Test {
public:
    void TestInit();
    void TestCheck();

private:
    PipelineContext ctx;
};

void ConditionUnittest::TestInit() {
    Json::Value configJson;
    string configStr, errorMsg;
    {
        configStr = R"(
            {
                "Type": "event_type",
                "Condition": "log"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL(Condition::Type::EVENT_TYPE, cond.mType);
    }
    {
        configStr = R"(
            {
                "Type": "tag_value",
                "Condition": "INFO"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL(Condition::Type::TAG_VALUE, cond.mType);
    }
    {
        configStr = R"(
            {
                "type": "event_type"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "type": "tag_value"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "Type": true
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "Type": ""
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "Type": "unknown"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "Type": "event_type"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(
            {
                "Type": "event_type",
                "Condition": "unknown"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
}

void ConditionUnittest::TestCheck() {
    string errorMsg;
    {
        Json::Value configJson;
        string configStr = R"(
            {
                "Type": "event_type",
                "Condition": "log"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));

        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.AddLogEvent();
        APSARA_TEST_TRUE(cond.Check(g));
    }
    {
        Json::Value configJson;
        string configStr = R"(
            {
                "Type": "tag_value",
                "Condition": "INFO"
            }
        )";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        Condition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));

        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.SetTag(TagValueCondition::sTagKey, "INFO");
        APSARA_TEST_TRUE(cond.Check(g));
    }
}

UNIT_TEST_CASE(ConditionUnittest, TestInit)
UNIT_TEST_CASE(ConditionUnittest, TestCheck)

class EventTypeConditionUnittest : public testing::Test {
public:
    void TestInit();
    void TestCheck();

private:
    PipelineContext ctx;
};

void EventTypeConditionUnittest::TestInit() {
    Json::Value configJson;
    string configStr, errorMsg;
    {
        configStr = R"("log")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        EventTypeCondition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL(PipelineEvent::Type::LOG, cond.mType);
    }
    {
        configStr = R"("metric")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        EventTypeCondition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL(PipelineEvent::Type::METRIC, cond.mType);
    }
    {
        configStr = R"("trace")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        EventTypeCondition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL(PipelineEvent::Type::SPAN, cond.mType);
    }
    {
        configStr = R"("unknown")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        EventTypeCondition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(true)";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        EventTypeCondition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
}

void EventTypeConditionUnittest::TestCheck() {
    Json::Value configJson;
    string errorMsg;
    string configStr = R"("log")";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    EventTypeCondition cond;
    APSARA_TEST_TRUE(cond.Init(configJson, ctx));
    {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.AddLogEvent();
        APSARA_TEST_TRUE(cond.Check(g));
    }
    {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.AddMetricEvent();
        APSARA_TEST_FALSE(cond.Check(g));
    }
}

UNIT_TEST_CASE(EventTypeConditionUnittest, TestInit)
UNIT_TEST_CASE(EventTypeConditionUnittest, TestCheck)

class TagValueConditionUnittest : public testing::Test {
public:
    void TestInit();
    void TestCheck();

private:
    PipelineContext ctx;
};

void TagValueConditionUnittest::TestInit() {
    Json::Value configJson;
    string configStr, errorMsg;
    {
        configStr = R"("INFO")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        TagValueCondition cond;
        APSARA_TEST_TRUE(cond.Init(configJson, ctx));
        APSARA_TEST_EQUAL("INFO", cond.mContent);
    }
    {
        configStr = R"("")";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        TagValueCondition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
    {
        configStr = R"(true)";
        APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
        TagValueCondition cond;
        APSARA_TEST_FALSE(cond.Init(configJson, ctx));
    }
}

void TagValueConditionUnittest::TestCheck() {
    Json::Value configJson;
    string errorMsg;
    string configStr = R"("INFO")";
    APSARA_TEST_TRUE(ParseJsonTable(configStr, configJson, errorMsg));
    TagValueCondition cond;
    APSARA_TEST_TRUE(cond.Init(configJson, ctx));
    {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.SetTag(TagValueCondition::sTagKey, "INFO");
        APSARA_TEST_TRUE(cond.Check(g));
    }
    {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        g.SetTag(TagValueCondition::sTagKey, "ERROR");
        APSARA_TEST_FALSE(cond.Check(g));
    }
    {
        PipelineEventGroup g(make_shared<SourceBuffer>());
        APSARA_TEST_FALSE(cond.Check(g));
    }
}

UNIT_TEST_CASE(TagValueConditionUnittest, TestInit)
UNIT_TEST_CASE(TagValueConditionUnittest, TestCheck)

} // namespace logtail

UNIT_TEST_MAIN
