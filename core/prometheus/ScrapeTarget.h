#pragma once

#include "Labels.h"
namespace logtail {
class ScrapeTarget {
public:
    ScrapeTarget(const Labels& labels);

    Labels mLabels;

    // target info
    std::string mHost;
    uint32_t mPort;
};
} // namespace logtail