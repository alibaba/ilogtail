#include "ILogtailMetric.h"

namespace logtail {


ILogtailMetric::ILogtailMetric() {
}

ILogtailMetric::~ILogtailMetric() {
}

BaseMetric::BaseMetric() {
    mMetricObj = new MetricObj;
    mMetricObj->val = {0};
    mMetricObj->timestamp = time(NULL);
}

BaseMetric::BaseMetric(MetricObj*  metricObj) {
    mMetricObj = metricObj;
}

BaseMetric::~BaseMetric() {
    if (mMetricObj) {
        delete mMetricObj;
    }
}


void BaseMetric::baseMetricAdd(uint64_t val) {
    mMetricObj->val += val;
    mMetricObj->timestamp = {time(NULL)};
}

BaseMetric::MetricObj*  BaseMetric::snapShotMetricObj() {
    MetricObj*  newMetricObj = new MetricObj;

    LOG_INFO(sLogger, ("mMetricObj->val exchange before", mMetricObj->val));
    long value = mMetricObj->val.exchange(0);
    newMetricObj->val = {value};
    LOG_INFO(sLogger, ("mMetricObj->val exchange after", mMetricObj->val));

    LOG_INFO(sLogger, ("mMetricObj->timestamp exchange after", mMetricObj->timestamp));
    newMetricObj->timestamp = {time(NULL)};
    LOG_INFO(sLogger, ("mMetricObj->timestamp exchange after", mMetricObj->timestamp));

    return newMetricObj;
}

BaseMetric::MetricObj*  BaseMetric::getMetricObj() {
    return mMetricObj;
}


BaseMetricPtr PipelineMetric::getBaseMetric(std::string metricField) {
    LOG_INFO(sLogger, ("getBaseMetric", metricField));
    std::unordered_map<std::string, BaseMetricPtr>::iterator iter = mBaseMetrics.find(metricField);
    if (iter != mBaseMetrics.end()) {
        return iter->second;
    } else {
        BaseMetricPtr base(new BaseMetric());
        mBaseMetrics.insert(std::pair<std::string, BaseMetricPtr>(metricField, base));
        return base;
    }
}


PipelineMetricPtr ILogtailMetric::createPipelineMetric(std::list<std::string> fields , std::unordered_map<std::string, std::string> labels) {
    PipelineMetricPtr fileMetric(new PipelineMetric());

    LOG_INFO(sLogger, ("label size before", fileMetric->mLabels.size()));

    fileMetric->mLabels = labels;

    LOG_INFO(sLogger, ("label size before", fileMetric->mLabels.size()));

    for (std::list<std::string>::iterator it = fields.begin(); it != fields.end(); ++it) {
        BaseMetricPtr base(new BaseMetric());
        fileMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetricPtr>(*it, base));
    }
    std::lock_guard<std::mutex> lock(mMetricsLock);
    mPipelineMetrics.push_back(fileMetric);
    return fileMetric;
}

void ILogtailMetric::deletePipelineMetric(PipelineMetricPtr pipelineMetric) {
    std::lock_guard<std::mutex> lock(mMetricsLock);
    mPipelineMetrics.remove(pipelineMetric);
}


PipelineMetricPtr ILogtailMetric::createFileMetric(std::string configUid, std::string pluginUid, std::string filePath) {
    std::unordered_map<std::string, std::string> labels;
    LOG_INFO(sLogger, ("label size before", labels.size()));

    labels.insert(std::pair<std::string, std::string>("configUid", configUid));
    labels.insert(std::pair<std::string, std::string>("pluginUid", pluginUid));
    labels.insert(std::pair<std::string, std::string>("filePath", filePath));

    LOG_INFO(sLogger, ("label size before", labels.size()));
    std::list<std::string> fields;
    fields.push_back(METRIC_FILE_READ_COUNT);
    fields.push_back(METRIC_FILE_READ_BYTES);
    return createPipelineMetric(fields, labels);
}


BaseMetricPtr ILogtailMetric::getBaseMetric(PipelineMetricPtr pipelineMetric, std::string metricField) {
    LOG_INFO(sLogger, ("getBaseMetric", metricField));
    std::unordered_map<std::string, BaseMetricPtr>::iterator iter = pipelineMetric->mBaseMetrics.find(metricField);
    if (iter != pipelineMetric->mBaseMetrics.end()) {
        return iter->second;
    } else {
        BaseMetricPtr base(new BaseMetric());
        pipelineMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetricPtr>(metricField, base));
        return base;
    }
}

}