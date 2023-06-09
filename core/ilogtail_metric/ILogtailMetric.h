#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <atomic>
#include "logger/Logger.h"
#include <MetricConstants.h>


namespace logtail {


class BaseMetric{
    struct MetricObj {
        /* counters and gauges */
        std::atomic_long val;

        std::atomic_long timestamp;
    };
    public:
        BaseMetric();
        BaseMetric(MetricObj* metricObj);
        //~BaseMetric();
        MetricObj* mMetricObj;
        void baseMetricAdd(uint64_t val);
        MetricObj* getMetricObj();
        MetricObj* snapShotMetricObj();
};

class PipelineMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> mBaseMetrics;
        std::unordered_map<std::string, std::string> mLabels;

        BaseMetric* getBaseMetric(std::string metricField);

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

    std::list<PipelineMetric*> mPipelineMetrics;
    PipelineMetric* mInstanceMetric;  

    PipelineMetric* createPipelineMetric(std::list<std::string> fields , std::unordered_map<std::string, std::string> labels);

    PipelineMetric* createFileMetric(std::string configUid, std::string pluginUid, std::string filePath);

    BaseMetric* getBaseMetric(PipelineMetric* pipelineMetric, std::string metricField);

};
}