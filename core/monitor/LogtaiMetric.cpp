#include "LogtailMetric.h"
#include "common/StringTools.h"
#include "MetricConstants.h"
#include "logger/Logger.h"
#include "common/TimeUtil.h"
#include "app_config/AppConfig.h"


using namespace sls_logs;

namespace logtail {


const uint64_t BaseMetric::GetValue() const {
    return mVal;
}

const std::string& BaseMetric::GetName() const {
    return mName;
}

Counter::Counter(const std::string &name, uint64_t val = 0) : BaseMetric(name, val) {}

Counter* Counter::CopyAndReset() {
    return new Counter(mName, mVal.exchange(0));
}

void Counter::SetValue(uint64_t value) {
    mVal += value;
}

Gauge::Gauge(const std::string &name, uint64_t val = 0) : BaseMetric(name, val) {}

Gauge* Gauge::CopyAndReset() {
    return new Gauge(mName, mVal.exchange(0));
}

void Gauge::SetValue(uint64_t value) {
    mVal = value;
}


Metrics::Metrics(const std::vector<std::pair<std::string, std::string>>&& labels) : mLabels(labels), mDeleted(false) {}

Metrics::Metrics() : mDeleted(false) {}

MetricPtr Metrics::CreateCounter(const std::string& name) {
    MetricPtr counterPtr = std::make_shared<Counter>(name);
    mValues.emplace_back(counterPtr);
    return counterPtr;
}

MetricPtr Metrics::CreateGauge(const std::string& name) {
    MetricPtr gaugePtr = std::make_shared<Gauge>(name);
    mValues.emplace_back(gaugePtr);
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
    Metrics* metrics = new Metrics(std::move(newLabels)); 
    for (auto &item: mValues) {
        MetricPtr newPtr(item->CopyAndReset());
        metrics->mValues.emplace_back(newPtr);
    }
    return metrics;
}

Metrics* Metrics::GetNext() {
    return mNext;
}

void Metrics::SetNext(Metrics* next) {
    mNext = next; 
}

MetricsRef::MetricsRef() {}

MetricsRef::~MetricsRef() {
    if (mMetrics) {
        mMetrics->MarkDeleted();
    }
}

void MetricsRef::Init(const std::vector<std::pair<std::string, std::string>>& Labels) {
    if (!mMetrics) {
        mMetrics = WriteMetrics::GetInstance()->CreateMetrics(std::move(Labels));
    }
}

Metrics* MetricsRef::Get() {
    return mMetrics;
}

WriteMetrics::WriteMetrics() {}

Metrics* WriteMetrics::CreateMetrics(const std::vector<std::pair<std::string, std::string>>& labels) {
    Metrics* cur = new Metrics(std::move(labels)); 
    std::lock_guard<std::mutex> lock(mMutex);   

    Metrics* oldHead = mHead;
    mHead = cur;
    mHead->SetNext(oldHead);

    return cur;
}

Metrics* WriteMetrics::GetHead() {
    return mHead;
}

Metrics* WriteMetrics::DoSnapshot() {
    // new read head
    Metrics* snapshot = nullptr;
    Metrics* toDeleteHead = nullptr;
    Metrics* tmp = nullptr;
    
    Metrics emptyHead;
    Metrics* preTmp = nullptr;

    Metrics* tmpHead = nullptr;
    Metrics* tmpHeadNext = nullptr;
    // find the first not deleted node and set as new mHead
    {
        std::lock_guard<std::mutex> lock(mMutex); 
        emptyHead.SetNext(mHead);
        preTmp = &emptyHead;
        tmp = preTmp->GetNext();
        bool findHead = false;
        while (tmp) {
            if (tmp->IsDeleted()) {
                preTmp->SetNext(tmp->GetNext());
                tmp->SetNext(toDeleteHead);
                toDeleteHead = tmp;
                tmp = preTmp->GetNext();
            } else {
                // find head
                if (!findHead) {
                    mHead = tmp;
                    preTmp = tmp;
                    tmp = tmp->GetNext();

                    mHead->SetNext(nullptr);
                    findHead = true;
                // find head next
                } else {
                    mHead->SetNext(tmp);
                    preTmp = tmp;
                    tmp = tmp->GetNext();
                    break;
                } 
            }
        }
        // if no undeleted node, set null to mHead
        if (!findHead) {
            mHead = nullptr;
        } else {
            tmpHead = mHead;
            tmpHeadNext = mHead->GetNext();
        }
    }

    // copy head
    if (tmpHead) {
        Metrics* newMetrics = tmpHead->CopyAndReset();
        newMetrics->SetNext(snapshot);
        snapshot = newMetrics;
    }
    // copy head next
    if (tmpHeadNext) {
        Metrics* newMetrics = tmpHead->CopyAndReset();
        newMetrics->SetNext(snapshot);
        snapshot = newMetrics;
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
    return snapshot;
}

ReadMetrics::ReadMetrics() {}

void ReadMetrics::ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup*>& logGroupMap) {
    ReadLock lock(mReadWriteLock);
    Metrics* tmp = mHead;
    while(tmp) {
        Log* logPtr = nullptr;
        for (auto &item: tmp->GetLabels()) {
            std::pair<std::string, std::string> pair = item;
            if (METRIC_FIELD_REGION == pair.first) {
                std::map<std::string, sls_logs::LogGroup*>::iterator iter;
                std::string region = pair.second;
                iter = logGroupMap.find(region);
                if (iter != logGroupMap.end()) {
                    sls_logs::LogGroup* logGroup = iter->second;
                    logPtr = logGroup->add_logs();
                } else {
                    sls_logs::LogGroup* logGroup = new sls_logs::LogGroup();
                    logPtr = logGroup->add_logs();
                    logGroupMap.insert(std::pair<std::string, sls_logs::LogGroup*>(region, logGroup));
                }
            }
        }
        if (!logPtr) {
            std::map<std::string, sls_logs::LogGroup*>::iterator iter;
            iter = logGroupMap.find(METRIC_REGION_DEFAULT);
            if (iter != logGroupMap.end()) {
                sls_logs::LogGroup* logGroup = iter->second;
                logPtr = logGroup->add_logs();
            } else {
                sls_logs::LogGroup* logGroup = new sls_logs::LogGroup();
                logPtr = logGroup->add_logs();
                logGroupMap.insert(std::pair<std::string, sls_logs::LogGroup*>(METRIC_REGION_DEFAULT, logGroup));
            }
        }
        auto now = GetCurrentLogtailTime();
        SetLogTime(logPtr, AppConfig::GetInstance()->EnableLogTimeAutoAdjust() ? now.tv_sec + GetTimeDelta() : now.tv_sec, now.tv_nsec);
        for (auto &item: tmp->GetLabels()) {
            std::pair<std::string, std::string> pair = item;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(LABEL_PREFIX + pair.first);
            contentPtr->set_value(pair.second);
        }

        for (auto &item: tmp->GetValues()) {
            MetricPtr counter = item;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(VALUE_PREFIX + counter->GetName());
            contentPtr->set_value(ToString(counter->GetValue()));
        }
        // set default key
        {
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(METRIC_TOPIC_FIELD_NAME);
            contentPtr->set_value(METRIC_TOPIC_TYPE);
        }
        tmp = tmp->GetNext();
    }
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

Metrics* ReadMetrics::GetHead() {
    return mHead;
}

}