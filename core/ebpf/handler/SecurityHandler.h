#pragma once

#include <vector>
#include <string>

#include "ebpf/handler/AbstractHandler.h"
#include "ebpf/include/export.h"

namespace logtail {
namespace ebpf {

class SecurityHandler : public AbstractHandler {
public:
    SecurityHandler(logtail::PipelineContext* ctx, uint32_t idx);
    void handle(std::vector<std::unique_ptr<AbstractSecurityEvent>>&& events);
private:
    std::string host_ip_;
    std::string host_name_;
};

}
}