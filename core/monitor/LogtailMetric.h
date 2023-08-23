#pragma once
#include <string>
#include <atomic>
#include "common/Lock.h"
#include "log_pb/sls_logs.pb.h"


namespace logtail {


class MetricNameValue {
protected:
    std::string mName;
    std::atomic_long mVal;

public:
    MetricNameValue(const std::string& name, uint64_t val) {
        mName = name;
        mVal = val;
    };
    virtual ~MetricNameValue(){};
    const uint64_t GetValue() const;
    const std::string& GetName() const;
    virtual void SetValue(uint64_t val) = 0;
    virtual MetricNameValue* CopyAndReset() = 0;
};


class Gauge : public MetricNameValue {
public:
    Gauge(const std::string& name, uint64_t val);
    void SetValue(uint64_t val) override;
    Gauge* CopyAndReset() override;
};


class Counter : public MetricNameValue {
public:
    Counter(const std::string& name, uint64_t val);
    void SetValue(uint64_t val) override;
    Counter* CopyAndReset() override;
};

using MetricNameValuePtr = std::shared_ptr<MetricNameValue>;
using LabelsPtr = std::shared_ptr<std::vector<std::pair<std::string, std::string>>>;

class MetricsRecord {
private:
    LabelsPtr mLabels;
    std::atomic_bool mDeleted;
    std::vector<MetricNameValuePtr> mValues;
    MetricsRecord* mNext = nullptr;

public:
    MetricsRecord(LabelsPtr labels);
    MetricsRecord();
    void MarkDeleted();
    bool IsDeleted();
    LabelsPtr GetLabels();
    const std::vector<MetricNameValuePtr>& GetValues() const;
    MetricNameValuePtr CreateCounter(const std::string& Name);
    MetricNameValuePtr CreateGauge(const std::string& Name);
    MetricsRecord* CopyAndReset();
    void SetNext(MetricsRecord* next);
    MetricsRecord* GetNext();
};

class MetricsRecordRef {
private:
    MetricsRecord* mMetrics = nullptr;

public:
    MetricsRecordRef();
    ~MetricsRecordRef();
    void Init(const std::vector<std::pair<std::string, std::string>>& labels);
    MetricsRecord* operator->() const;
};

class WriteMetrics {
private:
    WriteMetrics();
    std::mutex mMutex;
    MetricsRecord* mHead = nullptr;

public:
    ~WriteMetrics();
    static WriteMetrics* GetInstance() {
        static WriteMetrics* ptr = new WriteMetrics();
        return ptr;
    }
    MetricsRecord* CreateMetrics(LabelsPtr Labels);
    MetricsRecord* DoSnapshot();
    MetricsRecord* GetHead();
};

class ReadMetrics {
private:
    ReadMetrics();
    ReadWriteLock mReadWriteLock;
    MetricsRecord* mHead = nullptr;

public:
    ~ReadMetrics();
    static ReadMetrics* GetInstance() {
        static ReadMetrics* ptr = new ReadMetrics();
        return ptr;
    }
    void ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup*>& logGroupMap);
    void UpdateMetrics();
    MetricsRecord* GetHead();
};
} // namespace logtail