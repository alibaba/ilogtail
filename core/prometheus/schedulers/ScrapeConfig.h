#pragma once

#include <json/value.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "prometheus/labels/Relabel.h"


namespace logtail {

class ScrapeConfig {
public:
    std::string mJobName;
    int64_t mScrapeIntervalSeconds;
    int64_t mScrapeTimeoutSeconds;
    std::string mMetricsPath;
    bool mHonorLabels;
    std::string mScheme;

    std::map<std::string, std::string> mAuthHeaders;

    uint64_t mMaxScrapeSizeBytes;
    uint64_t mSampleLimit;
    uint64_t mSeriesLimit;
    RelabelConfigList mRelabelConfigs;
    RelabelConfigList mMetricRelabelConfigs;

    std::map<std::string, std::vector<std::string>> mParams;

    std::string mQueryString;

    ScrapeConfig();
    bool Init(const Json::Value& config);
    bool InitStaticConfig(const Json::Value& config);

private:
    bool InitBasicAuth(const Json::Value& basicAuth);
    bool InitAuthorization(const Json::Value& authorization);

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ScrapeConfigUnittest;
#endif
};

} // namespace logtail