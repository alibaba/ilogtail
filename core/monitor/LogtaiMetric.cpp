#include "LogtailMetric.h"
#include "common/StringTools.h"
#include "MetricConstants.h"
#include "logger/Logger.h"
#include "common/TimeUtil.h"
#include "app_config/AppConfig.h"


using namespace sls_logs;

namespace logtail {


const uint64_t MetricNameValue::GetValue() const {
    return mVal;
}

const std::string& MetricNameValue::GetName() const {
    return mName;
}

Counter::Counter(const std::string& name, uint64_t val = 0) : MetricNameValue(name, val) {
}

Counter* Counter::CopyAndReset() {
    return new Counter(mName, mVal.exchange(0));
}

void Counter::SetValue(uint64_t value) {
    mVal += value;
}

Gauge::Gauge(const std::string& name, uint64_t val = 0) : MetricNameValue(name, val) {
}

Gauge* Gauge::CopyAndReset() {
    return new Gauge(mName, mVal.exchange(0));
}

void Gauge::SetValue(uint64_t value) {
    mVal = value;
}

MetricsRecord::MetricsRecord(LabelsPtr labels) : mLabels(labels), mDeleted(false) {
}

MetricsRecord::MetricsRecord() : mDeleted(false) {
}

MetricNameValuePtr MetricsRecord::CreateCounter(const std::string& name) {
    MetricNameValuePtr counterPtr = std::make_shared<Counter>(name);
    mValues.emplace_back(counterPtr);
    return counterPtr;
}

MetricNameValuePtr MetricsRecord::CreateGauge(const std::string& name) {
    MetricNameValuePtr gaugePtr = std::make_shared<Gauge>(name);
    mValues.emplace_back(gaugePtr);
    return gaugePtr;
}

void MetricsRecord::MarkDeleted() {
    mDeleted = true;
}

bool MetricsRecord::IsDeleted() {
    return mDeleted;
}

LabelsPtr MetricsRecord::GetLabels() {
    return mLabels;
}

const std::vector<MetricNameValuePtr>& MetricsRecord::GetValues() const {
    return mValues;
}

MetricsRecord* MetricsRecord::CopyAndReset() {
    MetricsRecord* metrics = new MetricsRecord(mLabels);
    for (auto& item : mValues) {
        MetricNameValuePtr newPtr(item->CopyAndReset());
        metrics->mValues.emplace_back(newPtr);
    }
    return metrics;
}

MetricsRecord* MetricsRecord::GetNext() {
    return mNext;
}

void MetricsRecord::SetNext(MetricsRecord* next) {
    mNext = next;
}

MetricsRecordRef::MetricsRecordRef() {
}

MetricsRecordRef::~MetricsRecordRef() {
    if (mMetrics) {
        mMetrics->MarkDeleted();
    }
}

void MetricsRecordRef::Init(const std::vector<std::pair<std::string, std::string>>& labels) {
    if (!mMetrics) {
        mMetrics = WriteMetrics::GetInstance()->CreateMetrics(
            std::make_shared<std::vector<std::pair<std::string, std::string>>>(labels));
    }
}

MetricsRecord* MetricsRecordRef::operator->() const {
    return mMetrics;
}

WriteMetrics::WriteMetrics() {
}

WriteMetrics::~WriteMetrics() {
    while (mHead) {
        MetricsRecord* toDeleted = mHead;
        mHead = mHead->GetNext();
        delete toDeleted;
    }
}

MetricsRecord* WriteMetrics::CreateMetrics(LabelsPtr labels) {
    MetricsRecord* cur = new MetricsRecord(labels);
    std::lock_guard<std::mutex> lock(mMutex);
    cur->SetNext(mHead);
    mHead = cur;
    return cur;
}

MetricsRecord* WriteMetrics::GetHead() {
    return mHead;
}

MetricsRecord* WriteMetrics::DoSnapshot() {
    // new read head
    MetricsRecord* snapshot = nullptr;
    MetricsRecord* toDeleteHead = nullptr;
    MetricsRecord* tmp = nullptr;

    MetricsRecord emptyHead;
    MetricsRecord* preTmp = nullptr;

    MetricsRecord* tmpHead = nullptr;
    // find the first undeleted node and set as new mHead
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
                mHead = tmp;
                preTmp = tmp;
                tmp = tmp->GetNext();
                findHead = true;
                break;
            }
        }
        // if no undeleted node, set null to mHead
        if (!findHead) {
            mHead = nullptr;
        }
        tmpHead = mHead;
    }

    // copy head
    if (tmpHead) {
        MetricsRecord* newMetrics = tmpHead->CopyAndReset();
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
            MetricsRecord* newMetrics = tmp->CopyAndReset();
            newMetrics->SetNext(snapshot);
            snapshot = newMetrics;
            preTmp = tmp;
            tmp = tmp->GetNext();
        }
    }

    while (toDeleteHead) {
        MetricsRecord* toDeleted = toDeleteHead;
        toDeleteHead = toDeleteHead->GetNext();
        delete toDeleted;
    }
    return snapshot;
}

ReadMetrics::ReadMetrics() {
}

ReadMetrics::~ReadMetrics() {
    while (mHead) {
        MetricsRecord* toDeleted = mHead;
        mHead = mHead->GetNext();
        delete toDeleted;
    }
}

void ReadMetrics::ReadAsLogGroup(std::map<std::string, sls_logs::LogGroup*>& logGroupMap) {
    ReadLock lock(mReadWriteLock);
    MetricsRecord* tmp = mHead;
    while (tmp) {
        Log* logPtr = nullptr;
        // for (auto &item: tmp->GetLabels()) {
        for (auto item = tmp->GetLabels()->begin(); item != tmp->GetLabels()->end(); ++item) {
            std::pair<std::string, std::string> pair = *item;
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
        SetLogTime(logPtr,
                   AppConfig::GetInstance()->EnableLogTimeAutoAdjust() ? now.tv_sec + GetTimeDelta() : now.tv_sec,
                   now.tv_nsec);
        for (auto item = tmp->GetLabels()->begin(); item != tmp->GetLabels()->end(); ++item) {
            std::pair<std::string, std::string> pair = *item;
            Log_Content* contentPtr = logPtr->add_contents();
            contentPtr->set_key(LABEL_PREFIX + pair.first);
            contentPtr->set_value(pair.second);
        }

        for (auto& item : tmp->GetValues()) {
            MetricNameValuePtr counter = item;
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
    MetricsRecord* snapshot = WriteMetrics::GetInstance()->DoSnapshot();
    MetricsRecord* toDelete;
    {
        // Only lock when change head
        WriteLock lock(mReadWriteLock);
        toDelete = mHead;
        mHead = snapshot;
    }
    // delete old linklist
    while (toDelete) {
        MetricsRecord* obj = toDelete;
        toDelete = toDelete->GetNext();
        delete obj;
    }
}

MetricsRecord* ReadMetrics::GetHead() {
    return mHead;
}

} // namespace logtail