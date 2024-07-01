#ifndef _MEMORY_COLLECT_H_
#define _MEMORY_COLLECT_H_

#include <string>
#include "base_collect.h"

namespace cloudMonitor {

#include "common/test_support"
class MemoryCollect : public BaseCollect {
public:
    MemoryCollect();

    ~MemoryCollect();

    int Collect(std::string &data);
    int Init() {
        return 0;
    }

private:
    static void GetMemoryMetricData(const SicMemoryInformation &memoryStat,
                                    const std::string &metricName,
                                    common::MetricData &metricData);

    static void GetSwapMetricData(const SicSwapInformation &swapStat,
                                  const std::string &metricName,
                                  common::MetricData &metricData);

    static void GetOldMetricData(double usedPercent, const std::string &metricName, common::MetricData &metricData);

private:
    const std::string mModuleName;
};
#include "common/test_support"

}
#endif
