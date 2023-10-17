#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>

// #include "table/Table.h"
#include "json/json.h"

namespace logtail {
struct GlobalConfig {
    enum class TopicType {NONE, FILEPATH, MACHINE_GROUP_TOPIC, CUSTOM};

    static const std::unordered_set<std::string> sNativeParam;

    // bool Init(const Table& config, const std::string& configName);
    bool Init(const Json::Value& config, const std::string& configName);

    TopicType mTopicType = TopicType::NONE;
    std::string mTopicFormat;
    uint32_t mProcessPriority = 0;
    bool mEnableTimestampNanosecond = false;
    bool mUsingOldContentTag = false;
};
} // namespace logtail
