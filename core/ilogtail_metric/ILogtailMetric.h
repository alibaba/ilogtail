#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include "logger/Logger.h"
#include <MetricConstants.h>


namespace logtail {


class BaseMetric{

    public:
        BaseMetric();
        //~BaseMetric();
        struct MetricObj {
            /* counters and gauges */
            uint64_t val;

            uint64_t timestamp;

            /* internal */
            std::unordered_map<std::string, std::string> labels;
        };

        MetricObj* baseMetric;
        void baseMetricAdd(uint64_t val);
        MetricObj* getMetricObj();
};

class SubPluginMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> mBaseMetrics;
        std::unordered_map<std::string, std::string> mLabels;

        std::unordered_map<std::string, BaseMetric*> getBaseMetrics();
        //std::unordered_map<std::string, std::string> getLabels();

};

class PluginMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> mBaseMetrics;
        std::unordered_map<std::string, std::string> mLabels;
        std::unordered_map<std::string, SubPluginMetric*> mSubMetrics;

        std::unordered_map<std::string, BaseMetric*> getBaseMetrics();
};

class PipelineMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> mBaseMetrics;
        std::unordered_map<std::string, std::string> mLabels;
        std::unordered_map<std::string, PluginMetric*> mPluginMetrics;

        std::unordered_map<std::string, BaseMetric*> getBaseMetrics();
};


class ILogtailMetric {

private:
    ILogtailMetric();
    ~ILogtailMetric();


public:
    static ILogtailMetric* GetInstance() {
        static ILogtailMetric* ptr = new ILogtailMetric();
        return ptr;
    }

    std::unordered_map<std::string, PipelineMetric*> mPipelineMetrics;
    PluginMetric* mInstanceMetric;  

    void createInstanceMetric(PipelineMetric* groupMetric, std::string id);

    PipelineMetric* getPipelineMetric(std::string configUid);

    PluginMetric* getPluginMetric(PipelineMetric* pipelineMetric, std::string pluginUid); 

    SubPluginMetric* getFileSubMetric(PluginMetric* pluginMetric, std::string filePath); 

    BaseMetric* getBaseMetric(SubPluginMetric* fileMetric, std::string metricField);
};
}