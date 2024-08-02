// Copyright 2024 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "route/Condition.h"

#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {

bool EventTypeCondition::Init(const Json::Value& config, const PipelineContext& ctx) {
    string errorMsg;
    if (!config.isString()) {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "param Match.Condition is not of type string",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }
    string type = config.asString();
    if (type == "log") {
        mType = PipelineEvent::Type::LOG;
    } else if (type == "metric") {
        mType = PipelineEvent::Type::METRIC;
    } else if (type == "trace") {
        mType = PipelineEvent::Type::SPAN;
    } else {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "param Match.Condition is not valid",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }
    return true;
}

bool EventTypeCondition::Check(const PipelineEventGroup& g) const {
    if (g.GetEvents().empty()) {
        return false;
    }
    return g.GetEvents()[0]->GetType() == mType;
}

string TagValueCondition::sTagKey = "route.condition";

bool TagValueCondition::Init(const Json::Value& config, const PipelineContext& ctx) {
    string errorMsg;
    if (!config.isString()) {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "param Match.Condition is not of type string",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }

    mContent = config.asString();
    if (mContent.empty()) {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "mandatory string param Match.Condition is empty",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }

    return true;
}

bool TagValueCondition::Check(const PipelineEventGroup& g) const {
    return g.GetTag(sTagKey) == mContent;
}

bool Condition::Init(const Json::Value& config, const PipelineContext& ctx) {
    string errorMsg;

    // Type
    string type;
    if (!GetMandatoryStringParam(config, "Type", type, errorMsg)) {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           errorMsg,
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    } else if (type == "event_type") {
        mType = Type::EVENT_TYPE;
    } else if (type == "tag_value") {
        mType = Type::TAG_VALUE;
    } else {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "string param Match.Type is not valid",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }

    // Condition
    const char* key = "Condition";
    const Json::Value* itr = config.find(key, key + strlen(key));
    if (itr == nullptr) {
        PARAM_ERROR_RETURN(ctx.GetLogger(),
                           ctx.GetAlarm(),
                           "mandatory param Match.Condition is missing",
                           noModule,
                           ctx.GetConfigName(),
                           ctx.GetProjectName(),
                           ctx.GetLogstoreName(),
                           ctx.GetRegion());
    }
    switch (mType) {
        case Type::EVENT_TYPE:
            if (!mDetail.emplace<EventTypeCondition>().Init(*itr, ctx)) {
                return false;
            }
            break;
        case Type::TAG_VALUE:
            if (!mDetail.emplace<TagValueCondition>().Init(*itr, ctx)) {
                return false;
            }
            break;
        default:
            return false;
    }

    return true;
}

bool Condition::Check(const PipelineEventGroup& g) const {
    switch (mType) {
        case Type::EVENT_TYPE:
            return get_if<EventTypeCondition>(&mDetail)->Check(g);
        case Type::TAG_VALUE:
            return get_if<TagValueCondition>(&mDetail)->Check(g);
        default:
            return false;
    }
}

} // namespace logtail
