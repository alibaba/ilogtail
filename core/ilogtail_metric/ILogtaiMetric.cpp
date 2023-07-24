#include "ILogtailMetric.h"

using namespace sls_logs;

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

Metrics::Metrics() {
    mDeleted.store(false);
}

Metrics::~Metrics() {
}

WriteMetrics::WriteMetrics() {   
    mSnapshotting.store(false);
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

std::string Counter::GetName() {
    return mName;
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

const std::vector<std::pair<std::string, std::string>>& Metrics::GetLabels() {
    return mLabels;
}

const std::vector<Counter*>& Metrics::GetValues() {
    return mValues;
}

Metrics* Metrics::Copy() {
    std::vector<std::pair<std::string, std::string>> newLabels;
    for (std::vector<std::pair<std::string, std::string>>::iterator it = mLabels.begin(); it != mLabels.end(); ++it) {
        std::pair<std::string, std::string> pair = *it;
        newLabels.push_back(std::make_pair(pair.first, pair.second));
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
    std::lock_guard<std::mutex> lock(mMutex);   
    // add metric to head
    // 根据mSnapshotting的状态，决定节点加到哪里
    if (mSnapshotting) {
        Metrics* oldHead = mSnapshottingHead;
        mSnapshottingHead = cur;
        mSnapshottingHead->next = oldHead;
        LOG_INFO(sLogger, ("CreateMetrics", "mSnapshottingHead"));

    } else {
        Metrics* oldHead = mHead;
        mHead = cur;
        mHead->next = oldHead;
        LOG_INFO(sLogger, ("CreateMetrics", "mHead"));
    }
    return cur;
}


// mark as deleted
void WriteMetrics::DestroyMetrics(Metrics* metrics) {
    // deleted is atomic_bool, no need to lock
    metrics->MarkDeleted();    
}


Metrics* WriteMetrics::DoSnapshot() {
    mSnapshotting.store(true);
    // new read head
    Metrics* snapshot = NULL;
    // new read head iter
    Metrics* rTmp;
    // old head iter
    //Metrics* tmp = mHead;
    Metrics* emptyHead = new Metrics();
    {
        std::lock_guard<std::mutex> lock(mMutex);  
        emptyHead->next = mHead;
    }
    Metrics* preTmp = emptyHead;
    while(preTmp) {
        Metrics* tmp = preTmp->next;
        if (!tmp) {
            break;
        }
        if (tmp->IsDeleted()) {
            Metrics* toDeleted = tmp;
            //tmp = tmp->next;
            preTmp->next = tmp->next;
            //preTmp = preTmp->next;
            delete toDeleted;
            continue;
        }
        Metrics* newMetrics = tmp->Copy();
        // Get Head
        if (!snapshot) {

            preTmp = preTmp->next;
            snapshot = newMetrics;
            rTmp = snapshot;
            continue;
        }
        preTmp = preTmp->next;
        rTmp->next = newMetrics;
        rTmp = newMetrics;
    }
    // Only lock when change head
    {
        std::lock_guard<std::mutex> lock(mMutex);
        // 把临时的节点加到链表里
        if (mSnapshottingHead) {
            Metrics* tmpSnapHead = mSnapshottingHead;
            Metrics* tmpMHead = emptyHead->next;
            emptyHead->next = mSnapshottingHead;
            while(tmpSnapHead) {
                if (tmpSnapHead->next == NULL) {
                    tmpSnapHead->next = tmpMHead;
                    break;
                } else {
                    tmpSnapHead = tmpSnapHead->next;
                }
            }
            mSnapshottingHead = NULL;
        }
        mHead = emptyHead->next;
        mSnapshotting.store(false);
        delete emptyHead;
    }
    return snapshot;
}

void ReadMetrics::ReadAsLogGroup(sls_logs::LogGroup& logGroup) {
    logGroup.set_category("metric-test");

    ReadLock lock(mReadWriteLock);
    Metrics* tmp = mHead;
    while(tmp) {
        Log* logPtr = logGroup.add_logs();
        logPtr->set_time(time(NULL));
        std::vector<std::pair<std::string, std::string>> labels = tmp->GetLabels();
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key("label." + pair.first);
            contentPtr->set_value(pair.second);
        }

        std::vector<Counter*> values = tmp->GetValues();
        for (std::vector<Counter*>::iterator it = values.begin(); it != values.end(); ++it) {
            Counter* counter = *it;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key("value." + counter->GetName());
            contentPtr->set_value(ToString(counter->GetValue()));
        }
        tmp = tmp->next;
    }
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
        WriteLock lock(mReadWriteLock);
        mHead = snapshot;
    }
    // delete old linklist
    while(toDelete) {
        Metrics* obj = toDelete;
        toDelete = toDelete->next;
        delete obj;
    }
}
}