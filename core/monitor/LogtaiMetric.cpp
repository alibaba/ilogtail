#include "LogtailMetric.h"
#include "common/StringTools.h"
#include "MetricConstants.h"
#include "logger/Logger.h"


using namespace sls_logs;

namespace logtail {


const uint64_t BaseMetric::GetValue() const {
    return mVal;
}

const std::string BaseMetric::GetName() const {
    return mName;
}

Counter::Counter(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
}

Counter* Counter::CopyAndReset() {
    Counter* counter = new Counter(mName);
    counter->mVal = mVal.exchange(0);
    return counter;
}

void Counter::Add(uint64_t value) {
    mVal += value;
}

Gauge::Gauge(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
}

Gauge* Gauge::CopyAndReset() {
    Gauge* gauge = new Gauge(mName);
    gauge->mVal = mVal.exchange(0);
    return gauge;
}

void Gauge::Set(uint64_t value) {
    mVal = value;
}


Metrics::Metrics(const std::vector<std::pair<std::string, std::string>>& labels) : mLabels(std::move(labels)), mDeleted(false) {}

Metrics::Metrics() : mDeleted(false) {}

WriteMetrics::WriteMetrics() {}

ReadMetrics::ReadMetrics() {}


CounterPtr Metrics::CreateCounter(const std::string name) {
    CounterPtr counterPtr = std::make_shared<Counter>(name);
    mValues.push_back(counterPtr);
    return counterPtr;
}

GaugePtr Metrics::CreateGauge(const std::string name) {
    GaugePtr gaugePtr = std::make_shared<Gauge>(name);
    mValues.push_back(gaugePtr);
    return gaugePtr;
}

void Metrics::MarkDeleted() {
    mDeleted = true;
}

bool Metrics::IsDeleted() {
    return mDeleted;
}

const std::vector<std::pair<std::string, std::string>>& Metrics::GetLabels() const {
    return mLabels;
}

const std::vector<MetricPtr>& Metrics::GetValues() const {
    return mValues;
}

Metrics* Metrics::CopyAndReset() {
    std::vector<std::pair<std::string, std::string>> newLabels(mLabels);
    Metrics* metrics =  new Metrics(newLabels); 
    for (auto &item: mValues) {
        MetricPtr newPtr(item->CopyAndReset());
        metrics->mValues.push_back(newPtr);
    }
    return metrics;
}

Metrics* Metrics::GetNext() {
    return mNext;
}

void Metrics::SetNext(Metrics* next) {
    mNext = next; 
}

// mark as deleted
void DestroyMetrics(Metrics* metrics) {
    // deleted is atomic_bool, no need to lock
    LOG_INFO(sLogger, ("DestroyMetrics", "true"));
    metrics->MarkDeleted();
}

MetricsPtr WriteMetrics::CreateMetrics(const std::vector<std::pair<std::string, std::string>>& labels) {
    Metrics* cur = new Metrics(std::move(labels)); 
    std::lock_guard<std::mutex> lock(mMutex);   

    Metrics* oldHead = mHead;
    mHead = cur;
    mHead->SetNext(oldHead);

    MetricsPtr curUnique(cur, DestroyMetrics);
    return curUnique;
}

Metrics* WriteMetrics::GetHead() {
    return mHead;
}

Metrics* WriteMetrics::DoSnapshot() {
    // new read head
    Metrics* snapshot = nullptr;
    Metrics* toDeleteHead = nullptr;
    Metrics* tmp = nullptr;
    
    Metrics* emptyHead = new Metrics();
    Metrics* preTmp = nullptr;
    // find the first not deleted node and set as new mHead
    {
        std::lock_guard<std::mutex> lock(mMutex); 
        emptyHead->SetNext(mHead);
        preTmp = emptyHead;
        tmp = preTmp->GetNext();
        while (tmp) {
            if (tmp->IsDeleted()) {
                preTmp->SetNext(tmp->GetNext());
                tmp->SetNext(toDeleteHead);
                toDeleteHead = tmp;
                tmp = preTmp->GetNext();
            } else {
                mHead = tmp;
                break;
            }
        }
        if (!tmp) {
            mHead = nullptr;
        }
    }
    
    while (tmp) {
        if (tmp->IsDeleted()) {
            preTmp->SetNext(tmp->GetNext());
            tmp->SetNext(toDeleteHead);
            toDeleteHead = tmp;
            tmp = preTmp->GetNext();
        } else {
            Metrics* newMetrics = tmp->CopyAndReset();
            newMetrics->SetNext(snapshot);
            snapshot = newMetrics;
            preTmp = tmp;
            tmp = tmp->GetNext();
        }
    }

    while(toDeleteHead) {
        Metrics* toDeleted = toDeleteHead;
        toDeleteHead = toDeleteHead->GetNext();
        delete toDeleted;
    }
    delete emptyHead;
    return snapshot;
}

void ReadMetrics::ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup>& logGroupMap) {
    ReadLock lock(mReadWriteLock);
    Metrics* tmp = mHead;
    while(tmp) {
        Log* logPtr = nullptr;
        for (auto &item: tmp->GetLabels()) {
            std::pair<std::string, std::string> pair = item;
            if (METRIC_FIELD_REGION == pair.first) {
                std::map<std::string, sls_logs::LogGroup>::iterator iter;
                std::string region = pair.second;
                iter = logGroupMap.find(region);
                if (iter != logGroupMap.end()) {
                    sls_logs::LogGroup logGroup = iter->second;
                    logPtr = logGroup.add_logs();
                } else {
                    sls_logs::LogGroup logGroup;
                    logPtr = logGroup.add_logs();
                    logGroupMap.insert(std::pair<std::string, sls_logs::LogGroup>(region, logGroup));
                }
            }
        }
        if (!logPtr) {
            std::map<std::string, sls_logs::LogGroup>::iterator iter;
            iter = logGroupMap.find(METRIC_REGION_DEFAULT);
            if (iter != logGroupMap.end()) {
                sls_logs::LogGroup logGroup = iter->second;
                logPtr = logGroup.add_logs();
            } else {
                sls_logs::LogGroup logGroup;
                logPtr = logGroup.add_logs();
                logGroupMap.insert(std::pair<std::string, sls_logs::LogGroup>(METRIC_REGION_DEFAULT, logGroup));
            }
        }
        logPtr->set_time(time(nullptr));
        for (auto &item: tmp->GetLabels()) {
            std::pair<std::string, std::string> pair = item;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(LABEL_PREFIX + pair.first);
            contentPtr->set_value(pair.second);
        }

        //std::vector<MetricPtr> values = tmp->GetValues();
        for (auto &item: tmp->GetValues()) {
            MetricPtr counter = item;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(VALUE_PREFIX + counter->GetName());
            contentPtr->set_value(ToString(counter->GetValue()));
        }
        tmp = tmp->GetNext();
    }
}

Metrics* ReadMetrics::GetHead() {
    return mHead;
}

void ReadMetrics::UpdateMetrics() {
    Metrics* snapshot = WriteMetrics::GetInstance()->DoSnapshot();
    Metrics* toDelete;
    {
        // Only lock when change head
        WriteLock lock(mReadWriteLock);
        toDelete = mHead;
        mHead = snapshot;
    }
    // delete old linklist
    while(toDelete) {
        Metrics* obj = toDelete;
        toDelete = toDelete->GetNext();
        delete obj;
    }
}
}