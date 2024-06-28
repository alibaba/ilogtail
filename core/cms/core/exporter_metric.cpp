#include "exporter_metric.h"
#include "common/StringUtils.h"
#include "common/PrometheusMetric.h"
#include "common/Chrono.h"

using namespace std;
using namespace common;

namespace argus {
    ExporterMetric::ExporterMetric(const vector <MetricFilterInfo> &metricFilterInfos,
                                   const vector <LabelAddInfo> &labelAddInfos)
            : BaseParseMetric(metricFilterInfos, labelAddInfos) {
    }

    ExporterMetric::ExporterMetric(const std::vector<LabelAddInfo> &labelAddInfos)
            : ExporterMetric(vector < MetricFilterInfo > {}, labelAddInfos) {
    }

    ExporterMetric::~ExporterMetric() = default;

    void ExporterMetric::AddMetricExtra(const string &metricStr, bool haveTimestamp, map <string, string> *labelMap) {
        vector<string> lines = StringUtils::split(metricStr, "\n", true);
        int64_t timestamp = NowMillis();
        for (const auto &line: lines) {
            CommonMetric commonMetric;
            if (ParseLine(line, commonMetric) == 0) {
                if (!haveTimestamp || commonMetric.timestamp == 0) {
                    commonMetric.timestamp = timestamp;
                }
                if (labelMap != nullptr) {
                    for (auto &it: *labelMap) {
                        commonMetric.tagMap[it.first] = it.second;
                    }
                }
                AddCommonMetric(commonMetric);
            }
        }
    }

    BaseParseMetric::ParseErrorType ExporterMetric::AddMetric(const string &metricStr) {
        if (metricStr.empty()) {
            return EMPTY_STRING;
        }

        vector<string> lines = StringUtils::split(metricStr, "\n", true);
        int64_t timestamp = NowMillis();
        size_t errorCount = 0;
        for (const auto &line: lines) {
            CommonMetric commonMetric;
            bool error = ParseLine(line, commonMetric) != 0;
            errorCount += (error ? 1 : 0);
            // if (ParseLine(line, commonMetric) != 0)
            // {
            //     errorFlag=true;
            //     continue;
            // }
            if (!error) {
                if (commonMetric.timestamp == 0) {
                    commonMetric.timestamp = timestamp;
                }
                AddCommonMetric(commonMetric);
            }
        }

        return errorCount == 0 ? PARSE_SUCCESS : PROMETHEUS_LINE_INVALID;
    }

    int ExporterMetric::ParseLine(const string &line, CommonMetric &commonMetric) {
        return PrometheusMetric::ParseMetric(line, commonMetric);
    }

    int ExporterMetric::AddCommonMetric(const CommonMetric &commonMetric) {
        string metricName;
        int ret = GetCommonMetricName(commonMetric, mMetricFilterInfoMap, metricName);
        if (ret < 0) {
            return -1;
        }
        if (metricName.empty()) {
            mCommonMetrics.push_back(commonMetric);
        } else {
            CommonMetric tmpCommonMetric = commonMetric;
            tmpCommonMetric.name = metricName;
            mCommonMetrics.push_back(tmpCommonMetric);
        }
        return 0;
    }
// string ExporterMetric::GetMetricKey(const CommonMetric &commonMetric)
// {
//     string metricKey = commonMetric.name;
//     for (const auto & it : commonMetric.tagMap)
//     {
//         metricKey += it.first + it.second;
//     }
//     return metricKey;
// }
}