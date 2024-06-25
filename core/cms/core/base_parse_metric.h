#ifndef _ARGUS_BASE_PARSE_METRIC_H_
#define _ARGUS_BASE_PARSE_METRIC_H_

#include <map>
#include <vector>
#include <string>

#include "TaskManager.h"
#include "common/CommonMetric.h"

namespace argus {
    struct LabelGetter {
        virtual ~LabelGetter() = default;
        virtual std::string get(const std::string &key) const;
    };

#include "common/test_support"
class BaseParseMetric {
public:
    enum ParseErrorType {
        PARSE_SUCCESS = 0,
        PROMETHEUS_LINE_INVALID,
        ALIMONITOR_JSON_RESULT_NOT_JSON,
        ALIMONITOR_JSON_RESULT_NOT_OBJECT,
        ALIMONITOR_JSON_MSG_INVALID,
        ALIMONITOR_JSON_MSG_ARRAY_ELEMENT_INVALID,
        ALIMONITOR_JSON_MSG_MEMBER_INVALID,
        ALIMONITOR_TEXT_TIMESTAMP_INVALID,
        ALIMONITOR_TEXT_KEY_VALUE_PAIR_INVALID,
        EMPTY_STRING,
        PARSE_FAILED,
        PARSE_ERROR_TYPE_END
    };
    BaseParseMetric(const std::vector<MetricFilterInfo> &metricFilterInfos,
                    const std::vector<LabelAddInfo> &labelAddInfos);
    virtual ~BaseParseMetric() = default;

    virtual enum ParseErrorType AddMetric(const std::string &metricStr) { return PARSE_SUCCESS; };
    virtual int GetCommonMetrics(std::vector<common::CommonMetric> &commonMetrics);
    static bool ParseAddLabelInfo(const std::vector<LabelAddInfo> &labelAddInfos,
                                  std::map<std::string, std::string> &addTagMap,
                                  std::map<std::string, std::string> *renameTagMap,
                                  const LabelGetter *labelGetter = nullptr);
    //返回-1则该metric被过滤掉，metricName不需要了。
    static int GetCommonMetricName(const common::CommonMetric &commonMetric,
                                   const std::map<std::string, MetricFilterInfo> &metricFilterInfoMap,
                                   std::string &metricName);
public:
    const static int ParseErrorTypeNum;
    const static std::string ParseErrorString[];
protected:
    std::map<std::string, MetricFilterInfo> mMetricFilterInfoMap;
    std::map<std::string, std::string> mAddTagMap;
    std::map<std::string, std::string> mRenameTagMap;
    std::vector<common::CommonMetric> mCommonMetrics;
};
#include "common/test_support"

} // namespace argus
#endif
