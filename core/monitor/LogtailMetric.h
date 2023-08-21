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
        BaseMetric(const std::string &name, uint64_t val) {
            mName = name;
            mVal = val;
        };
        virtual ~BaseMetric() {};
        const uint64_t GetValue() const;
        const std::string& GetName() const;
        virtual void SetValue(uint64_t val) = 0;
        virtual BaseMetric* CopyAndReset() = 0;
};


class Gauge : public BaseMetric {
    public:
        Gauge(const std::string &name, uint64_t val);   
        void SetValue(uint64_t val) override;
        Gauge* CopyAndReset() override;
};


class Counter : public BaseMetric {
    public:
        Counter(const std::string &name, uint64_t val);
        void SetValue(uint64_t val) override;
        Counter* CopyAndReset() override;
};

using MetricPtr = std::shared_ptr<BaseMetric>;

class Metrics {
    private:
        std::vector<std::pair<std::string, std::string>> mLabels;
        std::atomic_bool mDeleted;
        std::vector<MetricPtr> mValues;
        Metrics* mNext  = nullptr;
    public:
        Metrics(const std::vector<std::pair<std::string, std::string>>&& labels);
        Metrics();
        void MarkDeleted();
        bool IsDeleted();
        const std::vector<std::pair<std::string, std::string>>& GetLabels() const;
        const std::vector<MetricPtr>& GetValues() const;
        MetricPtr CreateCounter(const std::string& Name);
        MetricPtr CreateGauge(const std::string& Name);
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