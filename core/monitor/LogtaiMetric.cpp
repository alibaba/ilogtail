#include "LogtailMetric.h"

using namespace sls_logs;

namespace logtail {


const uint64_t BaseMetric::GetValue() const {
    return mVal;
}

const uint64_t BaseMetric::GetTimestamp() const {
    return mTimestamp;
}

const std::string BaseMetric::GetName() const {
    return mName;
}

Counter::Counter(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
    mTimestamp = time(NULL);
}

Counter* Counter::CopyAndReset() {
    Counter* counter = new Counter(mName);
    counter->mVal = mVal.exchange(0);
    counter->mTimestamp = mTimestamp.exchange(0);
    return counter;
}

void Counter::Add(uint64_t value) {
    mVal += value;
    mTimestamp = time(NULL);
}

Gauge::Gauge(std::string name) {
    mName = name;
    mVal = (uint64_t)0;
    mTimestamp = time(NULL);
}

Gauge* Gauge::CopyAndReset() {
    Gauge* gauge = new Gauge(mName);
    gauge->mVal = mVal.exchange(0);
    gauge->mTimestamp = mTimestamp.exchange(0);
    return gauge;
}

void Gauge::Set(uint64_t value) {
    mVal = value;
    mTimestamp = time(NULL);
}


Metrics::Metrics(std::vector<std::pair<std::string, std::string>> labels) {
    mLabels = labels;
    mDeleted.store(false);
}

Metrics::Metrics() {
    mDeleted.store(false);
}

WriteMetrics::WriteMetrics() {   
    mSnapshotting.store(false);
}

ReadMetrics::ReadMetrics() {
    mWriteMetrics = WriteMetrics::GetInstance();
}


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
    mDeleted.store(true);
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

Metrics* Metrics::Copy() {
    std::vector<std::pair<std::string, std::string>> newLabels;
    for (std::vector<std::pair<std::string, std::string>>::iterator it = mLabels.begin(); it != mLabels.end(); ++it) {
        std::pair<std::string, std::string> pair = *it;
        newLabels.push_back(std::make_pair(pair.first, pair.second));
    }
    Metrics* metrics =  new Metrics(newLabels); 
    for (std::vector<MetricPtr>::iterator it = mValues.begin(); it != mValues.end(); ++it) {
        MetricPtr cur = *it;
        MetricPtr newPtr(cur->CopyAndReset());
        metrics->mValues.push_back(newPtr);
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

    } else {
        Metrics* oldHead = mHead;
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
    mSnapshotting.store(true);
    // new read head
    Metrics* snapshot = NULL;
    // new read head iter
    Metrics* rTmp;

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
            preTmp->next = tmp->next;
            delete toDeleted;
            continue;
        }
        Metrics* newMetrics(tmp->Copy());
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

void ReadMetrics::ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup>& logGroupMap) {
    ReadLock lock(mReadWriteLock);
    Metrics* tmp = mHead;
    while(tmp) {
        Log* logPtr = NULL;
        std::vector<std::pair<std::string, std::string>> labels = tmp->GetLabels();
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
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
        logPtr->set_time(time(NULL));
        for (std::vector<std::pair<std::string, std::string>>::iterator it = labels.begin(); it != labels.end(); ++it) {
            std::pair<std::string, std::string> pair = *it;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(LABEL_PREFIX + pair.first);
            contentPtr->set_value(pair.second);
        }

        std::vector<MetricPtr> values = tmp->GetValues();
        for (std::vector<MetricPtr>::iterator it = values.begin(); it != values.end(); ++it) {
            MetricPtr counter = *it;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(VALUE_PREFIX + counter->GetName());
            contentPtr->set_value(ToString(counter->GetValue()));
        }
        tmp = tmp->next;
    }
}



void ReadMetrics::UpdateMetrics() {
    Metrics* snapshot = mWriteMetrics->DoSnapshot();
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
        toDelete = toDelete->next;
        delete obj;
    }
}
}