#pragma once

#include "pipeline/PipelineContext.h"

namespace logtail{
namespace ebpf {

class AbstractHandler {
public:
    AbstractHandler() {}
    AbstractHandler(logtail::PipelineContext* ctx, uint32_t idx) : mCtx(ctx), mPluginIdx(idx) {}
    // context
    void update_context(const logtail::PipelineContext* ctx, uint32_t index) { 
        mCtx = ctx; 
        mPluginIdx = index;
    }
protected:
    const logtail::PipelineContext* mCtx = nullptr;
    uint64_t mProcessTotalCnt = 0;
    uint32_t mPluginIdx = 0;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

}
}
