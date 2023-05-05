#pragma once
#include <string>
#include <unordered_map>

namespace logtail {

class ILogtailMetric {

struct Metric {
    std::string name;
    std::string unit;
    std::string tags;
    std::string description;
    std::string metricType;
    float value;
    uint64_t observedTimestamp;
    uint64_t timestamp;
};

struct GroupInfo {

};

struct MetricGroup {
    GroupInfo* meta;
    Metric* metrics[];
};

private:
    ILogtailMetric();
    ~ILogtailMetric();
    typedef std::unordered_map<std::string, MetricGroup*> MetricGroupMap;
    MetricGroupMap mILogtailInstanceMetrics;
    MetricGroupMap mPluginMetrics;
    MetricGroupMap mSubPluginMetrics;

public:
    static ILogtailMetric* GetInstance() {
        static ILogtailMetric* ptr = new ILogtailMetric();
        return ptr;
    }

    MetricGroupMap GetInstanceMetrics();
    MetricGroupMap GetPluginMetrics();
    MetricGroupMap GetSubPluginMetrics();

    MetricGroup* CreateMetric(std::string uid, std::string type);
    void DeleteMetric(std::string uid, std::string type);
};
}