#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <metric/Metric.h>

namespace logtail {


class GroupMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> baseMetrics;
        std::unordered_map<std::string, std::string> labels;
        std::unordered_map<Std::string, Metric*> metrics;
};

class Metric {
    public:
        std::unordered_map<std::string, BaseMetric*> baseMetrics;
        std::unordered_map<std::string, std::string> labels;
        std::unordered_map<Std::string, SubMetric*> subMetrics;

        std::unordered_map<std::string, BaseMetric*> getBaseMetrics();
        std::unordered_map<std::string, string> getLabels();
};

class SubMetric {
    public:
        std::unordered_map<std::string, BaseMetric*> mBaseMetrics;
        std::unordered_map<std::string, string> mLabels;

        std::unordered_map<std::string, BaseMetric*> getBaseMetrics();
        std::unordered_map<std::string, string> getLabels();

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

    std::unordered_map<std::string, GroupMetric*> pipelineMetrics;
    Metric* instanceMetric;  

    Metric* createInstanceMetric();
    SubMetric* createFileSubMetric(Metric* Metric, std::string filePath); 

};
}