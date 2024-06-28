#include "common/ThrowWithTrace.h"
#include "process_collect.h"

#include <algorithm>
#include <thread>
#include <boost/program_options.hpp>

#include "common/Common.h" // getVersionDetail
#include "common/CpuArch.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/CommonUtils.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"
#include "common/JsonValueEx.h"
#include "common/Config.h"
#include "core/agent_status.h"
#include "core/TaskManager.h"
#include "core/self_monitor.h"
#include "sic/system_information_collector_util.h"
#include "common/Chrono.h"

#include "cloud_monitor_config.h"
#include "cloud_monitor_common.h"
#include "collect_info.h"

using namespace std;
using namespace argus;
using namespace common;
using namespace std::chrono;
using namespace std::placeholders;

//topN进行的缓存为55s
const std::chrono::seconds ProcessSortInterval{55};

namespace cloudMonitor {
    static const auto &topTypeMap = *new std::map<std::string, ProcessCollect::EnumType>{
            {"cpu",    ProcessCollect::Cpu},
            {"fd",     ProcessCollect::Fd},
            {"mem",    ProcessCollect::Memory},
            {"memory", ProcessCollect::Memory},
    };

    ProcessCollect::EnumType ProcessCollect::ParseTopType(const std::string &s) {
        const auto it = topTypeMap.find(StringUtils::ToLower(s));
        if (it != topTypeMap.end()) {
            return it->second;
        }
        ThrowWithTrace<std::runtime_error>("unknown TopType({})", s);
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    int ProcessCollect::toolCollectTopN(const char *argv0, int argc, const char * const *argv, std::ostream &os) {
        namespace po = boost::program_options;
        po::options_description desc(std::string{"Usage: "} + argv0 + " tool top [options]\nAllowed options");
        int topN = 5;
        std::string orderBy{"fd"};
        desc.add_options()
                ("help,h", "produce help message of tool top")
                ("number,n", po::value(&topN)->default_value(topN), "top N processes")
                ("by,b", po::value(&orderBy)->default_value(orderBy), "cpu, mem, memory, fd")
                ("fast", "ignore cpu metric in output"); // 不输出cpu指标

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            os << desc << std::endl;
            return 0;
        }

        ProcessCollect procCollector;
        int code = procCollector.Init(topN, ParseTopType(orderBy));
        if (code != 0) {
            return -1;
        }
        std::cout << "top" << topN << " by " << orderBy << std::endl;
        CollectData collectData;
        std::vector<pid_t> pids;

        procCollector.UpdateTotalMemory(procCollector.mTotalMemory); // 更新系统内存大小
        auto collect = [&]() {
            if (pids.empty()) {
                procCollector.GetProcessPids(pids, 0, nullptr);
            }
            collectData.dataVector.clear();
            procCollector.collectTopN(pids, collectData);
        };

        collect();
        // cpu需要两次采集
        if (procCollector.mTopType == Cpu || vm.count("fast") == 0) {
            std::cout << "." << std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds{1100});
            collect();
            std::cout << "." << std::endl;
        }

        auto pretty = json::Pretty{};
        pretty.indentation = "  ";
        os << "[" << std::endl;
        for (const auto &data: collectData.dataVector) {
            os << pretty.indentation;
            pretty.print(os, boost::json::value_from(data.toJson()), pretty.indentation);
            // os << "  " <<  boost::json::serialize(data.toJson());
            if (&data != &collectData.dataVector.back()) {
                os << ",";
            }
            os << std::endl;
        }
        os << "]" << std::endl;
        return 0;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    ProcessCollect::ProcessCollect() : mModuleName{"process"}, mSelfPid{GetPid()}, mParentPid{GetParentPid()} {
        auto cfg = SingletonConfig::Instance();
        cfg->GetValue("agent.process.collect.count.limit", maxProcessCount, maxProcessCount);
        fnGetPidsMetric = std::bind(&ProcessCollect::GetPidsCpu, this, _1, _2);
        fnTopValue = [](uint64_t cur, uint64_t prev) { return cur - prev; };
        LogInfo("load {}", mModuleName);
        // windows机器一般不会有这么多进程，所以这个限制主要对linux机器
        cfg->GetValue("cms.process.collect.silent.count", mProcessSilentCount, 1000);
#ifdef WIN32
        //windows only
        cfg->GetValue("cms.per.process.collect.silent.interval", mPerProcessSilentFactor, 0);
#endif
        mLimit = std::make_shared<tagMetricLimit>(argus::GetConfigLimit());
    }

