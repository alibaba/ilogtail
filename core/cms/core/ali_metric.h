#ifndef ARGUS_ALI_METRIC_H
#define ARGUS_ALI_METRIC_H

#include <set>
#include "base_parse_metric.h"

namespace argus {

#include "common/test_support"
class AliMetric : public BaseParseMetric {
public:
    AliMetric(const std::vector<MetricFilterInfo> &metricFilterInfos, const std::vector<LabelAddInfo> &labelAddInfos);
    explicit AliMetric(const std::vector<LabelAddInfo> &labelAddInfos);
    ParseErrorType AddMetric(const std::string &metricStr) override;
private:
    static void buildMap(const std::vector<MetricFilterInfo> &metricFilterInfos,
                         std::unordered_map<std::string, std::set<std::string>> &m);
private:
    std::unordered_map<std::string, int64_t> mLastMetricMap;
    std::unordered_map<std::string, std::set<std::string>> mMetricMap;
};
#include "common/test_support"

}
#endif