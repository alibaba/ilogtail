#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/LoongCollectorMetricTypes.h"
#include "prometheus/Constants.h"
using namespace std;

namespace logtail {
bool PromSelfMonitor::Init(const std::string& mPodName, const std::string& mOperatorHost) {
    mDefaultLabels = std::make_shared<MetricLabels>();
    mDefaultLabels->emplace_back(prometheus::POD_NAME, mPodName);
    mDefaultLabels->emplace_back(prometheus::OPERATOR_HOST, mOperatorHost);
    return true;
}
void PromSelfMonitor::InitMetricManager(const std::string& key,
                                        std::unordered_map<std::string, MetricType> metricKeys) {
    if (!mPromMetricsMap.count(key)) {
        mPromMetricsMap[key] = std::make_shared<PluginMetricManager>(mDefaultLabels, metricKeys);
    }
}

void PromSelfMonitor::CounterAdd(const std::string& key,
                                 const std::string& metricName,
                                 const std::map<std::string, std::string>& labels,
                                 uint64_t val) {
    auto metricLabels = MetricLabels(labels.begin(), labels.end());
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, metricLabels);
    recordRef->GetCounter(metricName)->Add(val);
}

void PromSelfMonitor::IntGaugeSet(const std::string& key,
                                  const std::string& metricName,
                                  const std::map<std::string, std::string>& labels,
                                  uint64_t value) {
    auto metricLabels = MetricLabels(labels.begin(), labels.end());
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, metricLabels);
    recordRef->GetIntGauge(metricName)->Set(value);
}

ReentrantMetricsRecordRef PromSelfMonitor::GetOrCreateReentrantMetricsRecordRef(const std::string& key,
                                                                                MetricLabels& labels) {
    if (!mPromMetricsMap.count(key)) {
        return nullptr;
    }
    return mPromMetricsMap[key]->GetOrCreateReentrantMetricsRecordRef(labels);
}
} // namespace logtail