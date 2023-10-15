#pragma once

#include <cstdint>
#include <string>

#include "table/Table.h"

namespace logtail {
struct Global {
    enum class TopicType {NONE, FILEPATH, MACHINE_GROUP_TOPIC, CUSTOM};

    bool Init(const Table& config);

    TopicType mTopicType = TopicType::NONE;
    std::string mTopicFormat;
    uint32_t mProcessPriority = 0;
    bool mEnableTimestampNanosecond = false;
    bool mUsingOldContentTag = false;
};
} // namespace logtail
