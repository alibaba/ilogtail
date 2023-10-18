// Copyright 2023 iLogtail Authors
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

#include "pipeline/GlobalConfig.h"

#include "json/json.h"

#include "common/LogstoreFeedbackQueue.h"
#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {

const std::unordered_set<std::string> GlobalConfig::sNativeParam
    = {"TopicType", "TopicFormat", "ProcessPriority", "EnableTimestampNanosecond", "UsingOldContentTag"};

bool GlobalConfig::Init(const Json::Value& config, const std::string& configName) {
    string errorMsg;

    // TopicType
    string topicType;
    if (!GetOptionalStringParam(config, "TopicType", topicType, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, "global", configName);
    } else if (topicType == "custom") {
        mTopicType = TopicType::CUSTOM;
    } else if (topicType == "machine_group_topic") {
        mTopicType = TopicType::MACHINE_GROUP_TOPIC;
    } else if (topicType == "file_path") {
        mTopicType = TopicType::FILEPATH;
    } else if (!topicType.empty()) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, "global", configName);
    }

    // TopicFormat
    if (mTopicType == TopicType::CUSTOM || mTopicType == TopicType::MACHINE_GROUP_TOPIC
        || mTopicType == TopicType::FILEPATH) {
        if (!GetMandatoryStringParam(config, "TopicFormat", mTopicFormat, errorMsg)) {
            mTopicType = TopicType::NONE;
            LOG_WARNING(
                sLogger,
                ("problem encountered during pipeline initialization", "param TopicFormat is not valid")(
                    "action", "ignore param TopicType and TopicFormat")("module", "global")("config", configName));
        } else if (mTopicType == TopicType::FILEPATH && !IsRegexValid(mTopicFormat)) {
            mTopicType = TopicType::NONE;
            mTopicFormat.clear();
            LOG_WARNING(
                sLogger,
                ("problem encountered during pipeline initialization", "param TopicFormat is not valid")(
                    "action", "ignore param TopicType and TopicFormat")("module", "global")("config", configName));
        }
    }

    // ProcessPriority
    uint32_t priority = 0;
    if (!GetOptionalUIntParam(config, "ProcessPriority", priority, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, "global", configName);
    } else if (priority > MAX_CONFIG_PRIORITY_LEVEL) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, "global", configName);
    } else {
        mProcessPriority = priority;
    }
    // if (mProcessPriority > 0) {
    //     LogProcess::GetInstance()->SetPriorityWithHoldOn(config->mLogstoreKey, mProcessPriority);
    // } else {
    //     LogProcess::GetInstance()->DeletePriorityWithHoldOn(config->mLogstoreKey);
    // }

    // EnableTimestampNanosecond
    if (!GetOptionalBoolParam(config, "EnableTimestampNanosecond", mEnableTimestampNanosecond, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, "global", configName);
    }

    // UsingOldContentTag
    if (!GetOptionalBoolParam(config, "UsingOldContentTag", mUsingOldContentTag, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, "global", configName);
    }

    // generate Go Global module if necessary
    // if (mContext->IsFlushingThroughGoPipeline()) {
    //     Json::Value global(Json::objectValue);
    //     for (auto itr = config.begin(); itr != config.end(); ++itr) {
    //         if (sNativeParam.find(itr.name()) != sNativeParam.end()) {
    //             global[itr.name()] = *itr;
    //         }
    //     }
    // }
}

} // namespace logtail
