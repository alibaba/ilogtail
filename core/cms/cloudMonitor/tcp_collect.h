//
// Created by yunjian.ph on 2022/2/11.
//

#ifndef ARGUS_CLOUD_MONITOR_TCP_COLLECT_H
#define ARGUS_CLOUD_MONITOR_TCP_COLLECT_H

#include <chrono>
#include "common/PluginUtils.h"
#include "metric_calculate_2.h"

#include "base_collect.h"

namespace cloudMonitor {
    struct TcpMetric {
        //需要check的
        double activeOpens = 0;
        double passiveOpens = 0;
        double inSegs = 0;
        double outSegs = 0;
        double estabResets = 0;
        double attemptFails = 0;
        double retransSegs = 0;
        double inErrs = 0;
        double outRsts = 0;
        //不需要check的
        double currEstab = 0;
        //ratio
        double retransRate = 0;
    };
    void enumerate(const std::function<void(const FieldName<TcpMetric> &)> &);

#include "common/test_support"

class TcpCollect : public BaseCollect {
public:
    TcpCollect();
    virtual ~TcpCollect();

    int Init(int totalCount);

    int Collect(std::string &data);

private:
    static int ReadSnmpFile(TcpMetric &tcpMetric, const std::string &filePath);
    void CheckTcpMetric(TcpMetric &currentMetric, const TcpMetric &lastMetric);
    void Calculate();
    void GetCollectData(const TcpMetric &max, const TcpMetric &min, const TcpMetric &avg, std::string &collectStr);

    const std::string mModuleName = "tcp";
    TcpMetric mtcpMetric;
    TcpMetric mLastStat;
    TcpMetric mCurrentStat;
    MetricCalculate2<TcpMetric> mMetricCalculate;
    // TcpMetric mMaxMetrics{};
    // TcpMetric mMinMetrics{};
    // TcpMetric mAvgMetrics{};

    std::chrono::steady_clock::time_point mLastTime;
    std::chrono::steady_clock::time_point mCurrentTime;
    int mTotalCount = 0;
    int mCount = 0;
    bool mActivate = false;
    bool mFirstCollect = true;
    bool mCheckStatus = true;

    const std::string SNMP_FILE = "/proc/net/snmp";
};

#include "common/test_support"

}

#endif //__CLOUD_MONITOR_TCP_COLLECT_H__
