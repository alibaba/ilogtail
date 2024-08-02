/*
 * Copyright 2024 iLogtail Authors
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

#include <json/json.h>

#include <variant>

#include "models/PipelineEventGroup.h"
#include "pipeline/PipelineContext.h"

namespace logtail {

class EventTypeCondition {
public:
    bool Init(const Json::Value& config, const PipelineContext& ctx);
    bool Check(const PipelineEventGroup& g) const;

private:
    PipelineEvent::Type mType;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class EventTypeConditionUnittest;
#endif
};

class TagValueCondition {
public:
    bool Init(const Json::Value& config, const PipelineContext& ctx);
    bool Check(const PipelineEventGroup& g) const;

private:
    static std::string sTagKey;

    std::string mContent;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class TagValueConditionUnittest;
    friend class ConditionUnittest;
#endif
};

class Condition {
public:
    bool Init(const Json::Value& config, const PipelineContext& ctx);
    bool Check(const PipelineEventGroup& g) const;

private:
    enum class Type { EVENT_TYPE, TAG_VALUE };

    Type mType;
    std::variant<EventTypeCondition, TagValueCondition> mDetail;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ConditionUnittest;
#endif
};

} // namespace logtail
