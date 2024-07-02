#ifndef ARGUS_PROMETHEUS_METRIC_H
#define ARGUS_PROMETHEUS_METRIC_H

#include "CommonMetric.h"
#include <vector>

namespace common {
    class PrometheusMetric {
    public:
        static int ParseMetrics(const std::string &metricsStr, std::vector<CommonMetric> &metrics);
        static int ParseMetric(const std::string &lineStr, CommonMetric &commonMetric);
        static std::string HandleEscape(const std::string &valueStr);
        static std::string MetricToLine(const CommonMetric &metric, bool needTimestamp = true);
    };
}

#define _PromEsc(s) ::common::PrometheusMetric::HandleEscape(s)

#endif
