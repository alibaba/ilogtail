#include "ILogtailMetric.h"

namespace logtail {


ILogtailMetric::ILogtailMetric() {
}

ILogtailMetric::~ILogtailMetric() {
}

BaseMetric::BaseMetric() {
    baseMetric = new MetricObj;
    baseMetric->val = 0;
    baseMetric->timestamp = time(NULL);
}

void BaseMetric::baseMetricAdd(uint64_t val) {
    baseMetric->val += val;
    baseMetric->timestamp = time(NULL);
}

BaseMetric::MetricObj* BaseMetric::getMetricObj() {
    return baseMetric;
}

std::unordered_map<std::string, BaseMetric*> PipelineMetric::getBaseMetrics() {
    return mBaseMetrics;
}


std::unordered_map<std::string, BaseMetric*> PluginMetric::getBaseMetrics() {
    return mBaseMetrics;
}


std::unordered_map<std::string, BaseMetric*> SubPluginMetric::getBaseMetrics() {
    return mBaseMetrics;
}


void ILogtailMetric::createInstanceMetric(PipelineMetric* groupMetric, std::string id) {
    ILogtailMetric::mInstanceMetric = new PluginMetric();
    //ILogtailMetric::instanceMetric->getLabels().insert(std::make_pair("instance", id));

    //ILogtailMetric::instanceMetric->getBaseMetrics().insert(std::make_pair("cpu", NULL));
    //ILogtailMetric::instanceMetric->getBaseMetrics().insert(std::make_pair("memory", NULL));
}


PipelineMetric* ILogtailMetric::getPipelineMetric(std::string configUid) {
    std::unordered_map<std::string, PipelineMetric*>::iterator iter = ILogtailMetric::mPipelineMetrics.find(configUid);
    if (iter != mPipelineMetrics.end()) {
        return iter->second;
    } else {
        PipelineMetric* pipelineMetric = new PipelineMetric();
        pipelineMetric->mLabels.insert(std::pair<std::string, std::string>("configUid", configUid));

        mPipelineMetrics.insert(std::pair<std::string, PipelineMetric*>(configUid, pipelineMetric));
        return pipelineMetric;
    }
}

PluginMetric* ILogtailMetric::getPluginMetric(PipelineMetric* pipelineMetric, std::string pluginUid) {
    std::unordered_map<std::string, PluginMetric*>::iterator iter = pipelineMetric->mPluginMetrics.find(pluginUid);
     if (iter != pipelineMetric->mPluginMetrics.end()) {
        return iter->second;
    } else {
        PluginMetric* pluginMetric = new PluginMetric();
        pluginMetric->mLabels.insert(std::pair<std::string, std::string>("pluginUid", pluginUid));
        pipelineMetric->mPluginMetrics.insert(std::pair<std::string, PluginMetric*>(pluginUid, pluginMetric));
        return pluginMetric;
    }
}


SubPluginMetric* ILogtailMetric::getFileSubMetric(PluginMetric* pluginMetric, std::string filePath) {
    std::unordered_map<std::string, SubPluginMetric*>::iterator iter = pluginMetric->mSubMetrics.find(filePath);
    if (iter != pluginMetric->mSubMetrics.end()) {
        return iter->second;
    } else {
        SubPluginMetric* fileSubMetric = new SubPluginMetric();

        LOG_INFO(sLogger, ("label size before", fileSubMetric->mLabels.size()));
        fileSubMetric->mLabels.insert(std::pair<std::string, std::string>("filename", filePath));
        LOG_INFO(sLogger, ("label size before", fileSubMetric->mLabels.size()));

        fileSubMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(METRIC_FILE_READ_COUNT, new BaseMetric()));
        fileSubMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(METRIC_FILE_READ_BYTES, new BaseMetric()));
        
        pluginMetric->mSubMetrics.insert(std::pair<std::string, SubPluginMetric*>(filePath, fileSubMetric));
        return fileSubMetric;
    }
}


BaseMetric* ILogtailMetric::getBaseMetric(SubPluginMetric* subMetric, std::string metricField) {
    LOG_INFO(sLogger, ("lgetBaseMetric", "start"));
    std::unordered_map<std::string, BaseMetric*>::iterator iter = subMetric->mBaseMetrics.find(metricField);
    if (iter != subMetric->mBaseMetrics.end()) {
        return iter->second;
    } else {
        BaseMetric* base = new BaseMetric();
        subMetric->mBaseMetrics.insert(std::pair<std::string, BaseMetric*>(metricField, base));
        return base;
    }
}

}