    // https://aone.alibaba-inc.com/v2/project/899492/req/52293287# 《【稳定性】argus某一个版本开始，支持全进程监控》
    int ProcessCollect::Init(int topN, EnumType topType, bool) {
        mTopN = topN;
        mTopType = topType;
        if (topType == Cpu) {
            fnGetPidsMetric = std::bind(&ProcessCollect::GetPidsCpu, this, _1, _2);
            fnTopValue = [](uint64_t cur, uint64_t prev) { return cur - prev; };
        } else if (topType == Fd) {
            fnGetPidsMetric = std::bind(&ProcessCollect::GetPidsFd, this, _1, _2);
            fnTopValue = [](uint64_t cur, uint64_t prev) { return cur; };
        } else if (topType == Memory) {
            fnGetPidsMetric = std::bind(&ProcessCollect::GetPidsMem, this, _1, _2);
            fnTopValue = [](uint64_t cur, uint64_t prev) { return cur; };
        } else {
            LogError("unknown EnumTopType: {}", int(topType));
            return -1;
        }
        mProcessSortCollectTime = steady_clock::time_point{}; // 清零
        mTotalMemory = 0;
        mCollectCount = 0;
        return 0;
    }

    ProcessCollect::~ProcessCollect() {
        LogInfo("unload {}", mModuleName);
    }

    void ProcessCollect::collectTopN(const std::vector<pid_t> &pids, CollectData &collectData) {
        //排序
        vector<pid_t> sortPids;
        CopyAndSortByCpu(pids, sortPids);  // 进程按cpu排序

        TimeProfile tp;
        // 获取topN进程的信息
        vector<ProcessStat> topN;
        topN.reserve(mTopN);
        GetTopNProcessStat(sortPids, mTopN, topN);
        LogDebug("get top{} processes cost: {},", mTopN, tp.cost<fraction_millis>());

        //topN数据格式化
        for (const auto &detail: topN) {
            MetricData metricDataProcessStat;
            GetProcessStatMetricData("system.process.detail", detail, metricDataProcessStat);
            collectData.dataVector.push_back(metricDataProcessStat);
        }
    }

    // 进程监控
    void ProcessCollect::collectMatched(const std::vector<pid_t> &pids, CollectData &collectData) {
        TimeProfile tp;
        // 进程监控
        //获取进程匹配数据
        vector<ProcessMatchInfo> processMatchInfos;
        GetProcessMatchInfos(pids, processMatchInfos);
        LogDebug("get {} process match infos cost: {},", processMatchInfos.size(), tp.cost<fraction_millis>());

        //进程匹配数据格式化
        for (auto &processMatchInfo: processMatchInfos) {
            MetricData sysProcessNumber;
            GetProcessMatchInfoMetricData("system.process.number", processMatchInfo, sysProcessNumber);
            collectData.dataVector.push_back(sysProcessNumber);

            // 兼容旧指标
            MetricData vmProcessNumber;
            GetOldProcessMatchInfoMetricData("vm.process.number", processMatchInfo, vmProcessNumber);
            collectData.dataVector.push_back(vmProcessNumber);
        }
    }

    void ProcessCollect::collectSelf(CollectData &collectData) {
        TimeProfile tp;
        // agent自监控
        ProcessSelfInfo processSelfInfo;
        GetProcessSelfInfo(processSelfInfo);
        LogDebug("get process self info cost: {:2}", tp.cost<fraction_millis>());

        //agent自身数据格式化
        MetricData processSelfInfoMetricData;
        GetProcessSelfInfoMetricData("system.process.agent", processSelfInfo, processSelfInfoMetricData);
        collectData.dataVector.push_back(processSelfInfoMetricData);
    }

    void ProcessCollect::collectSysTasks(const std::vector<pid_t> &pids, CollectData &collectData) {
        //统计任务数(只有linux统计)
        //系统任务数据格式化
#if !defined(WIN32)
        SystemTaskInfo systemTaskInfo;
        GetSystemTask(pids, systemTaskInfo);
        LogDebug("system has {} process, {} threads, {} zombie process",
                 systemTaskInfo.processCount, systemTaskInfo.threadCount, systemTaskInfo.zombieProcessCount);

#ifdef ENABLE_CMS_SYS_TASK // metrichub已不支持system.task
        MetricData metricDataSystemTask;
        GetSystemTaskMetricData("system.task", systemTaskInfo, metricDataSystemTask);
        collectData.dataVector.push_back(metricDataSystemTask);
#endif

        MetricData oldMetricDataSystemTask;
        GetOldSystemTaskMetricData("vm.ProcessCount", systemTaskInfo, oldMetricDataSystemTask);
        collectData.dataVector.push_back(oldMetricDataSystemTask);
#endif // !Win32
    }
    // pid  - argusagent 自身
    // ppid - argusagent -d 进程
    static void removePid(std::vector<pid_t> &pids, pid_t pid, pid_t ppid) {
        size_t count = pids.size();
        for (size_t i = 0; i < count;) {
            pid_t curPid = pids[i];
            if (curPid == pid || curPid == ppid) {
                count--;
                pids[i] = pids[count];
            } else {
                ++i;
            }
        }
        if (count != pids.size()) {
            pids.resize(count);
        }
    }

