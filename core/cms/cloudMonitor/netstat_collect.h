#ifndef _NET_STAT_COLLECT_H_
#define _NET_STAT_COLLECT_H_

#include <string>
#include <map>
#include "base_collect.h"

namespace cloudMonitor {

#include "common/test_support"
class NetStatCollect : public BaseCollect {
public:
    NetStatCollect();

    int Init();

    ~NetStatCollect();

    int Collect(std::string &data);

private:

    static void GetNetStatMetricData(const NetStat &netStat,
                              const std::string &metricName,
                              const std::string &ns,
                              int index,
                              common::MetricData &metricData);

    static void GetNetStatMetricData(const std::string &metricName, int udpSession, common::MetricData &metricData);

private:
    const std::string mModuleName;
    bool mUseTcpAllState = false;
};
#include "common/test_support"

}
#endif