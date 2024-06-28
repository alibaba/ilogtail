#ifndef _SYSTEM_COLLECT_H_
#define _SYSTEM_COLLECT_H_

#include <string>
#include "base_collect.h"
#include "metric_calculate_2.h"

namespace cloudMonitor {
    void enumerate(const std::function<void(const FieldName<SicLoadInformation> &)> &);

#include "common/test_support"
class SystemCollect : public BaseCollect {
public:
    SystemCollect();

    int Init(int totalCount);

    ~SystemCollect();

    int Collect(std::string &data);

private:

    static int GetMetricData(const std::string &metricName,
                             double metricValue,
                             const std::string &tagName,
                             const std::string &tagValue,
                             common::MetricData &metricData);

    static int GetMetricData(const SicLoadInformation &minLoadStat,
                             const SicLoadInformation &maxLoadStat,
                             const SicLoadInformation &avgLoadStat,
                             int64_t uptimeSeconds,
                             common::MetricData &metricData,
                             const std::string &metricName);

    static int GetMetricDataPerCore(const SicLoadInformation &minLoadStat,
                                    const SicLoadInformation &maxLoadStat,
                                    const SicLoadInformation &avgLoadStat,
                                    const std::string &metricName,
                                    common::MetricData &metricData);

private:
    const std::string mModuleName;
    int mTotalCount = 0;
    int mCount = 0;
    MetricCalculate2<SicLoadInformation> mMetricCalculate;
};
#include "common/test_support"

}
#endif