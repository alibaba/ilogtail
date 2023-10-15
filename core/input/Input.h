#pragma once

#include "table/Table.h"
#include "pipeline/PipelineContext.h"

namespace logtail {
class Input {
public:
    virtual ~Input() = default;

    virtual bool Init(const Table& config) = 0;
    virtual bool Start() = 0;
    virtual bool Stop(bool isPipelineRemoving) = 0;
    void SetContext(PipelineContext& context) { mContext = &context; }
    PipelineContext& GetContext() { return *mContext; }

protected:
    PipelineContext* mContext = nullptr;
};
} // namespace logtail
