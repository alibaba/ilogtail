#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <atomic>
#include "logger/Logger.h"
#include <MetricConstants.h>
#include "common/Lock.h"
#include "log_pb/sls_logs.pb.h"
#include "common/StringTools.h"


namespace logtail {


class BaseMetric {
    protected:
        std::string mName;
        std::atomic_long mVal;
        std::atomic_long mTimestamp;
    public:
        const uint64_t GetValue() const;
        const uint64_t GetTimestamp() const;
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
        std::vector<MetricPtr> mValues;
        std::atomic_bool mDeleted;


    public:
        Metrics(std::vector<std::pair<std::string, std::string>> labels);
        Metrics();
        void MarkDeleted();
        bool IsDeleted();
        const std::vector<std::pair<std::string, std::string>>& GetLabels() const;
        const std::vector<MetricPtr>& GetValues() const;
        CounterPtr CreateCounter(const std::string Name);
        GaugePtr CreateGauge(const std::string Name);

        //using MetricsUniq = std::unique_ptr<Metrics, void(*)(Metrics*)>;
        Metrics* Copy();
        Metrics* next;
};


class WriteMetrics {
    private:
        WriteMetrics();
        std::atomic_bool mSnapshotting;
    public:
        static WriteMetrics* GetInstance() {
            static WriteMetrics* ptr = new WriteMetrics();
            return ptr;
        }
        Metrics* CreateMetrics(std::vector<std::pair<std::string, std::string>> Labels);
        void DestroyMetrics(Metrics* metrics);
        Metrics* DoSnapshot();
        Metrics* mHead = NULL;
        Metrics* mSnapshottingHead = NULL;
        std::mutex mMutex;
        std::mutex mSnapshottingMutex;

};

class ReadMetrics {
   private:
        ReadMetrics();
    public:
        static ReadMetrics* GetInstance() {
            static ReadMetrics* ptr = new ReadMetrics();
            return ptr;
        }
        void ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup>& logGroupMap);
        // void ReadAsMap(std::map<std::string, std::string> map);
        // void ReadAsPrometheus();
        void UpdateMetrics();
        Metrics* mHead = NULL;
        WriteMetrics* mWriteMetrics;
        ReadWriteLock mReadWriteLock;
};
}