    int ProcessCollect::Collect(string &data) {
        TimeProfile tpCost;
        defer(LogDebug("Collect({} processes), total cost {},", mLastPidCpuMap->size(), tpCost.cost<fraction_millis>()));

        mCollectCount++;
        steady_clock::time_point startTime = tpCost.lastTime();
        //更新总内存
        TimeProfile tp;
        if (UpdateTotalMemory(mTotalMemory) != 0) {
            return -1;
        }
        LogDebug("update total memory cost: {},", tp.cost<fraction_millis>());
        //采集所有的进程pid列表
        vector<pid_t> pids;
        bool isOverflow = false;
        // 溢出时依然返回0
        if (GetProcessPids(pids, maxProcessCount, &isOverflow) != SIC_EXECUTABLE_SUCCESS) {
            return -1;
        }
        if (isOverflow) {
            LogWarn("total process count({}) exceeds limit({}), to avoid performance risk, process collection stopped",
                    pids.size(), maxProcessCount);
            return -1;
        }
        removePid(pids, mSelfPid, mParentPid);
        LogDebug("get process pids cost: {},", tp.cost<fraction_millis>());
        //格式化采集数据
        CollectData collectData;
        collectData.moduleName = mModuleName;

        collectTopN(pids, collectData);
        LogDebug("collectTop {} cost: {},", mTopN, tp.cost<fraction_millis>());
        collectMatched(pids, collectData);
        LogDebug("collectMatched cost: {},", tp.cost<fraction_millis>());
        collectSelf(collectData);
        LogDebug("collectSelf cost: {},", tp.cost<fraction_millis>());
        collectSysTasks(pids, collectData);
        LogDebug("collectSysTasks cost: {},", tp.cost<fraction_millis>());

        ModuleData::convertCollectDataToString(collectData, data);
        mLastCollectTime = system_clock::now();
        mLastCollectSteadyTime = steady_clock::now();
        mLastCostTime = mLastCollectSteadyTime - startTime;
        return static_cast<int>(data.size());
    }

    int ProcessCollect::UpdateTotalMemory(uint64_t &totalMemory) {
        //更新下totalMemory
        SicMemoryInformation memoryStat;
        if (GetMemoryStat(memoryStat) != 0) {
            return -1;
        }
        totalMemory = memoryStat.total;
        return 0;
    }

    int ProcessCollect::GetSystemTask(const std::vector<pid_t> &pids, SystemTaskInfo &systemTaskInfo) {
        for (pid_t pid: pids) {
            SicProcessState processState;
            if (GetProcessState(pid, processState) == 0) {
                systemTaskInfo.threadCount += processState.threads;
                if (processState.state == SIC_PROC_STATE_ZOMBIE) {
                    systemTaskInfo.zombieProcessCount++;
                }
            }
        }
        systemTaskInfo.processCount = pids.size();
        return 0;
    }

    static bool compare(const tagPidTotal &p1, const tagPidTotal &p2) {
        return p1.total > p2.total || (p1.total == p2.total && p1.pid < p2.pid);
    }

    static bool comparePointer(const tagPidTotal *p1, const tagPidTotal *p2) {
        return compare(*p1, *p2);
    }

