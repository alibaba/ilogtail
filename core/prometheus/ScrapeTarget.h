#pragma once

#include <cstdint>
#include <string>

#include "prometheus/Labels.h"
namespace logtail {
class ScrapeTarget {
public:
    ScrapeTarget();
    ScrapeTarget(const Labels& labels);
    ScrapeTarget(const ScrapeTarget& other);
    ScrapeTarget& operator=(const ScrapeTarget& other);

    Labels mLabels;

    // target info
    std::string mHost;
    uint32_t mPort;

    std::string GetHash();
};
} // namespace logtail