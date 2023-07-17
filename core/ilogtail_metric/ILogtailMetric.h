#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <atomic>
#include "logger/Logger.h"
#include <MetricConstants.h>
#include "common/Lock.h"


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
        
        void Add(uint64_t val);
        void Set(uint64_t val);
        Counter* CopyAndReset();
};

class Metrics {
    private:
        std::vector<std::pair<std::string, std::string>> mLabels;
        std::vector<Counter*> mValues;
        std::atomic_bool mDeleted;

    public:
        Metrics(std::vector<std::pair<std::string, std::string> > labels);
        ~Metrics();
        void MarkDeleted();
        bool IsDeleted();
        std::vector<std::pair<std::string, std::string>> GetLabels();
        Counter* CreateCounter(std::string Name);
        Metrics* Copy();
        Metrics* next = NULL;
};

class WriteMetrics {
    private:
        WriteMetrics();
        ~WriteMetrics();
    public:
        static WriteMetrics* GetInstance() {
            static WriteMetrics* ptr = new WriteMetrics();
            return ptr;
        }
        Metrics* CreateMetrics(std::vector<std::pair<std::string, std::string>> Labels);
        void DestroyMetrics(Metrics* metrics);
        void ClearDeleted();
        Metrics* DoSnapshot();
        // empty head node
        Metrics* mHead = NULL;
        std::mutex mMutex;
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
        void Read();
        void UpdateMetrics();
        Metrics* mHead = NULL;
        WriteMetrics* mWriteMetrics;
        ReadWriteLock mReadWriteLock;
};
}