    void ProcessCollect::CopyAndSortByCpu(const std::vector<pid_t> &pids, std::vector<pid_t> &sortPids) {
        steady_clock::time_point now = steady_clock::now();
        if (mProcessSortCollectTime + ProcessSortInterval > now) {
            sortPids = mSortPids;
            return;
        }

        TimeProfile tpTotal;
        defer(LogDebug("CopyAndSortByCpu({} processes), total cost {},",
                       mLastPidCpuMap->size(), tpTotal.cost<fraction_millis>()));

        vector<tagPidTotal> sortPidInfos;
        TimeProfile tp;
        {
            auto currentPidMap = std::make_shared<map<pid_t, uint64_t>>();
            fnGetPidsMetric(pids, *currentPidMap);

            if (!mLastPidCpuMap) {
                mLastPidCpuMap = currentPidMap;
                if (mTopType == Cpu) {
                    // cpu是增量指标，第一次只能做为基础值，此时无法进行排序
                    LogInfo("CopyAndSortByCpu with the first time");
                    return;
                }
            }
            LogDebug("{}({} processes), top{}, collect cost: {},", __FUNCTION__,
                     pids.size(), currentPidMap->size(), tp.cost<fraction_millis>());

            sortPidInfos.reserve(currentPidMap->size());
            for (auto const &curIt: *currentPidMap) {
                const pid_t pid = curIt.first;

                auto const prevIt = mLastPidCpuMap->find(pid);
                if (prevIt != mLastPidCpuMap->end() && curIt.second >= prevIt->second) {
                    // tagPidTotal pidSortInfo;
                    // pidSortInfo.pid = pid;
                    // pidSortInfo.total = curIt.second - prevIt->second;
                    sortPidInfos.emplace_back(pid, fnTopValue(curIt.second, prevIt->second));
                }
            }
            LogDebug("{}({} processes), copy cost: {},", __FUNCTION__, pids.size(), tp.cost<fraction_millis>());
            mLastPidCpuMap = currentPidMap; // currentPidCpuMap使用完毕
            LogDebug("{}({} processes), cache cost: {},", __FUNCTION__, pids.size(), tp.cost<fraction_millis>());
        }
        const size_t pidCount = sortPidInfos.size();
        vector<tagPidTotal *> sortPidInfo2;
        sortPidInfo2.resize(sortPidInfos.size());
        for (size_t i = 0; i < pidCount; i++) {
            sortPidInfo2[i] = &sortPidInfos[i];
        }
        sort(sortPidInfo2.begin(), sortPidInfo2.end(), comparePointer);
        LogDebug("{}({} processes), sort cost: {},", __FUNCTION__, pids.size(), tp.cost<fraction_millis>());

        mSortPids.clear();
        mSortPids.reserve(sortPidInfo2.size());
        for (const auto &sortPidInfo: sortPidInfo2) {
            mSortPids.push_back(sortPidInfo->pid);
        }
        sortPids = mSortPids;
        mProcessSortCollectTime = steady_clock::now();
    }

    int ProcessCollect::GetPidsCpu(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap) {
        int readCount = 0;
        for (pid_t pid: pids) {
            if (++readCount > mProcessSilentCount) {
                readCount = 0;
                std::this_thread::sleep_for(milliseconds{100});
            }

            SicProcessCpuInformation procCpu;
            if (0 == GetProcessCpu(pid, procCpu, false)) {
                pidMap[pid] = procCpu.total;
            }
        }
        return 0;
    }

    int ProcessCollect::GetPidsFd(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap) {
        int readCount = 0;
        for (pid_t pid: pids) {
            if (++readCount > mProcessSilentCount) {
                readCount = 0;
                std::this_thread::sleep_for(milliseconds{100});
            }
            SicProcessFd procFd;
            if (GetProcessFdNumber(pid, procFd) == 0) {
                pidMap[pid] = procFd.total;
            }
        }
        return 0;
    }

    int ProcessCollect::GetPidsMem(const std::vector<pid_t> &pids, std::map<pid_t, uint64_t> &pidMap) {
        int readCount = 0;
        for (pid_t pid: pids) {
            if (++readCount > mProcessSilentCount) {
                readCount = 0;
                std::this_thread::sleep_for(milliseconds{100});
            }

            SicProcessMemoryInformation procMem;
            if (0 == GetProcessMemory(pid, procMem)) {
                pidMap[pid] = procMem.resident;
            }
        }
        return 0;
    }

