#pragma once

#include <json/value.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "Relabel.h"


namespace logtail {
class ScrapeConfig {
public:
    std::string mJobName;
    std::string mScheme;
    std::string mMetricsPath;
    // in seconds
    int64_t mScrapeInterval;
    // in seconds
    int64_t mScrapeTimeout;
    // in bytes
    int64_t mMaxScrapeSize;
    int64_t mSampleLimit;
    int64_t mSeriesLimit;
    std::vector<RelabelConfig> mRelabelConfigs;

    std::map<std::string, std::vector<std::string>> mParams;
    std::map<std::string, std::string> mHeaders;

    ScrapeConfig();
    bool Init(const Json::Value& config);

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeConfigUnittest;
#endif
};
} // namespace logtail