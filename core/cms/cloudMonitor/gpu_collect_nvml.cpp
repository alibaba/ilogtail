//
// Created by 韩呈杰 on 2024/1/24.
//
#include "gpu_collect_nvml.h"

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// INvml
INvml::~INvml() = default;

/// @brief 
namespace cloudMonitor {
    // nvml (1|2)
    int toolCollectNvml(const char *, int argc, const char *const *argv, std::ostream &os) {
        auto nvml = NewNvml(LEVEL_ERROR);
        os << (nvml ? nvml->CollectToXml(argc > 1 ? convert<int>(argv[1]) : 1) : "");
        return 0;
    }
}