    int ProcessCollect::GetProcessStat(pid_t pid, ProcessStat &processStat) {
        TimeProfile tp;
        int ret = GetProcessCpu(pid, processStat.processCpu, false);
        LogDebug("get process({}) cpu cost: {},, success: {}", pid, tp.cost<fraction_millis>(), (ret == 0));
        if (ret != 0) {
            return -1;
        }

        ret = GetProcessState(pid, processStat.processState);
        LogDebug("get process({}) state cost: {},, success: {}", pid, tp.cost<fraction_millis>(), (ret == 0));
        if (ret != 0) {
            return -1;
        }

        ret = GetProcessMemory(pid, processStat.processMemory);
        LogDebug("get process({}) memory cost: {},, success: {}", pid, tp.cost<fraction_millis>(), (ret == 0));
        if (ret != 0) {
            return -1;
        }

        SicProcessFd procFd;
        int res = GetProcessFdNumber(pid, procFd);
        LogDebug("get process({}) fd num cost: {},, success: {}, fdNum: {}{}", pid, tp.cost<fraction_millis>(),
                 (res == 0), processStat.fdNum, (procFd.exact ? "" : "+"));
        if (res != SIC_EXECUTABLE_SUCCESS) {
            return -1;
        }
        processStat.fdNum = procFd.total;
        processStat.fdNumExact = procFd.exact;  // Linux下超10000就不再统计，此时exact为false

        res = GetProcessInfo(pid, processStat.processInfo);
        LogDebug("get process({}) info cost: {},, success: {}", pid, tp.cost<fraction_millis>(), (res == 0));
        if (res != 0) {
            return -1;
        }
        if ((processStat.processInfo.name.empty() || processStat.processInfo.name == "unknown") &&
            !processStat.processState.name.empty()) {
            processStat.processInfo.name = "[" + processStat.processState.name + "]";
        }

        processStat.memPercent = mTotalMemory == 0 ? 0 : 100.0 * processStat.processMemory.resident / mTotalMemory;
        return 0;
    }

    int ProcessCollect::GetTopNProcessStat(vector <pid_t> &sortPids, int topN, vector <ProcessStat> &processStats) {
        processStats.clear();
        processStats.reserve(topN);
        for (pid_t pid: sortPids) {
            TimeProfile tp;
            ProcessStat processStat;
            if (GetProcessStat(pid, processStat) == 0) {
                processStats.push_back(processStat);
                auto cost = tp.cost<fraction_millis>();
                LogDebug("get top[{}] process({}) cost: {}", processStats.size(), pid, cost);
                if (processStats.size() >= (size_t) topN) {
                    break;
                }
#ifdef WIN32
                if(mPerProcessSilentFactor > 0){
                    LogDebug("get one top[{}] process state finished, will sleep: {} ms", processStats.size(),
                             cost.count() * mPerProcessSilentFactor/1000);
                    auto ms = microseconds{ static_cast<int64_t>(cost.count()) * 1000 * mPerProcessSilentFactor };
                    std::this_thread::sleep_for(ms);
                }
#endif
            }
        }
        return 0;
    }

    void ProcessCollect::GetProcessMatchInfos(const std::vector<pid_t> &pids,
                                              std::vector<ProcessMatchInfo> &matchInfos) {
        // 进程监控的配置
        std::shared_ptr<std::vector<ProcessCollectItem>> mProcessCollectItems;
        SingletonTaskManager::Instance()->ProcessCollectItems().Get(mProcessCollectItems);
        if (!mProcessCollectItems || mProcessCollectItems->empty()) {
            return;
        }

        matchInfos.clear();
        matchInfos.reserve(mProcessCollectItems->size());
        for (const auto &item: *mProcessCollectItems) {
            ProcessMatchInfo processMatchInfo;
            dynamic_cast<ProcessCollectItem &>(processMatchInfo) = item;
            processMatchInfo.name = StringUtils::ToLower(processMatchInfo.name);

            matchInfos.push_back(processMatchInfo);
        }
        int readCount = 0;
        for (pid_t pid: pids) {
            if (++readCount > mProcessSilentCount) {
                readCount = 0;
                std::this_thread::sleep_for(100_ms);
            }
            ProcessInfo processInfo;
            if (GetProcessInfo(pid, processInfo) != 0) {
                continue;
            }
            // LogDebug("get process(pid:{}) match info cost: {}", pid, tp.cost());
            for (ProcessMatchInfo &cur: matchInfos) {
                cur.appendIfMatch(processInfo);
            }
        }
    }

    bool ProcessMatchInfo::isMatch(const ProcessInfo &pi) const {
        if (!this->processName.empty() && this->processName != pi.name) {
            return false;
        }
        if (!this->processUser.empty() && this->processUser != pi.user) {
            return false;
        }
        if (!this->name.empty()) {
            const std::string *keys[] = {&pi.path, &pi.name, &pi.user, &pi.args,};
            bool matched = false;
            for (size_t i = 0; !matched && i < sizeof(keys) / sizeof(keys[0]); i++) {
                matched = StringUtils::Contain(StringUtils::ToLower(*keys[i]), this->name);
            }
            if (!matched) {
                return false;
            }
        }
        return !this->isEmpty();
    }
    bool ProcessMatchInfo::appendIfMatch(const ProcessInfo &processInfo) {
        bool match = isMatch(processInfo);
        if (match) {
            pids.push_back(processInfo.pid);
        }
        return match;
    }

