#include "ILogtailMetric.h"

namespace logtail {


Counter::Counter(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
    mTimestamp = time(NULL);
}

Counter::~Counter() {
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

uint64_t Counter::GetValue() {
    return mVal;
}

 uint64_t Counter::GetTimestamp() {
    return mTimestamp;
 }


Counter* Counter::CopyAndReset() {
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


void Metrics::MarkDeleted() {
    mDeleted.store(true);
}

bool Metrics::IsDeleted() {
    return mDeleted;
}

std::vector<std::pair<std::string, std::string>> Metrics::GetLabels() {
    return mLabels;
}

Metrics* Metrics::Copy() {
    std::vector<std::pair<std::string, std::string>> newLabels;
    for (std::vector<std::pair<std::string, std::string>>::iterator it = mLabels.begin(); it != mLabels.end(); ++it) {
        std::pair<std::string, std::string> pair = *it;
        newLabels.push_back(pair);
    }
    Metrics* metrics = new Metrics(newLabels);
    for (std::vector<Counter*>::iterator it = mValues.begin(); it != mValues.end(); ++it) {
        Counter* cur = *it;
        metrics->mValues.push_back(cur->CopyAndReset());
    }
    return metrics;
}

Metrics* WriteMetrics::CreateMetrics(std::vector<std::pair<std::string, std::string>> labels) {
    Metrics* cur = new Metrics(labels);    
    // add metric to head
    Metrics* oldHead = mHead;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mHead = cur;
        mHead->next = oldHead;
    }
    return cur;
}


// mark as deleted
void WriteMetrics::DestroyMetrics(Metrics* metrics) {
    // deleted is atomic_bool, no need to lock
    metrics->MarkDeleted();    
}


Metrics* WriteMetrics::DoSnapshot() {
    // new read head
    Metrics* snapshot = NULL;
    // new read head iter
    Metrics* rTmp ;
    // new write head
    Metrics* wTmpHead = NULL;
    // new write head iter
    Metrics* wTmp = NULL;
    // old head iter
    Metrics* tmp = mHead;

    while(tmp) {
        if (tmp->IsDeleted()) {
            Metrics* toDeleted = tmp;
            tmp = tmp->next;
            delete toDeleted;
            continue;
        }
        Metrics* newMetrics = tmp->Copy();
        // Get Head
        if (!snapshot) {
            wTmpHead = tmp;
            wTmp = wTmpHead;
            tmp = tmp->next;
            snapshot = newMetrics;
            rTmp = snapshot;
            continue;
        }
        wTmp->next = tmp;
        tmp = tmp->next;
        rTmp->next = newMetrics;
        rTmp = newMetrics;
    }
    // Only lock when change head
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mHead = wTmpHead;
    }
    return snapshot;
}

void ReadMetrics::ReadAsLogGroup() {
    ReadLock lock(mReadWriteLock);
    // Do some read
}

void ReadMetrics::ReadAsPrometheus() {
    ReadLock lock(mReadWriteLock);
    // Do some read
}

void ReadMetrics::UpdateMetrics() {
    Metrics* snapshot = mWriteMetrics->DoSnapshot();
    Metrics* toDelete = mHead;
    {
        // Only lock when change head
        mReadWriteLock.lock();
        mHead = snapshot;
        mReadWriteLock.unlock();
    }
    // delete old linklist
    while(toDelete) {
        Metrics* obj = toDelete;
        toDelete = toDelete->next;
        delete obj;
    }
}
}