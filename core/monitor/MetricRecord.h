
/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "LoongCollectorMetricTypes.h"

namespace logtail {

class MetricsRecord {
private:
    MetricLabelsPtr mLabels;
    DynamicMetricLabelsPtr mDynamicLabels;
    std::atomic_bool mDeleted;
    std::vector<CounterPtr> mCounters;
    std::vector<TimeCounterPtr> mTimeCounters;
    std::vector<IntGaugePtr> mIntGauges;
    std::vector<DoubleGaugePtr> mDoubleGauges;
    MetricsRecord* mNext = nullptr;
    std::function<bool(const MetricsRecord&)> mShouldSkipFunc;

public:
    MetricsRecord(MetricLabelsPtr labels, DynamicMetricLabelsPtr dynamicLabels = nullptr);
    MetricsRecord() = default;
    void MarkDeleted();
    bool IsDeleted() const;
    const MetricLabelsPtr& GetLabels() const;
    const DynamicMetricLabelsPtr& GetDynamicLabels() const;
    const std::vector<CounterPtr>& GetCounters() const;
    const std::vector<TimeCounterPtr>& GetTimeCounters() const;
    const std::vector<IntGaugePtr>& GetIntGauges() const;
    const std::vector<DoubleGaugePtr>& GetDoubleGauges() const;
    CounterPtr CreateCounter(const std::string& name);
    TimeCounterPtr CreateTimeCounter(const std::string& name);
    IntGaugePtr CreateIntGauge(const std::string& name);
    DoubleGaugePtr CreateDoubleGauge(const std::string& name);
    MetricsRecord* Collect();
    bool ShouldSkip();
    void SetShouldSkipFunc(std::function<bool(const MetricsRecord&)> func);
    void SetNext(MetricsRecord* next);
    MetricsRecord* GetNext() const;
};

class MetricsRecordRef {
    friend class WriteMetrics;
    friend bool operator==(const MetricsRecordRef& lhs, std::nullptr_t rhs);
    friend bool operator==(std::nullptr_t rhs, const MetricsRecordRef& lhs);

private:
    MetricsRecord* mMetrics = nullptr;

public:
    ~MetricsRecordRef();
    MetricsRecordRef() = default;
    MetricsRecordRef(const MetricsRecordRef&) = delete;
    MetricsRecordRef& operator=(const MetricsRecordRef&) = delete;
    MetricsRecordRef(MetricsRecordRef&&) = delete;
    MetricsRecordRef& operator=(MetricsRecordRef&&) = delete;
    void SetMetricsRecord(MetricsRecord* metricRecord);
    const MetricLabelsPtr& GetLabels() const;
    const DynamicMetricLabelsPtr& GetDynamicLabels() const;
    CounterPtr CreateCounter(const std::string& name);
    TimeCounterPtr CreateTimeCounter(const std::string& name);
    IntGaugePtr CreateIntGauge(const std::string& name);
    DoubleGaugePtr CreateDoubleGauge(const std::string& name);
    void SetShouldSkipFunc(std::function<bool(const MetricsRecord&)> func);
    const MetricsRecord* operator->() const;
    // this is not thread-safe, and should be only used before WriteMetrics::CommitMetricsRecordRef
    void AddLabels(MetricLabels&& labels);
#ifdef APSARA_UNIT_TEST_MAIN
    bool HasLabel(const std::string& key, const std::string& value) const;
#endif
};

inline bool operator==(const MetricsRecordRef& lhs, std::nullptr_t rhs) {
    return lhs.mMetrics == rhs;
}

inline bool operator==(std::nullptr_t lhs, const MetricsRecordRef& rhs) {
    return lhs == rhs.mMetrics;
}

inline bool operator!=(const MetricsRecordRef& lhs, std::nullptr_t rhs) {
    return !(lhs == rhs);
}

inline bool operator!=(std::nullptr_t lhs, const MetricsRecordRef& rhs) {
    return !(lhs == rhs);
}

}