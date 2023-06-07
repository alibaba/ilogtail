#include "ILogtailMetric.h"

namespace logtail {


ILogtailMetric::ILogtailMetric() {
}

ILogtailMetric::~ILogtailMetric() {
}



std::unordered_map<std::string, BaseMetric*> SubMetric::getBaseMetrics() {
    return SubMetric::mBaseMetrics;
}

std::unordered_map<std::string, std::string> SubMetric::getLabels() {
    return SubMetric::mLabelsl
}


void ILogtailMetric::createInstanceMetric(GroupMetric* groupMetric, std::string id) {
    ILogtailMetric::instanceMetric = new Metric();
    ILogtailMetric::instanceMetric->getLabels().insert(std::make_pair("instance", id));

    ILogtailMetric::instanceMetric->getBaseMetrics().insert(std::make_pair("cpu"), NULL);
    ILogtailMetric::instanceMetric->getBaseMetrics().insert(std::make_pair("memory"), NULL);
}

SubMetric* ILogtailMetric::createFileSubMetric(Metric* metric, std::string filePath) {
    SubMetric* fileSubMetric = new SubMetric();
    fileSubMetric->getLabels().insert(std::make_pair("filename", filePath));

    //fileSubMetric->getBaseMetrics()["file_read_count"] = NULL;
    //fileSubMetric->getBaseMetrics()["file_read_bytes"] = NULL;
    return fileSubMetric;
}
}