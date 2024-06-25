#ifndef _ALI_SCRIPT_METRIC_H_
#define _ALI_SCRIPT_METRIC_H_
#include <vector>
#include <string>
#include <map>

#include "base_parse_metric.h"
#include "TaskManager.h"

namespace json {
    class Object;
}
namespace argus {

#include "common/test_support"
class AliScriptMetric : public BaseParseMetric {
public:
    AliScriptMetric(const std::vector<MetricFilterInfo> &metricFilterInfos,
                    const std::vector<LabelAddInfo> &labelAddInfos,
                    const std::vector<MetricMeta> &metricMetas,
                    enum ScriptResultFormat resultFormat);
    ~AliScriptMetric();
    enum ParseErrorType AddMetric(const std::string &metricStr) override;
    void GetJsonLabelAddInfos(std::vector<LabelAddInfo> &labelAddInfos) const;
private:
    int AddCommonMetric(const common::CommonMetric &commonMetric);
    enum ParseErrorType AddJsonMetric(const std::string &metricStr);
    enum ParseErrorType AddJsonMsgObject(const json::Object &msgValue);
    enum ParseErrorType AddTextMetric(const std::string &metricStr);
    void AddConstructMetrics(const std::vector<std::pair<std::string, std::string>> &stringLabels,
                             const std::vector<std::pair<std::string, double> > &numberMetrics);
    static std::string GetMetricKey(const common::CommonMetric &commonMetric);
    static std::string StandardizePythonJson(const std::string &oldStr);
private:
    std::map<std::string, common::CommonMetric> mCommonMetricMap;
    enum ScriptResultFormat mResultFormat;
    std::map<std::string, MetricMeta> mMetricMetaMap;
    //未用锁保护，不应多线程读写。
    std::vector<LabelAddInfo> mJsonlabelAddInfos;
};
#include "common/test_support"

}
#endif
