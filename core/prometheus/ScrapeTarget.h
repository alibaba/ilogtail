#pragma once

#include <cstdint>
#include <string>

#include "prometheus/Labels.h"
namespace logtail {
class ScrapeTarget {
public:
    ScrapeTarget();
    explicit ScrapeTarget(const Labels& labels);
    ScrapeTarget(const ScrapeTarget& other) = default;
    ScrapeTarget(ScrapeTarget&& other) = default;
    ScrapeTarget& operator=(const ScrapeTarget& other) = default;
    ScrapeTarget& operator=(ScrapeTarget&& other) = default;
    ~ScrapeTarget() = default;

    Labels mLabels;

    std::string mHost;
    int32_t mPort;

    std::string GetHash();
};
} // namespace logtail