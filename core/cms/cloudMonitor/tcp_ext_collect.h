//
// Created by 许刘泽 on 2021/2/20.
//

#ifndef ARGUSAGENT_SRC_CLOUDMONITOR_TCP_EXT_COLLECT_H_
#define ARGUSAGENT_SRC_CLOUDMONITOR_TCP_EXT_COLLECT_H_
#include <chrono>
#include "base_collect.h"
#include "metric_calculate_2.h"

namespace cloudMonitor {
    struct TcpExtMetric {
        double delayedACKs = 0;
        double listenOverflows = 0;
        double listenDrops = 0;
        double tcpPrequeued = 0;
        double tcpPrequeueDropped = 0;
        double tcpPureAcks = 0;
        double tcpFACKReorder = 0;
        double tcpSACKReorder = 0;
        double tcpLossProbes = 0;
        double tcpLossProbeRecovery = 0;
        double tcpLostRetransmit = 0;
        double tcpFastRetrans = 0;
        double tcpSlowStartRetrans = 0;
        double tcpTimeouts = 0;
    };
    // For MetricCalculate2<TcpExtMetric, double>
    void enumerate(const std::function<void(const MetricCalculate2<TcpExtMetric>::FieldMeta &)>&);

#include "common/test_support"

class TcpExtCollect : public BaseCollect {
public:
    TcpExtCollect();

    virtual ~TcpExtCollect();

    int Init(int totalCount);

    int Collect(std::string &data);

private:

    int ReadTcpExtFile(TcpExtMetric &tcpExtMetric);

    void Calculate();

    void GetCollectData(std::string &collectStr);

    const std::string mModuleName;
    TcpExtMetric mTcpExtMetric;
    TcpExtMetric mLastStat;
    TcpExtMetric mCurrentStat;
    MetricCalculate2<TcpExtMetric> mMetricCalculate;
    TcpExtMetric mMaxMetrics;
    TcpExtMetric mMinMetrics;
    TcpExtMetric mAvgMetrics;

    int mTotalCount = 0;
    int mCount = 0;
    bool mActivate = false;

    unsigned int collectCount = 0;
    std::chrono::steady_clock::time_point mLastTime;
    std::chrono::steady_clock::time_point mCurrentTime;

    const std::string TCP_EXT_FILE = "/proc/net/netstat";
};
#include "common/test_support"

}

#endif //ARGUSAGENT_SRC_CLOUDMONITOR_TCP_EXT_COLLECT_H_
