#pragma once

#include <memory>
#include <json.hpp>
#include <unordered_map>
#include "Error.h"

namespace apsara::sls::spl {

struct TaskStats {
    int32_t inputRows_{0};
    uint64_t inputBytes_{0};
    int32_t outputRows_{0};
    uint64_t outputBytes_{0};
    int64_t durationMicros_{0};
    // mem
    int64_t memPeakBytes_;
};
void to_json(nlohmann::json& j, const TaskStats& p);
std::ostream& operator<<(std::ostream& os, const TaskStats& p);

struct PipelineStats {
    // task
    int32_t totalTaskCount_{0};
    int32_t succTaskCount_{0};
    int32_t failTaskCount_{0};
    int64_t processMicros_{0};
    int64_t inputMicros_{0};
    int64_t outputMicros_{0};
    int64_t memPeakBytes_{0};
    std::vector<TaskStats> taskStats_;
};

void to_json(nlohmann::json& j, const PipelineStats& p);
std::ostream& operator<<(std::ostream& os, const PipelineStats& p);

using PipelineStatsPtr = std::shared_ptr<PipelineStats>;

void copyToPipelineStatsPtr(const PipelineStatsPtr& ptr, const PipelineStats& pipelineStats);

}  // namespace apsara::sls::spl