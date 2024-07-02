#ifndef ARGUS_SELF_MONITOR_H
#define ARGUS_SELF_MONITOR_H

#include <vector>
#include <chrono>
#include <functional>
#include "common/ThreadWorker.h"
#include "common/CommonMetric.h"
#include "sic/system_information_collector.h"

#ifdef WIN32
#   define defaultFdLimit 700
#else
#   define defaultFdLimit 300
#endif

class SystemInformationCollector;

namespace argus {
    struct tagMetricLimit {
        double cpuPercent = 0.5;
        uint64_t rssMemory = 200 * 1024 * 1024;
        int fdCount = defaultFdLimit;
    };
    tagMetricLimit GetConfigLimit();

    struct ProcessStat : SicProcessTime {
        int pid = 0;
        //单位为Byte
        uint64_t rss = 0;

        int fdNum = 0;
    };

#include "common/test_support"
class SelfMonitor : public common::ThreadWorker {
public:
    explicit SelfMonitor(bool end = true);
    ~SelfMonitor() override;
    void Start();

private:
    void doRun() override;
    void GetAllAgentStatusMetrics(std::vector<common::CommonMetric> &metrics) const;
    int SendResourceExceedsNonCms();
    static void GetAgentStatus(const std::vector<common::CommonMetric> &metrics, common::CommonMetric &agentMetric);

    static bool GetProcessStat(int pid, ProcessStat &processStat);

    void doRun(const std::function<void(int)> &fnExit);
private:
    std::string mSn;
    std::chrono::steady_clock::time_point mLastTime;
    std::chrono::milliseconds mInterval{15 * 1000}; // 秒级间隔
    int mSelfPid = 0;
    std::chrono::milliseconds mLastCpu{0};
    tagMetricLimit mLimit;
    // double mMaxCpuPercent = 0;
    // uint64_t mMaxRssMemory = 0;
    int mCpuExceedCount = 0;
    int mMemoryExceedCount = 0;
    int mFdExceedCount = 0;
    int mMaxCount = 0;
    // int mMaxFdCount = 0;
    int mAgentStatusCount = 0;
    int mAgentStatusNumber = 0;
#if defined(__linux__) || defined(__unix__)
    int mMallocFreeCount = 0;
#endif
};
#include "common/test_support"

}
#endif