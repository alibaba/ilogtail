#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/MetricTypes.h"
#include "monitor/metric_constants/MetricConstants.h"
using namespace std;

namespace logtail {

void PromSelfMonitorUnsafe::InitMetricManager(const std::unordered_map<std::string, MetricType>& metricKeys,
                                        const MetricLabels& labels) {
    auto metricLabels = std::make_shared<MetricLabels>(labels);
    mPluginMetricManagerPtr = std::make_shared<PluginMetricManager>(metricLabels, metricKeys, MetricCategory::METRIC_CATEGORY_PLUGIN_SOURCE);
}

void PromSelfMonitorUnsafe::AddCounter(const std::string& metricName, uint64_t statusCode, uint64_t val) {
    auto& status = StatusToString(statusCode);
    if (!mMetricsCounterMap.count(metricName) || !mMetricsCounterMap[metricName].count(status)) {
        mMetricsCounterMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetCounter(metricName);
    }
    mMetricsCounterMap[metricName][status]->Add(val);
}

void PromSelfMonitorUnsafe::SetIntGauge(const std::string& metricName, uint64_t statusCode, uint64_t value) {
    auto& status = StatusToString(statusCode);
    if (!mMetricsIntGaugeMap.count(metricName) || !mMetricsIntGaugeMap[metricName].count(status)) {
        mMetricsIntGaugeMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetIntGauge(metricName);
    }
    mMetricsIntGaugeMap[metricName][status]->Set(value);
}

ReentrantMetricsRecordRef PromSelfMonitorUnsafe::GetOrCreateReentrantMetricsRecordRef(const std::string& status) {
    if (mPluginMetricManagerPtr == nullptr) {
        return nullptr;
    }
    if (!mPromStatusMap.count(status)) {
        mPromStatusMap[status]
            = mPluginMetricManagerPtr->GetOrCreateReentrantMetricsRecordRef({{METRIC_LABEL_KEY_STATUS, status}});
    }
    return mPromStatusMap[status];
}
} // namespace logtail