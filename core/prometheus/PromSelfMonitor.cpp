#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "common/StringTools.h"
#include "monitor/LoongCollectorMetricTypes.h"
#include "prometheus/Constants.h"
using namespace std;

namespace logtail {

void PromSelfMonitor::InitMetricManager(const std::unordered_map<std::string, MetricType>& metricKeys,
                                        const MetricLabels& labels) {
    auto metricLabels = std::make_shared<MetricLabels>(labels);
    mPluginMetricManagerPtr = std::make_shared<PluginMetricManager>(metricLabels, metricKeys);
}

void PromSelfMonitor::CounterAdd(const std::string& metricName, uint64_t status, uint64_t val) {
    if (!mMetricsCounterMap.count(metricName) || !mMetricsCounterMap[metricName].count(status)) {
        mMetricsCounterMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetCounter(metricName);
    }
    mMetricsCounterMap[metricName][status]->Add(val);
}

void PromSelfMonitor::IntGaugeSet(const std::string& metricName, uint64_t status, uint64_t value) {
    if (!mMetricsIntGaugeMap.count(metricName) || !mMetricsIntGaugeMap[metricName].count(status)) {
        mMetricsIntGaugeMap[metricName][status] = GetOrCreateReentrantMetricsRecordRef(status)->GetIntGauge(metricName);
    }
    mMetricsIntGaugeMap[metricName][status]->Set(value);
}

ReentrantMetricsRecordRef PromSelfMonitor::GetOrCreateReentrantMetricsRecordRef(uint64_t status) {
    if (mPluginMetricManagerPtr == nullptr) {
        return nullptr;
    }
    if (!mPromStatusMap.count(status)) {
        mPromStatusMap[status]
            = mPluginMetricManagerPtr->GetOrCreateReentrantMetricsRecordRef({{prometheus::STATUS, ToString(status)}});
    }
    return mPromStatusMap[status];
}
} // namespace logtail