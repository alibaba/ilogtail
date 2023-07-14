#include "ILogtailMetric.h"

namespace logtail {


Counter::Counter(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
    mTimestamp = time(NULL);
}

Counter::Counter() {
    mVal = (uint64_t)0;
    mTimestamp = time(NULL);
}

Counter::~Counter() {
}

Metrics::Metrics() {
    mDeleted.store(false);
}

Metrics::Metrics(std::vector<std::pair<std::string, std::string>> labels) {
    mLabels = labels;
    mDeleted.store(false);
}

Metrics::~Metrics() {
}

WriteMetrics::WriteMetrics() {   
}

WriteMetrics::~WriteMetrics() {   
}

ReadMetrics::ReadMetrics() {
    mWriteMetrics = WriteMetrics::GetInstance();
}

ReadMetrics::~ReadMetrics() {
}

void Counter::Add(uint64_t value) {
    mVal += value;
    mTimestamp = time(NULL);
}

void Counter::Set(uint64_t value) {
    mVal = value;
    mTimestamp = time(NULL);
}

Counter* Counter::Copy() {
    Counter* counter = new Counter(mName);
    counter->mVal = mVal.exchange(0);
    counter->mTimestamp = mTimestamp.exchange(0);
    return counter;
}

Counter* Metrics::CreateCounter(std::string name) {
    Counter* counter = new Counter(name);
    mValues.push_back(counter);
    return counter;
}


Metrics* Metrics::Copy() {
    Metrics* metrics = new Metrics();
    for (std::vector<std::pair<std::string, std::string>>::iterator it = mLabels.begin(); it != mLabels.end(); ++it) {
        std::pair<std::string, std::string> pair = *it;
        metrics->mLabels.push_back(pair);
    }
    for (std::vector<Counter*>::iterator it = mValues.begin(); it != mValues.end(); ++it) {
        Counter* cur = *it;
        metrics->mValues.push_back(cur->Copy());
    }
    return metrics;
}

Metrics* WriteMetrics::CreateMetrics(std::vector<std::pair<std::string, std::string>> labels) {
    std::lock_guard<std::mutex> lock(mMutex);
    Metrics* cur = new Metrics(labels);
    mTail->next = cur;
    mTail = cur;
    return cur;
}


void WriteMetrics::DestroyMetrics(Metrics* metrics) {
    std::lock_guard<std::mutex> lock(mMutex);
    // mark as delete
    metrics->mDeleted = true;
    mDeletedMetrics.push_back(metrics);
    
}

void WriteMetrics::ClearDeleted() {
    std::lock_guard<std::mutex> lock(mMutex);
    Metrics* cur = mHead;
    while(cur) {
        // unlink metrics from mHead and add to mDeletedMetrics;
        // do not modify metrics's next pointer to let who is reading this Metrics can read next Metrics normally
        Metrics* next = cur->next;
        if (next != NULL && next->mDeleted) {
            cur->next = next->next;
        }
        cur = cur->next;
    }
    std::vector<Metrics*> tmp;
    tmp.swap(mDeletedMetrics);
}

Metrics* WriteMetrics::DoSnapshot() {
    Metrics* snapshot;
    Metrics* tmp = mHead;
    Metrics* cTmp;
    if (tmp) {
        Metrics* newMetrics = tmp->Copy();
        tmp = tmp -> next;
        snapshot = newMetrics;
        cTmp = snapshot;
        while(tmp) {
            while(tmp->mDeleted) {
                tmp = tmp -> next;
            }
            Metrics* newMetrics = tmp->Copy();
            tmp = tmp -> next;
            cTmp -> next = newMetrics;
            cTmp = newMetrics;
        }
    }
    ClearDeleted();
    return snapshot;
}

void ReadMetrics::Read() {
    std::lock_guard<std::mutex> lock(mMutex);
      // Do some read
}

void ReadMetrics::UpdateMetrics() {
    Metrics* snapshot = mWriteMetrics->DoSnapshot();
    Metrics* toDelete = mHead;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mHead = snapshot->next;
    }
    // do deletion
    while(toDelete) {
        Metrics* obj = toDelete;
        toDelete = toDelete->next;
        delete obj;
    }
}
}