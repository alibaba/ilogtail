#pragma once

#include "PluginMetricManager.h"
#include "monitor/LoongCollectorMetricTypes.h"

namespace logtail {

class PromSelfMonitor {
public:
    bool Init() { return true; }
    void Stop();

    void InitMetricManager(const std::string& key,
                           MetricLabels& defaultLabels,
                           std::unordered_map<std::string, MetricType> metricKeys) {
        if (!mPromMetricsMap.count(key)) {
            mPromMetricsMap[key] = std::make_shared<PluginMetricManager>(defaultLabels, metricKeys);
        }
    }

    ReentrantMetricsRecordRef GetOrCreateReentrantMetricsRecordRef(const std::string& key, MetricLabels& labels) {
        if (!mPromMetricsMap.count(key)) {
            return nullptr;
        }
        return mPromMetricsMap[key]->GetOrCreateReentrantMetricsRecordRef(labels);
    }
    IntGaugePtr GetIntGauge(const std::string& key, const std::string& name) {
        if (!mPromMetricsMap.count(key)) {
            return nullptr;
        }
        return mPromMetricsMap[key]->GetIntGauge(name);
    }

private:
    std::map<std::string, PluginMetricManagerPtr> mPromMetricsMap;
};

} // namespace logtail