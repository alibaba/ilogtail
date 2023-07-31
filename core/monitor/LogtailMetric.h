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

class Counter{
    private:
        std::string mName;
        std::atomic_long mVal;
        std::atomic_long mTimestamp;

    public:
        Counter(std::string name);
        ~Counter();
        uint64_t GetValue();
        uint64_t GetTimestamp();
        std::string GetName();
        
        void Add(uint64_t val);
        void Set(uint64_t val);
        Counter* CopyAndReset();
};

typedef std::shared_ptr<Counter> CounterPtr;

class Metrics {
    private:
        std::vector<std::pair<std::string, std::string>> mLabels;
        std::vector<CounterPtr> mValues;
        std::atomic_bool mDeleted;

    public:
        Metrics(std::vector<std::pair<std::string, std::string> > labels);
        Metrics();
        ~Metrics();
        void MarkDeleted();
        bool IsDeleted();
        const std::vector<std::pair<std::string, std::string>>& GetLabels();
        const std::vector<CounterPtr>& GetValues();
        CounterPtr CreateCounter(std::string Name);
        Metrics* Copy();
        Metrics* next = NULL;
};

class WriteMetrics {
    private:
        WriteMetrics();
        ~WriteMetrics();
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
        ~ReadMetrics();
    public:
        static ReadMetrics* GetInstance() {
            static ReadMetrics* ptr = new ReadMetrics();
            return ptr;
        }
        void ReadAsLogGroup(sls_logs::LogGroup& logGroup);
        void ReadAsMap(std::map<std::string, std::string> map);
        void ReadAsPrometheus();
        void UpdateMetrics();
        Metrics* mHead = NULL;
        WriteMetrics* mWriteMetrics;
        ReadWriteLock mReadWriteLock;
};
}