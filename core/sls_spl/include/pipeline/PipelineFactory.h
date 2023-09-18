#pragma once

#include "Pipeline.h"
#include "plan/SplPlan.h"
#include "task/Task.h"

namespace apsara::sls::spl {

class PipelineFactory {
   public:
    PipelineFactory() {}
    ~PipelineFactory() {}
    static PipelinePtr create(
        const std::string& splPlan,
        Error& err,
        const uint64_t timeoutMills = 100,
        const int64_t maxMemoryBytes = 1024 * 1024 * 1024,
        const LoggerPtr& logger=nullptr);
};

}  // namespace apsara::sls::spl