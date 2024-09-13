#include "prometheus/PromSelfMonitor.h"

#include <map>
#include <string>
#include <unordered_map>

#include "monitor/LoongCollectorMetricTypes.h"
#include "prometheus/Constants.h"
using namespace std;

namespace logtail {

PromSelfMonitor::PromSelfMonitor() {
    mDefaultLabels = std::make_shared<MetricLabels>();
}

bool PromSelfMonitor::Init(const std::string& mPodName, const std::string& mOperatorHost) {
    mDefaultLabels->emplace_back(prometheus::POD_NAME, mPodName);
    mDefaultLabels->emplace_back(prometheus::OPERATOR_HOST, mOperatorHost);
    return true;
}

void PromSelfMonitor::InitMetricManager(const std::string& key,
                                        const std::unordered_map<std::string, MetricType>& metricKeys,
                                        MetricLabels labels) {
    if (!mPromMetricsMap.count(key)) {
        auto defaultLabels = std::make_shared<MetricLabels>(*mDefaultLabels);
        defaultLabels->insert(defaultLabels->end(), labels.begin(), labels.end());

        mPromMetricsMap[key] = std::make_shared<PluginMetricManager>(defaultLabels, metricKeys);
    }
}

void PromSelfMonitor::CounterAdd(const std::string& key,
                                 const std::string& metricName,
                                 const MetricLabels& labels,
                                 uint64_t val) {
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, labels);
    recordRef->GetCounter(metricName)->Add(val);
}

void PromSelfMonitor::IntGaugeSet(const std::string& key,
                                  const std::string& metricName,
                                  const MetricLabels& labels,
                                  uint64_t value) {
    auto recordRef = GetOrCreateReentrantMetricsRecordRef(key, labels);
    recordRef->GetIntGauge(metricName)->Set(value);
}

ReentrantMetricsRecordRef PromSelfMonitor::GetOrCreateReentrantMetricsRecordRef(const std::string& key,
                                                                                const MetricLabels& labels) {
    if (!mPromMetricsMap.count(key)) {
        return nullptr;
    }
    return mPromMetricsMap[key]->GetOrCreateReentrantMetricsRecordRef(labels);
}
} // namespace logtail