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
        BaseMetric(MetricObj*  metricObj);
        ~BaseMetric();
        MetricObj* mMetricObj;
        void baseMetricAdd(uint64_t val);
        MetricObj*  getMetricObj();
        MetricObj*  snapShotMetricObj();
};

typedef std::shared_ptr<BaseMetric> BaseMetricPtr;


class PipelineMetric {
    public:
        std::unordered_map<std::string, BaseMetricPtr> mBaseMetrics;
        std::unordered_map<std::string, std::string> mLabels;

        BaseMetricPtr getBaseMetric(std::string metricField);

};

typedef std::shared_ptr<PipelineMetric> PipelineMetricPtr;

class ILogtailMetric {

    private:
        ILogtailMetric();
        ~ILogtailMetric();


    public:
        static ILogtailMetric* GetInstance() {
            static ILogtailMetric* ptr = new ILogtailMetric();
            return ptr;
        }

        std::mutex mMetricsLock;

        std::list<PipelineMetricPtr> mPipelineMetrics;
        PipelineMetricPtr mInstanceMetric;

        PipelineMetricPtr createPipelineMetric(std::list<std::string> fields , std::unordered_map<std::string, std::string> labels);

        PipelineMetricPtr createFileMetric(std::string configUid, std::string pluginUid, std::string filePath);

        BaseMetricPtr getBaseMetric(PipelineMetricPtr pipelineMetric, std::string metricField);

        void deletePipelineMetric(PipelineMetricPtr pipelineMetric);

    };
}