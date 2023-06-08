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

void BaseMetric::baseMetricAdd(uint64_t val) {
    mMetricObj->val += val;
    mMetricObj->timestamp = time(NULL);
}

BaseMetric::MetricObj* BaseMetric::getMetricObj() {
    return mMetricObj;
}


BaseMetric* PipelineMetric::getBaseMetric(std::string metricField) {
    LOG_INFO(sLogger, ("getBaseMetric", metricField));
    std::unordered_map<std::string, BaseMetric*>::iterator iter = mBaseMetrics.find(metricField);
    if (iter != mBaseMetrics.end()) {
        return iter->second;
    } else {
        BaseMetric* base = new BaseMetric();
        mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(metricField, base));
        return base;
    }
}


PipelineMetric* ILogtailMetric::createPipelineMetric(std::list<std::string> fields , std::unordered_map<std::string, std::string> labels) {
    PipelineMetric* fileMetric = new PipelineMetric();

    LOG_INFO(sLogger, ("label size before", fileMetric->mLabels.size()));

    fileMetric->mLabels = labels;

    LOG_INFO(sLogger, ("label size before", fileMetric->mLabels.size()));

    for (std::list<std::string>::iterator it = fields.begin(); it != fields.end(); ++it) {
        fileMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(*it, new BaseMetric()));
    }
    mPipelineMetrics.push_back(fileMetric);
    return fileMetric;
}


PipelineMetric* ILogtailMetric::createFileMetric(std::string configUid, std::string pluginUid, std::string filePath) {
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


BaseMetric* ILogtailMetric::getBaseMetric(PipelineMetric* pipelineMetric, std::string metricField) {
    LOG_INFO(sLogger, ("getBaseMetric", metricField));
    std::unordered_map<std::string, BaseMetric*>::iterator iter = pipelineMetric->mBaseMetrics.find(metricField);
    if (iter != pipelineMetric->mBaseMetrics.end()) {
        return iter->second;
    } else {
        BaseMetric* base = new BaseMetric();
        pipelineMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(metricField, base));
        return base;
    }
}
}