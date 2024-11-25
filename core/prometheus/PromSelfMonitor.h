#pragma once

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/PluginMetricManager.h"

namespace logtail {

// manage status metrics, thread unsafe
class PromSelfMonitorUnsafe {
public:
    PromSelfMonitorUnsafe() = default;

    void InitMetricManager(const std::unordered_map<std::string, MetricType>& metricKeys, const MetricLabels& labels);

    void AddCounter(const std::string& metricName, const std::string& status, uint64_t val = 1);

    void SetIntGauge(const std::string& metricName, const std::string& status, uint64_t value);

    static std::string StatusToString(uint64_t status);
    static std::string CurlCodeToString(uint64_t code);

private:
    ReentrantMetricsRecordRef GetOrCreateReentrantMetricsRecordRef(const std::string& status);


    PluginMetricManagerPtr mPluginMetricManagerPtr;
    std::map<std::string, ReentrantMetricsRecordRef> mPromStatusMap;
    std::map<std::string, std::map<std::string, CounterPtr>> mMetricsCounterMap;
    std::map<std::string, std::map<std::string, IntGaugePtr>> mMetricsIntGaugeMap;
    MetricLabelsPtr mDefaultLabels;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class PromSelfMonitorUnittest;
#endif
};

} // namespace logtail