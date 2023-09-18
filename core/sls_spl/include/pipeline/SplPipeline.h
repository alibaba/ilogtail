#pragma once

#include "Error.h"
#include "Stats.h"
#include "logger/Logger.h"
#include "plan/SplPlan.h"
#include "rw/IO.h"

namespace apsara::sls::spl {

void doInitVelox();

void initSPL();
class SplPipeline {
   public:
    SplPipeline(
        const std::string& splPlan,
        Error& err,
        const uint64_t timeoutMills = 100,
        const int64_t maxMemoryBytes = 1024 * 1024 * 1024,
        const LoggerPtr& logger = nullptr);
    std::vector<std::string> getOutputLabels();
    std::vector<std::string> getInputSearches();
    StatusCode execute(
        const std::vector<InputPtr>& inputs,
        const std::vector<OutputPtr>& outputs,
        std::string* errorMsg = nullptr,
        const PipelineStatsPtr& pipelineStatsPtr = nullptr);
    ~SplPipeline();

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

using SplPipelinePtr = std::shared_ptr<SplPipeline>;

}  // namespace apsara::sls::spl