    int ProcessCollect::GetProcessSelfInfo(ProcessSelfInfo &self) {
        pid_t pid = mSelfPid;
        self.pid = pid;

        TimeProfile tp;
        microseconds upTime = GetUpMicros(); // 系统启动到现在经历的微秒数
        LogDebug("get GetUpMicros(), cost: {},", tp.cost<fraction_millis>());
        microseconds now = duration_cast<microseconds>(system_clock::now().time_since_epoch());
        self.sysUpTime = CommonUtils::GetTimeStr((now - upTime).count());
        self.sysRunTime = StringUtils::NumberToString(upTime.count() / (1000 * 1000)) + "s";
        LogDebug("get self process sysUptime cost: {},, sysUpTime: {}, sysRunTime: {}",
                 tp.cost<fraction_millis>(), self.sysUpTime, self.sysRunTime);

        SicProcessCpuInformation processCpu;
        if (GetProcessCpu(pid, processCpu, true) == 0) {
            //不能直接使用percent，因为TOPN时可以计算了agent，秒级以内读取进程CPU使用率会导致数据不准;
            self.startTime = CommonUtils::GetTimeStr(processCpu.startTime * 1000);
            self.runTime =
                    StringUtils::NumberToString((now.count() - processCpu.startTime * 1000) / (1000 * 1000)) + "s";
            if (IsZero(mLastCollectSteadyTime)) {
                self.cpuPercent = 0.0;
            } else {
                auto dura = steady_clock::now() - mLastCollectSteadyTime;
                double percent = 1000.0 * (processCpu.total - mLastAgentTotalMillis) / ToMicros(dura);
                self.cpuPercent = 100.0 * percent;
            }
            mLastAgentTotalMillis = processCpu.total;
            self.cpuTotal = processCpu.total;
        }
        LogDebug("get self process cpu cost: {},, cpuPercent: {:.2f}%", tp.cost<fraction_millis>(), self.cpuPercent);
        SicProcessMemoryInformation processMemory;
        // curTime = NowMicros();
        if (GetProcessMemory(pid, processMemory) == 0) {
            self.memResident = processMemory.resident;
            self.memSize = processMemory.size;
            if (processMemory.share < mTotalMemory) {
                self.memPrivate = processMemory.resident - processMemory.share;
                self.memShare = processMemory.share;
            }
            self.memPercent = mTotalMemory > 0 ? 100.0 * processMemory.resident / mTotalMemory : 0;
        }
        LogDebug("get self process memory cost: {},", tp.cost<fraction_millis>());
        // curTime = NowMicros();
        SicProcessFd procFd;
        int res = GetProcessFdNumber(pid, procFd);
        LogDebug("get self process fd cost: {},", tp.cost<fraction_millis>());
        if (res == 0) {
            self.openfiles = procFd.total;
            self.openFilesExact = procFd.exact;
        }
        ProcessInfo processInfo;
        if (GetProcessInfo(pid, processInfo) == 0) {
            self.exeName = processInfo.path;
            self.exeCwd = processInfo.cwd;
            self.exeRoot = processInfo.root;
            self.exeArgs = processInfo.args;
        }
        LogDebug("get self process info cost: {},", tp.cost<fraction_millis>());
        SicProcessState processState;
        // curTime = NowMicros();
        if (GetProcessState(pid, processState) == 0) {
            self.threadCount = processState.threads;
        }
        LogDebug("get self process state cost: {},", tp.cost<fraction_millis>());

        self.version = CollectInfo::GetVersion();
        self.collectCount = mCollectCount;
        self.lastCollectCost = mLastCostTime.count();
        self.lastCollectTime = CommonUtils::GetTimeStr(ToMicros(mLastCollectTime));
        self.curCollectTime = CommonUtils::GetTimeStr(NowMicros());

        CollectInfo *pCollectInfo = SingletonCollectInfo::Instance();
        self.lastCommitCode = pCollectInfo->GetLastCommitCode();
        self.lastCommitCost = pCollectInfo->GetLastCommitCost();
        self.lastCommitMsg = pCollectInfo->GetLastCommitMsg();

        self.pullConfigFailCount = pCollectInfo->GetPullConfigFailCount();
        self.pullConfigFailPerMinute = pCollectInfo->GetPullConfigFailPerMinute();
        self.pullConfigSuccCount = pCollectInfo->GetPullConfigSuccCount();

        self.putMetricFailCount = pCollectInfo->GetPutMetricFailCount();
        self.putMetricFailPerMinute = pCollectInfo->GetPutMetricFailPerMinute();
        self.putMetricSuccCount = pCollectInfo->GetPutMetricSuccCount();
        self.resourceExceedCount = SingletonAgentStatus::Instance()->GetAgentStatus(AGENT_RESOURCE_STATUS);
        self.coredumpCount = SingletonAgentStatus::Instance()->GetAgentStatus(AGENT_COREDUMP_STATUS);
        self.restartCount = SingletonAgentStatus::Instance()->GetAgentStatus(AGENT_START_STATUS);
        return 0;
    }

