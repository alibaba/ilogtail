#pragma once

#include <velox/common/memory/Memory.h>
#include <velox/vector/ComplexVector.h>
#include <memory>
#include <unordered_map>
#include "Stats.h"
#include "Result.h"
#include "logger/Logger.h"
#include "pipeline/Error.h"
#include "plan/SplPlan.h"
#include "rw/IO.h"
#include "rw/IO.h"
#include "task/Task.h"
#include "task/TaskContext.h"
#include "util/Type.h"

namespace apsara::sls::spl {

class PipelineFactory;
class SplPipeline;

constexpr int64_t pipelineMinMemoryBytes = 512;

class Pipeline {
   private:
    SplPlanPtr splPlanPtr_;
    LoggerPtr logger_;
    std::unique_ptr<memory::MemoryManager> memoryManager_;
    TaskContext taskCtx_;
    std::vector<uint32_t> orderedTaskIds_;
    std::unordered_map<uint32_t, TaskPtr> tasks_;
    std::unordered_map<uint32_t, std::vector<uint32_t>> nodeDependentIds_;
    std::unordered_map<uint32_t, bool> fastFailTaskIds_;
    uint64_t timeoutMills_;
    int64_t maxMemoryBytes_;
    bool alwaysFalse_;
    bool alwaysTrue_;
    VectorBuilderPtr vbPtr_;

    std::vector<TaskPtr> getDependentTasks(const uint32_t taskId);
    void executeTask(
        const TaskContext& taskCtx,
        const uint32_t taskId,
        const std::vector<TaskData>& inputData,
        TaskResults& taskResults,
        TaskStats& taskStats_,
        Error& err);

    bool initMemoryPool(Error& err);
    bool initTasks(Error& err);
    void initTaskGraph();
    std::vector<int32_t> getResultTaskIds();

   public:
    void executeInternal(
        const TaskContext& taskCtx,
        const std::vector<TaskData>& inputData,
        PipelineResult& pipelineResult,
        PipelineStats& pipelineStats);

    std::vector<std::string> getOutputLabels();
    std::vector<std::string> getInputSearches();
    StatusCode execute(
        const std::vector<InputPtr>& inputReaders,
        const std::vector<OutputPtr>& outputWriters,
        std::string* errorMsg = nullptr,
        const PipelineStatsPtr& pipelineStatsPtr = nullptr);

    Pipeline(
        const SplPlanPtr splPlanPtr,
        const LoggerPtr& logger,
        const uint64_t timeoutMills = 100,
        const int64_t maxMemoryBytes = 1024 * 1024 * 1024);
    ~Pipeline();
    void printPool();
    void printPool(const TaskContext& taskCtx);

    friend class PipelineFactory;
    friend class SplPipeline;
};

using PipelinePtr = std::shared_ptr<Pipeline>;

}  // namespace apsara::sls::spl