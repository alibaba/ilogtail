#include "ILogtailMetric.h"

namespace logtail {

ILogtailMetric::ILogtailMetric() {
}

ILogtailMetric::~ILogtailMetric() {
}

ILogtailMetric::MetricGroupMap ILogtailMetric::GetInstanceMetrics() {
    return mILogtailInstanceMetrics;
}

ILogtailMetric::MetricGroupMap ILogtailMetric::GetPluginMetrics() {
    return mPluginMetrics;
}

ILogtailMetric::MetricGroupMap ILogtailMetric::GetSubPluginMetrics() {
    return mSubPluginMetrics;
}


ILogtailMetric::MetricGroup* ILogtailMetric::CreateMetric(std::string uid, std::string type) {
    std::unordered_map<std::string,  MetricGroup*>::iterator iter = mILogtailInstanceMetrics.find(uid);
    if (iter == mILogtailInstanceMetrics.end()) {
        MetricGroup *s = new(MetricGroup);
        iter = mILogtailInstanceMetrics.insert(iter, std::make_pair(uid, s));
        return s;
    } else {
        return iter->second;
    }
}

void ILogtailMetric::DeleteMetric(std::string uid, std::string type) {

}

}