    void ProcessCollect::GetSystemTaskMetricData(const string &metricName,
                                                 const SystemTaskInfo &systemTaskInfo,
                                                 MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["process_count"] = static_cast<double>(systemTaskInfo.processCount);
        metricData.valueMap["thread_count"] = static_cast<double>(systemTaskInfo.threadCount);
        metricData.valueMap["zombie_process_count"] = static_cast<double>(systemTaskInfo.zombieProcessCount);
        metricData.valueMap["metricValue"] = 0;
    }

    void ProcessCollect::GetOldSystemTaskMetricData(const string &metricName,
                                                    const SystemTaskInfo &systemTaskInfo,
                                                    MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["ns"] = "acs/ecs";
        metricData.valueMap["metricValue"] = static_cast<double>(systemTaskInfo.processCount);
    }

    void ProcessCollect::GetProcessStatMetricData(const string &metricName,
                                                  const ProcessStat &processStat,
                                                  MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ns"] = "acs/host";
        metricData.tagMap["name"] = processStat.processInfo.name;
        metricData.tagMap["args"] = processStat.processInfo.args;
        metricData.tagMap["path"] = processStat.processInfo.path;
        metricData.tagMap["user"] = processStat.processInfo.user;
        metricData.valueMap["cpu"] = processStat.processCpu.percent * 100.0;
        metricData.valueMap["mem"] = processStat.memPercent;
        metricData.valueMap["mem_share"] = static_cast<double>(processStat.processMemory.share);
        metricData.valueMap["mem_size"] = static_cast<double>(processStat.processMemory.resident);
        metricData.valueMap["openfiles"] = static_cast<double>(processStat.fdNum);
        metricData.valueMap["is_openfiles_exact"] = (processStat.fdNumExact ? 1 : 0);
        // https://aone.alibaba-inc.com/v2/project/460851/task/49881791#《当pid为2135683时，存入valueMap后，由于精度原因变成了2135680》
        metricData.tagMap["pid"] = convert(processStat.processInfo.pid);
    }

    void tags2Metric(const std::vector<TagItem> &tags, MetricData &metricData) {
        for (const auto &tagItem: tags) {
            metricData.tagMap["__tag__" + tagItem.key] = tagItem.value;
        }
    }
    void ProcessCollect::GetProcessMatchInfoMetricData(const string &metricName,
                                                       const ProcessMatchInfo &processMatchInfo,
                                                       MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = static_cast<double>(processMatchInfo.pids.size());
        metricData.tagMap["ns"] = "acs/host";
        metricData.tagMap["user"] = processMatchInfo.processUser;
        metricData.tagMap["name"] = processMatchInfo.processName;
        metricData.tagMap["keyword"] = processMatchInfo.name;
        metricData.tagMap["process_id"] = ToJson(processMatchInfo.pids);
        tags2Metric(processMatchInfo.tags, metricData);
    }

    void ProcessCollect::GetOldProcessMatchInfoMetricData(const string &metricName,
                                                          const ProcessMatchInfo &processMatchInfo,
                                                          MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = static_cast<double>(processMatchInfo.pids.size());
        metricData.tagMap["processName"] = processMatchInfo.name;
        metricData.tagMap["ns"] = "acs/ecs";
        tags2Metric(processMatchInfo.tags, metricData);
    }

    string ProcessCollect::ToJson(const std::vector<pid_t> &pids) {
        vector<string> pidStrs;
        pidStrs.reserve(pids.size());
        for (pid_t pid: pids) {
            pidStrs.push_back(convert(pid));
        }
        return StringUtils::join(pidStrs, ",", "[", "]", false);
    }

