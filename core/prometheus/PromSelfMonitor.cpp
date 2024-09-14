#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "common/StringTools.h"
#include "monitor/LoongCollectorMetricTypes.h"
#include "prometheus/Constants.h"
using namespace std;

namespace logtail {

PromSelfMonitor::PromSelfMonitor() {
}

void PromSelfMonitor::InitMetricManager(const std::string& key,
                                        const std::unordered_map<std::string, MetricType>& metricKeys,
                                        MetricLabels labels) {
    if (!mPromMetricsMap.count(key)) {
        mPromMetricsMap[key] = std::make_shared<PluginMetricManager>(labels, metricKeys);
    }
}

void PromSelfMonitor::CounterAdd(const std::string& key, const std::string& metricName, uint64_t status, uint64_t val) {
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, status);
    if (recordRef) {
        recordRef->GetCounter(metricName)->Add(val);
    }
}

void PromSelfMonitor::IntGaugeSet(const std::string& key,
                                  const std::string& metricName,
                                  uint64_t status,
                                  uint64_t value) {
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, status);
    if (recordRef) {
        recordRef->GetIntGauge(metricName)->Set(value);
    }
}

ReentrantMetricsRecordRef PromSelfMonitor::GetOrCreateReentrantMetricsRecordRef(const std::string& key,
                                                                                uint64_t status) {
    if (!mPromMetricsMap.count(key)) {
        return nullptr;
    }
    if (!mPromStatusMap[key].count(status)) {
        mPromStatusMap[key][status]
            = mPromMetricsMap[key]->GetOrCreateReentrantMetricsRecordRef({{prometheus::STATUS, ToString(status)}});
    }
    return mPromStatusMap[key][status];
}
} // namespace logtail