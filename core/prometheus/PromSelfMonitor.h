#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/LogtailMetric.h"
#include "monitor/PluginMetricManager.h"

namespace logtail {

// manage status metrics
class PromSelfMonitor {
public:
    PromSelfMonitor() = default;

    void InitMetricManager(const std::unordered_map<std::string, MetricType>& metricKeys, const MetricLabels& labels);

    void CounterAdd(const std::string& metricName, uint64_t status, uint64_t val = 1);

    void IntGaugeSet(const std::string& metricName, uint64_t status, uint64_t value);

private:
    ReentrantMetricsRecordRef GetOrCreateReentrantMetricsRecordRef(uint64_t status);

    PluginMetricManagerPtr mPluginMetricManagerPtr;
    std::map<uint64_t, ReentrantMetricsRecordRef> mPromStatusMap;
    std::map<std::string, std::map<uint64_t, CounterPtr>> mMetricsCounterMap;
    std::map<std::string, std::map<uint64_t, IntGaugePtr>> mMetricsIntGaugeMap;
    MetricLabelsPtr mDefaultLabels;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PromSelfMonitorUnittest;
#endif
};

} // namespace logtail