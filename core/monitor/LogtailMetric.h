#pragma once
#include <string>
#include <atomic>
#include "common/Lock.h"
#include "log_pb/sls_logs.pb.h"


namespace logtail {

class Counter {
private:
    std::string mName;
    std::atomic_long mVal;

public:
    Counter(const std::string& name, uint64_t val);
    uint64_t GetValue() const;
    const std::string& GetName() const;
    void Add(uint64_t val);
    Counter* CopyAndReset();
};

class Gauge {
private:
    std::string mName;
    std::atomic_long mVal;

public:
    Gauge(const std::string& name, uint64_t val);
    uint64_t GetValue() const;
    const std::string& GetName() const;
    void Set(uint64_t val);
    Gauge* CopyAndReset();
};


using CounterPtr = std::shared_ptr<Counter>;
using GaugePtr = std::shared_ptr<Gauge>;

using MetricLabels = std::vector<std::pair<std::string, std::string>>;
using LabelsPtr = std::shared_ptr<MetricLabels>;

class MetricsRecord {
private:
    LabelsPtr mLabels;
    std::atomic_bool mDeleted;
    std::vector<CounterPtr> mCounters;
    std::vector<GaugePtr> mGauges;
    MetricsRecord* mNext = nullptr;

public:
    MetricsRecord(LabelsPtr labels);
    MetricsRecord();
    void MarkDeleted();
    bool IsDeleted() const;
    const LabelsPtr& GetLabels() const;
    const std::vector<CounterPtr>& GetCounters() const;
    const std::vector<GaugePtr>& GetGauges() const;
    CounterPtr CreateCounter(const std::string& Name);
    GaugePtr CreateGauge(const std::string& Name);
    MetricsRecord* CopyAndReset();
    void SetNext(MetricsRecord* next);
    MetricsRecord* GetNext() const;
};

class MetricsRecordRef {
private:
    MetricsRecord* mMetrics = nullptr;

public:
    ~MetricsRecordRef();
    void SetMetricsRecord(MetricsRecord* metricRecord);
    MetricsRecord* operator->() const;
};

class WriteMetrics {
private:
    WriteMetrics();
    std::mutex mMutex;
    MetricsRecord* mHead = nullptr;
    void Clear();
public:
    ~WriteMetrics();
    static WriteMetrics* GetInstance() {
        static WriteMetrics* ptr = new WriteMetrics();
        return ptr;
    }
    void PrepareMetricsRecordRef(MetricsRecordRef& ref, MetricLabels&& Labels);
    MetricsRecord* DoSnapshot();
    MetricsRecord* GetHead() const;

#ifdef APSARA_UNIT_TEST_MAIN
    friend class ILogtailMetricUnittest;
#endif
};

class ReadMetrics {
private:
    ReadMetrics();
    mutable ReadWriteLock mReadWriteLock;
    MetricsRecord* mHead = nullptr;
    void Clear();
public:
    ~ReadMetrics();
    static ReadMetrics* GetInstance() {
        static ReadMetrics* ptr = new ReadMetrics();
        return ptr;
    }
    void ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup*>& logGroupMap) const;
    void UpdateMetrics();
    MetricsRecord* GetHead() const;
    
#ifdef APSARA_UNIT_TEST_MAIN
    friend class ILogtailMetricUnittest;
#endif
};
} // namespace logtail
