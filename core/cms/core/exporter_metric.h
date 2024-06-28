#ifndef ARGUS_CORE_EXPORTER_METRIC_H
#define ARGUS_CORE_EXPORTER_METRIC_H

#include  "base_parse_metric.h"

namespace argus {

#include "common/test_support"
class ExporterMetric : public BaseParseMetric {
public:
    ExporterMetric(const std::vector<MetricFilterInfo> &metricFilterInfos,
                   const std::vector<LabelAddInfo> &labelAddInfos);
    explicit ExporterMetric(const std::vector<LabelAddInfo> &labelAddInfos);
    ~ExporterMetric() override;

    void AddMetricExtra(const std::string &metricStr, bool haveTimestamp = false,
                        std::map<std::string, std::string> *labelMap = nullptr);
    ParseErrorType AddMetric(const std::string &metricStr) override;

private:
    static int ParseLine(const std::string &line, common::CommonMetric &commonMetric);
    int AddCommonMetric(const common::CommonMetric &commonMetric);
    // std::string GetMetricKey(const common::CommonMetric &commonMetric);
private:
    std::map<std::string, common::CommonMetric> mCommonMetricMap;
};

#include "common/test_support"

}
#endif
