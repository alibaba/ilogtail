#pragma once

#include <vector>

#include "ebpf/handler/AbstractHandler.h"
#include "ebpf/include/export.h"

namespace logtail {
namespace ebpf {

class MeterHandler : public AbstractHandler {
public:
    MeterHandler(logtail::PipelineContext* ctx, uint32_t idx) : AbstractHandler(ctx, idx) {}

    virtual void handle(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&&, uint64_t) = 0;
};

class ArmsMeterHandler : public MeterHandler {
public:
    ArmsMeterHandler(PipelineContext* ctx, uint32_t idx) : MeterHandler(ctx, idx) {}
    void handle(std::vector<std::unique_ptr<ApplicationBatchMeasure>>&& measures, uint64_t timestamp) override;
};

class SpanHandler : public AbstractHandler {
public:
    SpanHandler(logtail::PipelineContext* ctx, uint32_t idx) : AbstractHandler(ctx, idx) {}
    virtual void handle(std::vector<std::unique_ptr<ApplicationBatchSpan>>&&) = 0;
};

class ArmsSpanHandler : public SpanHandler {
public:
    ArmsSpanHandler(PipelineContext* ctx, uint32_t idx) : SpanHandler(ctx, idx) {}
    void handle(std::vector<std::unique_ptr<ApplicationBatchSpan>>&&) override;
};

}
}