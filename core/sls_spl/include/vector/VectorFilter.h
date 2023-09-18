#pragma once

#include <velox/common/base/VeloxException.h>
#include <velox/common/memory/Memory.h>
#include <velox/core/Expressions.h>
#include <velox/exec/OperatorUtils.h>
#include <velox/expression/Expr.h>
#include <velox/parse/Expressions.h>
#include <velox/parse/ExpressionsParser.h>
#include <velox/parse/TypeResolver.h>
#include <velox/vector/BaseVector.h>
#include "util/Util.h"

namespace apsara::sls::spl {

facebook::velox::RowVectorPtr filterRowVector(
    facebook::velox::exec::ExprSet& exprSet,
    facebook::velox::core::ExecCtx& execCtx,
    facebook::velox::RowVectorPtr input,
    facebook::velox::RowVectorPtr filterInput,
    MemoryPoolPtr allocatePool,
    BufferPtr& selectedIndices);

bool isSequence(
    const facebook::velox::vector_size_t* numbers,
    const facebook::velox::vector_size_t start,
    const facebook::velox::vector_size_t end);

facebook::velox::VectorPtr wrapChild(
    facebook::velox::vector_size_t size,
    facebook::velox::BufferPtr mapping,
    const facebook::velox::VectorPtr& child,
    facebook::velox::BufferPtr nulls = nullptr);

facebook::velox::RowVectorPtr fillOutput(
    facebook::velox::memory::MemoryPool* pool,
    facebook::velox::RowVectorPtr input_,
    facebook::velox::vector_size_t size,
    facebook::velox::BufferPtr mapping);

}  // namespace apsara::sls::spl