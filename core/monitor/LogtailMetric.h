#pragma once
#include <string>
#include <atomic>
#include "common/Lock.h"
#include "log_pb/sls_logs.pb.h"


namespace logtail {


class BaseMetric {
    protected:
        std::string mName;
        std::atomic_long mVal;
    public:
        const uint64_t GetValue() const;
        const std::string GetName() const;
        virtual BaseMetric* CopyAndReset() = 0;
};


class Gauge : public BaseMetric {
    public:
        Gauge(const std::string name);   
        void Set(uint64_t val);
        Gauge* CopyAndReset();
};


class Counter : public BaseMetric {
    public:
        Counter(const std::string name);
        void Add(uint64_t val);
        Counter* CopyAndReset();
};

using MetricPtr = std::shared_ptr<BaseMetric>;
using CounterPtr = std::shared_ptr<Counter>;
using GaugePtr = std::shared_ptr<Gauge>;


class Metrics {
    private:
        std::vector<std::pair<std::string, std::string>> mLabels;
        std::atomic_bool mDeleted;
        std::vector<MetricPtr> mValues;
        Metrics* mNext  = nullptr;
    public:
        Metrics(const std::vector<std::pair<std::string, std::string>>& labels);
        Metrics();
        void MarkDeleted();
        bool IsDeleted();
        const std::vector<std::pair<std::string, std::string>>& GetLabels() const;
        const std::vector<MetricPtr>& GetValues() const;
        CounterPtr CreateCounter(const std::string Name);
        GaugePtr CreateGauge(const std::string Name);
        Metrics* CopyAndReset();
        void SetNext(Metrics* next);
        Metrics* GetNext();
};

class MetricsRef {
    private:
        Metrics* mMetrics = nullptr;
    public:
        MetricsRef();
        ~MetricsRef();
        void Init(const std::vector<std::pair<std::string, std::string>>& Labels);
        Metrics* Get();
};

class WriteMetrics {
    private:
        WriteMetrics();
        std::mutex mMutex;
        Metrics* mHead = nullptr;
    public:
        static WriteMetrics* GetInstance() {
            static WriteMetrics* ptr = new WriteMetrics();
            return ptr;
        }
        Metrics* CreateMetrics(const std::vector<std::pair<std::string, std::string>>& Labels);

        Metrics* DoSnapshot();
        Metrics* GetHead();
};

class ReadMetrics {
   private:
        ReadMetrics();
        ReadWriteLock mReadWriteLock;
        Metrics* mHead = nullptr;
    public:
        static ReadMetrics* GetInstance() {
            static ReadMetrics* ptr = new ReadMetrics();
            return ptr;
        }
        void ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup*>& logGroupMap);
        void UpdateMetrics();
        Metrics* GetHead();
};
}