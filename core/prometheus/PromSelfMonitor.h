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
    bool Init(const std::string& mPodName, const std::string& mOperatorHost);

    void InitMetricManager(const std::string& key,
                           const std::unordered_map<std::string, MetricType>& metricKeys,
                           MetricLabels labels);

    void
    CounterAdd(const std::string& key, const std::string& metricName, const MetricLabels& labels, uint64_t val = 1);


    void IntGaugeSet(const std::string& key, const std::string& metricName, const MetricLabels& labels, uint64_t value);

private:
    ReentrantMetricsRecordRef GetOrCreateReentrantMetricsRecordRef(const std::string& key, const MetricLabels& labels);

    std::map<std::string, PluginMetricManagerPtr> mPromMetricsMap;
    MetricLabelsPtr mDefaultLabels;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PromSelfMonitorUnittest;
#endif
};

} // namespace logtail