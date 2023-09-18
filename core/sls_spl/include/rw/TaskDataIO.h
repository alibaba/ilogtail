#pragma once

#include <velox/common/memory/MemoryPool.h>
#include <velox/vector/ComplexVector.h>
#include "IO.h"
#include "IO.h"
#include "task/TaskData.h"
#include "util/Type.h"

namespace apsara::sls::spl {

class TaskDataIO {
   public:
    TaskDataIO() {}
    ~TaskDataIO() {}

    static void readFromInput(
        const TaskContext& taskCtx,
        const InputPtr& inputPtr,
        TaskData& result,
        Error& err);

    static void writeToOutput(
        const TaskContext& taskCtx,
        const TaskData& input,
        const OutputPtr& outputPtr,
        Error& err);
};

}  // namespace apsara::sls::spl