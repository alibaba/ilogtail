#pragma once
#include <string>
#include <unordered_map>

namespace logtail {


struct BaseMetric {
    /* counters and gauges */
    uint64_t val;

    /* internal */
    uint64_t timestamp;
    std::unordered_map<std::string, std::string> labels;
};


void baseMetricAdd(BaseMetric *metric, uint64_t timestamp, uint64_t val);

}