#ifndef CMS_PROCESS_STAT_COLLECT_H
#define CMS_PROCESS_STAT_COLLECT_H
#include <map>
#include <chrono>
#include "base_collect.h"
#include "core/TaskManager.h"

namespace argus {
//     struct ProcessCollectItem;
    struct tagMetricLimit;
}
namespace common{
    struct CollectData;
}

namespace cloudMonitor {
    struct SystemTaskInfo {
        uint64_t threadCount = 0;
        uint64_t processCount = 0;
        uint64_t zombieProcessCount = 0;
    };

    struct tagPidTotal {
        pid_t pid = 0;
        uint64_t total = 0;

        tagPidTotal() = default;

        tagPidTotal(pid_t p, uint64_t t) : pid(p), total(t) {}
    };
    struct ProcessStat {
        SicProcessState processState;
        ProcessInfo processInfo;
        SicProcessCpuInformation processCpu;
        SicProcessMemoryInformation processMemory;
        double memPercent = 0.0;
        uint64_t fdNum = 0;
        bool fdNumExact = true;
    };
    struct ProcessMatchInfo: argus::ProcessCollectItem {
        // std::string user;
        // std::string name;
        // std::string keyword;
        // std::vector<argus::TagItem> tags;
        std::vector<pid_t> pids;

        bool isMatch(const ProcessInfo &processInfo) const;
        bool appendIfMatch(const ProcessInfo &processInfo);
    };

    struct ProcessSelfInfo {
        pid_t pid = 0;
        //系统时间，代码为秒
        std::string sysUpTime;
        std::string sysRunTime;
        //agent cpu指标
        std::string startTime;
        std::string runTime;
        uint64_t cpuTotal = 0;
        double cpuPercent = 0;
        //agent 内存指标
        uint64_t memResident = 0;
        uint64_t memSize = 0;
        uint64_t memPrivate = 0;
        uint64_t memShare = 0;
        double memPercent = 0;

        uint64_t openfiles = 0;
        bool openFilesExact = true;
        uint64_t threadCount = 0;
        std::string version;

        //agent进程参数
        std::string exeName;
        std::string exeCwd;
        std::string exeRoot;
        std::string exeArgs;
        //采集指标
        uint64_t collectCount = 0;
        double lastCollectCost = 0;
        std::string lastCollectTime;
        std::string curCollectTime;
        //数据上报指标
        int lastCommitCode = 0;
        double lastCommitCost = 0;
        std::string lastCommitMsg;
        //心跳数据统计
        uint64_t putMetricFailCount = 0;
        uint64_t putMetricSuccCount = 0;
        double putMetricFailPerMinute = 0;
        //心跳数据统计
        uint64_t pullConfigFailCount = 0;
        uint64_t pullConfigSuccCount = 0;
        double pullConfigFailPerMinute = 0;
        //自我状态监控
        int coredumpCount = 0;
        int restartCount = 0;
        int resourceExceedCount = 0;
    };

#include "common/test_support"

class ProcessCollect : public BaseCollect {
public:
    enum EnumType {
        Cpu, Fd, Memory
    };
    static EnumType ParseTopType(const std::string &);
    ProcessCollect();

    int Init(int topN, EnumType topType = Cpu, bool saveFullProcessMetric = false);

    ~ProcessCollect();

    int Collect(std::string &data);

    using BaseCollect::GetProcessPids;
    void collectTopN(const std::vector<pid_t> &pids, common::CollectData &collectData);

    static int toolCollectTopN(const char *argv0, int argc, const char * const *argv, std::ostream &);

private:

    void CopyAndSortByCpu(const std::vector<pid_t> &pids, std::vector<pid_t> &sortPids);

    int GetSystemTask(const std::vector<pid_t> &pids, SystemTaskInfo &systemTaskInfo);

    int GetPidsCpu(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap);
    int GetPidsFd(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap);
    int GetPidsMem(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap);

    int GetProcessStat(pid_t pid, ProcessStat &processStat);

    int UpdateTotalMemory(uint64_t &totalMemory);

    int GetTopNProcessStat(std::vector<pid_t> &sortPids, int topN, std::vector<ProcessStat> &processStats);

    void GetProcessMatchInfos(const std::vector<pid_t> &pids, std::vector<ProcessMatchInfo> &processMatchInfo);

    int GetProcessSelfInfo(ProcessSelfInfo &processSelfInfo);

    static void GetSystemTaskMetricData(const std::string &metricName,
                                 const SystemTaskInfo &systemTaskInfo,
                                 common::MetricData &metricData);

    static void GetOldSystemTaskMetricData(const std::string &metricName,
                                    const SystemTaskInfo &systemTaskInfo,
                                    common::MetricData &metricData);

    static void GetProcessStatMetricData(const std::string &metricName,
                                         const ProcessStat &processStat,
                                         common::MetricData &metricData);

    static void GetProcessMatchInfoMetricData(const std::string &metricName,
                                       const ProcessMatchInfo &processMatchInfo,
                                       common::MetricData &metricData);

    static void GetOldProcessMatchInfoMetricData(const std::string &metricName,
                                          const ProcessMatchInfo &processMatchInfo,
                                          common::MetricData &metricData);

    void GetProcessSelfInfoMetricData(const std::string &metricName,
                                      const ProcessSelfInfo &processSelfInfo,
                                      common::MetricData &metricData) const;

    static std::string ToJson(const std::vector<pid_t> &pids);

    void collectMatched(const std::vector<pid_t> &pids, common::CollectData &collectData);
    void collectSelf(common::CollectData &collectData);
    void collectSysTasks(const std::vector<pid_t> &pids, common::CollectData &collectData);
private:
    const std::string mModuleName;
    const pid_t mSelfPid;
    const pid_t mParentPid;
    size_t maxProcessCount = 5000;
    std::vector<pid_t> mSortPids;
    int mTopN = 0;
    std::chrono::steady_clock::time_point mProcessSortCollectTime;
    std::shared_ptr<std::map<pid_t, uint64_t>> mLastPidCpuMap;
    uint64_t mTotalMemory = 0;
    uint64_t mCollectCount = 0;
    std::chrono::system_clock::time_point mLastCollectTime;
    std::chrono::steady_clock::time_point mLastCollectSteadyTime;
    std::chrono::duration<double, std::milli> mLastCostTime{0};
    decltype(SicProcessCpuInformation{}.total) mLastAgentTotalMillis = 0;
    int mProcessSilentCount = 0;

    std::function<int(const std::vector<pid_t> &, std::map<pid_t, uint64_t> &)> fnGetPidsMetric;
    std::function<uint64_t(uint64_t, uint64_t)> fnTopValue;
    EnumType mTopType = Cpu;
#ifdef WIN32
    uint32_t mPerProcessSilentFactor = 0;
#endif

    std::shared_ptr<argus::tagMetricLimit> mLimit;
};
#include "common/test_support"

}
#endif