#pragma once

#include "pipeline/PipelineContext.h"

namespace logtail{
namespace ebpf {

class AbstractHandler {
public:
    AbstractHandler() {}
    AbstractHandler(logtail::PipelineContext* ctx, uint32_t idx) : ctx_(ctx), plugin_idx_(idx) {}
    // context
    void update_context(const logtail::PipelineContext* ctx, uint32_t index) { 
        ctx_ = ctx; 
        plugin_idx_ = index;
    }
protected:
    const logtail::PipelineContext* ctx_ = nullptr;
    uint64_t process_total_count_ = 0;
    uint32_t plugin_idx_ = 0;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class eBPFServerUnittest;
#endif
};

}
}
