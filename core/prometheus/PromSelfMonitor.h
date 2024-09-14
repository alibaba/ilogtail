#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/LogtailMetric.h"
#include "monitor/PluginMetricManager.h"

namespace logtail {

class PromSelfMonitor {
public:
    PromSelfMonitor();

    void InitMetricManager(const std::string& key,
                           const std::unordered_map<std::string, MetricType>& metricKeys,
                           MetricLabels labels);

    void CounterAdd(const std::string& key, const std::string& metricName, uint64_t status, uint64_t val = 1);

    void IntGaugeSet(const std::string& key, const std::string& metricName, uint64_t status, uint64_t value);

private:
    ReentrantMetricsRecordRef GetOrCreateReentrantMetricsRecordRef(const std::string& key, uint64_t status);

    std::map<std::string, PluginMetricManagerPtr> mPromMetricsMap;
    std::map<std::string, std::map<uint64_t, ReentrantMetricsRecordRef>> mPromStatusMap;
    MetricLabelsPtr mDefaultLabels;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PromSelfMonitorUnittest;
#endif
};

} // namespace logtail