    void ProcessCollect::GetProcessSelfInfoMetricData(const string &metricName,
                                                      const ProcessSelfInfo &processSelfInfo,
                                                      MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["pid"] = processSelfInfo.pid;
        metricData.tagMap["sys_run_time"] = processSelfInfo.sysRunTime;
        metricData.tagMap["sys_up_time"] = processSelfInfo.sysUpTime;
        metricData.tagMap["start_time"] = processSelfInfo.startTime;
        metricData.tagMap["run_time"] = processSelfInfo.runTime;
        metricData.tagMap["os"] = OS_NAME;
        metricData.tagMap["target_os_arch"] = OS_NAME "-" CPU_ARCH;
        metricData.tagMap["cpu_total"] = convert(processSelfInfo.cpuTotal);
        metricData.valueMap["cpu_percent"] = processSelfInfo.cpuPercent;
        metricData.valueMap["cpu_percent_limit"] = mLimit->cpuPercent;
        metricData.valueMap["mem_resident"] = static_cast<double>(processSelfInfo.memResident);
        metricData.valueMap["mem_resident_limit"] = static_cast<double>(mLimit->rssMemory);
        metricData.valueMap["mem_size"] = static_cast<double>(processSelfInfo.memSize);
        if (processSelfInfo.memShare < mTotalMemory) {
            metricData.valueMap["mem_private"] = static_cast<double>(processSelfInfo.memPrivate);
            metricData.valueMap["mem_share"] = static_cast<double>(processSelfInfo.memShare);
        }
        metricData.valueMap["mem_percent"] = processSelfInfo.memPercent;
        metricData.valueMap["openfiles"] = static_cast<double>(processSelfInfo.openfiles);
        metricData.valueMap["openfiles_limit"] = mLimit->fdCount;
        metricData.valueMap["is_openfiles_exact"] = (processSelfInfo.openFilesExact ? 1 : 0);
        metricData.tagMap["version"] = processSelfInfo.version;
        metricData.tagMap["version_full"] = common::getVersionDetail();
        metricData.tagMap["compileTime"] = CollectInfo::GetCompileTime();
        metricData.valueMap["thread_count"] = static_cast<double>(processSelfInfo.threadCount);
        metricData.tagMap["exe_args"] = processSelfInfo.exeArgs;
        metricData.tagMap["exe_cwd"] = processSelfInfo.exeCwd;
        metricData.tagMap["exe_name"] = processSelfInfo.exeName;
        metricData.tagMap["exe_root"] = processSelfInfo.exeRoot;
        metricData.valueMap["collect_count"] = static_cast<double>(processSelfInfo.collectCount);
        metricData.tagMap["last_collect_time"] = processSelfInfo.lastCollectTime;
        metricData.tagMap["curr_collect_time"] = processSelfInfo.curCollectTime;
        metricData.valueMap["last_collect_millis"] = processSelfInfo.lastCollectCost;
        metricData.valueMap["last_commit_code"] = processSelfInfo.lastCommitCode;
        metricData.valueMap["last_commit_millis"] = processSelfInfo.lastCommitCost;
        if (!processSelfInfo.lastCommitMsg.empty()) {
            metricData.tagMap["last_commit_msg"] = processSelfInfo.lastCommitMsg;
        }
        metricData.valueMap["pullConfig_fail_count"] = static_cast<double>(processSelfInfo.pullConfigFailCount);
        metricData.valueMap["pullConfig_fail_per_minute"] = processSelfInfo.pullConfigFailPerMinute;
        metricData.valueMap["pullConfig_succ_count"] = static_cast<double>(processSelfInfo.pullConfigSuccCount);
        metricData.valueMap["putMetric_fail_count"] = static_cast<double>(processSelfInfo.putMetricFailCount);
        metricData.valueMap["putMetric_fail_per_minute"] = processSelfInfo.putMetricFailPerMinute;
        metricData.valueMap["putMetric_succ_count"] = static_cast<double>(processSelfInfo.putMetricSuccCount);
        //新增加的状态
        metricData.valueMap["coredump_count"] = processSelfInfo.coredumpCount;
        metricData.valueMap["restart_count"] = processSelfInfo.restartCount;
        metricData.valueMap["resource_exceed_count"] = processSelfInfo.resourceExceedCount;
        // 2023-05-18 (在一些虚拟机上cpu核数是可以在运行时变化的，此处需要时获取一次)
        vector<SicCpuInformation> cpuStats;
        metricData.valueMap["cpu_core_count"] = GetAllCpuStat(cpuStats) - 1;
        metricData.valueMap["hardware_concurrency"] = std::thread::hardware_concurrency();
    }
}

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////  DLL  /////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(process) {
    using namespace cloudMonitor;
    const int topN = SingletonConfig::Instance()->GetValue("cms.process.topN", 5);
    return module::NewHandler<ProcessCollect>(topN, ProcessCollect::Cpu, false);
}
