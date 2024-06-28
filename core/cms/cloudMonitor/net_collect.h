#ifndef _NET_COLLECT_H_
#define _NET_COLLECT_H_

#include <string>
#include <map>
#include <chrono>

#include "base_collect.h"
#include "metric_calculate_2.h"
#include "sic/system_information_collector.h"

namespace cloudMonitor {
    struct NetMetric : NetInterfaceMetric<double> {
        double rxErrorRatio = 0;
        double rxDropRatio = 0;
        double txErrorRatio = 0;
        double txDropRatio = 0;
    };
    // For MetricCalculate2<CpuMetric, double>
    void enumerate(const std::function<void(const FieldName<NetMetric, double> &)> &);

#include "common/test_support"
class NetCollect : public BaseCollect {
public:
    NetCollect();
    ~NetCollect();

    int Init(int totalCount);
    int Collect(std::string &data);

    int Exit();

private:
    void ReadInterfaceConfigs(std::map<std::string, InterfaceConfig> &interfaceConfigMap);

    static void GetNetMetric(const SicNetInterfaceInformation &currentStat,
                             const SicNetInterfaceInformation &lastStat,
                             NetMetric &netMetric,
                             double interval);

    static bool GetDeviceIp(const std::map<std::string, InterfaceConfig> &interfaceConfigMap,
                            const std::string &deviceName,
                            std::string &deviceIp);

    static void GetMetricData(const NetMetric &maxNetMetric,
                              const NetMetric &minNetMetric,
                              const NetMetric &avgNetMetric,
                              const std::string &metricName,
                              const std::string &deviceIp,
                              const std::string &deviceName,
                              common::MetricData &metricData);

    static void GetMetricData(const std::string &metricName, const std::string &deviceName, double metricValue,
                              common::MetricData &metricData);

private:
    std::map<std::string, MetricCalculate2<NetMetric, double>> mMetricCalculateMap;
    std::map<std::string, SicNetInterfaceInformation> mLastInterfaceStatMap;
    std::map<std::string, InterfaceConfig> mInterfaceConfigMap;
    std::chrono::steady_clock::time_point mInterfaceConfigExpireTime;
    std::chrono::steady_clock::time_point mLastTime;
    int mTotalCount = 0;
    int mCount = 0;
    const std::string mModuleName;
};
#include "common/test_support"

}
#endif 