#include "pipeline/Global.h"

#include "json/json.h"

#include "common/LogstoreFeedbackQueue.h"
#include "common/ParamExtractor.h"

using namespace std;

namespace logtail {
bool Global::Init(const Table& config) {
    Json::Value config1;
    string errorMsg;

    // TopicType
    string topicType;
    if (!GetOptionalStringParam(config1, "TopicType", topicType, errorMsg)) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, "global", "");
    }
    if (topicType == "custom") {
        mTopicType = TopicType::CUSTOM;
    } else if (topicType == "machine_group_topic") {
        mTopicType = TopicType::MACHINE_GROUP_TOPIC;
    } else if (topicType == "file_path") {
        mTopicType = TopicType::FILEPATH;
    } else if (!topicType.empty()) {
        PARAM_WARNING_IGNORE(sLogger, errorMsg, "global", "");
    }

    // TopicFormat
    if (mTopicType == TopicType::CUSTOM || mTopicType == TopicType::MACHINE_GROUP_TOPIC || mTopicType == TopicType::FILEPATH) {
        if (!GetMandatoryStringParam(config1, "TopicFormat", mTopicFormat, errorMsg)) {
            mTopicType = TopicType::NONE;
            LOG_WARNING(sLogger,
                        ("problem encountered during pipeline initialization", "param TopicFormat is not valid")(
                            "action", "ignore param TopicType and TopicFormat")("module", "plugin")("config", ""));
        }
        if (mTopicType == TopicType::FILEPATH && !IsRegexValid(mTopicFormat)) {
            mTopicType = TopicType::NONE;
            mTopicFormat.clear();
            LOG_WARNING(sLogger,
                        ("problem encountered during pipeline initialization", "param TopicFormat is not valid")(
                            "action", "ignore param TopicType and TopicFormat")("module", "plugin")("config", ""));
        }
    }

    // ProcessPriority
    if (!GetOptionalUIntParam(config1, "ProcessPriority", mProcessPriority, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, "plugin", "");
    }
    if (mProcessPriority > MAX_CONFIG_PRIORITY_LEVEL) {
        mProcessPriority = 0;
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, "plugin", "");
    }

    // EnableTimestampNanosecond
    if (!GetOptionalBoolParam(config1, "EnableTimestampNanosecond", mEnableTimestampNanosecond, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, "plugin", "");
    }

    // UsingOldContentTag
    if (!GetOptionalBoolParam(config1, "UsingOldContentTag", mUsingOldContentTag, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, false, "plugin", "");
    }
}
} // namespace logtail
