#pragma once
#include <string>
#include <unordered_map>
#include <list>

namespace logtail {

class ILogtailMetric {

private:
    ILogtailMetric();
    ~ILogtailMetric();


public:
    static ILogtailMetric* GetInstance() {
        static ILogtailMetric* ptr = new ILogtailMetric();
        return ptr;
    }

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
        std::string project;
        std::string logstore;
        std::string configName;
    };

    struct MetricGroup {
        GroupInfo* meta;
        std::list<Metric*> metrics;
    };

    typedef std::unordered_map<std::string, MetricGroup*> MetricGroupMap;
    MetricGroupMap mILogtailInstanceMetrics;
    MetricGroupMap mPluginMetrics;
    MetricGroupMap mSubPluginMetrics;

    MetricGroupMap GetInstanceMetrics();
    MetricGroupMap GetPluginMetrics();
    MetricGroupMap GetSubPluginMetrics();

    MetricGroup* CreateMetric(std::string uid, std::string type);
    void DeleteMetric(std::string uid, std::string type);
};
}