#include "self_monitor.h"

#include <fmt/color.h>

#if defined(__linux__)
#   include <malloc.h>  // malloc_trim
#endif

#include <chrono>

#include "common/Common.h"
#include "common/SystemUtil.h"
#include "common/StringUtils.h"
#include "common/Logger.h"

#include "common/Config.h"

#include "detect/detect_schedule_status.h"
#include "sic/system_information_collector.h"
#include "common/ChronoLiteral.h"
#include "common/Chrono.h"
#include "common/StringUtils.h" // sout
#include "common/Arithmetic.h"
#include "common/ThreadUtils.h"
#include "common/ResourceConsumptionRecord.h"

#include "agent_status.h"
#include "argus_manager.h"

#include "ChannelManager.h"

#if defined(ENABLE_CLOUD_MONITOR) || defined(ENABLE_COVERAGE)
#   include "cloudMonitor/cloud_client.h"
#   include "cloudMonitor/cloud_channel.h"
#   include "cloudMonitor/cloud_module.h"
using namespace cloudMonitor;
#endif
#if !defined(DISABLE_TIANJI)
#   include "tianji/DataAccessStart.h"
#endif

using namespace std;
using namespace std::chrono;
using namespace common;

template<typename T>
double bytesToMB(const T &v) {
    return static_cast<double>(v) / (1024.0 * 1024);
}

namespace argus {
    tagMetricLimit GetConfigLimit() {
        tagMetricLimit metricLimit;

        auto *cfg = SingletonConfig::Instance();
        metricLimit.rssMemory = 1024 * 1024 * cfg->GetValue("agent.resource.memory.limit", int64_t(200));
        metricLimit.cpuPercent = cfg->GetValue("agent.resource.cpu.limit", 0.5);
        metricLimit.fdCount = cfg->GetValue("agent.resource.fd.limit", int(defaultFdLimit));

        return metricLimit;
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelfMonitor
    SelfMonitor::SelfMonitor(bool end) : ThreadWorker(end) {
        mLimit = GetConfigLimit();

        Config *cfg = SingletonConfig::Instance();
        mInterval = std::chrono::seconds{cfg->GetValue("agent.resource.interval", 15)};
        // cfg->GetValue("agent.resource.memory.limit", mMaxRssMemory, 200);
        // mMaxRssMemory *= 1024 * 1024;
        // mMaxCpuPercent = cfg->GetValue("agent.resource.cpu.limit", 0.5);
        mMaxCount = cfg->GetValue("agent.resource.exceed.limit", 4);
        //默认10分钟更新一次
        int statusInterval = cfg->GetValue("agent.status.interval", 600);
        mAgentStatusNumber = static_cast<int>(statusInterval / mInterval.count());
        mSn = SystemUtil::getSn();
        mSelfPid = GetPid();
        // mMaxFdCount = cfg->GetValue<int>("agent.resource.fd.limit", defaultFdLimits);
    }

    SelfMonitor::~SelfMonitor() {
        endThread();
        join();
    }

    bool SelfMonitor::GetProcessStat(int pid, ProcessStat &processStat) {
        // 临时实例化，避免缓存
        auto collector = SystemInformationCollector::New();

        SicProcessTime procTime;
        bool ok = SIC_EXECUTABLE_SUCCESS == collector->SicGetProcessTime(pid, procTime, true);

        SicProcessMemoryInformation procMemory;
        ok = ok && SIC_EXECUTABLE_SUCCESS == collector->SicGetProcessMemoryInformation(pid, procMemory);

        SicProcessFd procFd{};
        ok = ok && SIC_EXECUTABLE_SUCCESS == collector->SicGetProcessFd(pid, procFd);

        processStat.pid = pid;
        if (ok) {
            dynamic_cast<SicProcessTime &>(processStat) = procTime;
            processStat.rss = procMemory.resident;
            processStat.fdNum = static_cast<int>(procFd.total);
        } else {
            LogWarn("{}", collector->SicPtr()->errorMessage);
        }
        return ok;
    }

    void SelfMonitor::Start() {
        this->runIt();
    }

#ifdef ENABLE_CLOUD_MONITOR
    int SendResourceExceedsCms(const std::vector<ResourceWaterLevel> &resources) {
        SingletonAgentStatus::Instance()->UpdateAgentStatus(AGENT_RESOURCE_STATUS);
        std::vector<ResourceConsumption> topTasks;
        AURCR::Instance()->GetTopTasks(topTasks);
        LogWarn("Top Tasks: \n{}", AURCR::Instance()->PrintTasks(topTasks));

#if !defined(WITHOUT_MINI_DUMP)
        std::list<tagThreadStack> lstThreadStack;
        std::stringstream ss;
        ss << "Enum Thread CallStacks:\n";
        WalkThreadStack([&](int index, const tagThreadStack &stack) {
            lstThreadStack.push_back(stack);
            const char *pattern = "[{:2d}] ThreadId: {}, ThreadName: {}, stack trace:\n{}";
            ss << fmt::format(pattern, index, stack.threadId, stack.threadName, stack.stackTrace) << std::endl;
        });
        LogWarn("{}", ss.str());
#endif // !WITHOUT_MINI_DUMP
        if (auto cloudClient = SingletonArgusManager::Instance()->GetCloudClient()) {
            cloudClient->SendThreadsDump(resources, topTasks
#if !defined(WITHOUT_MINI_DUMP)
                , lstThreadStack
#endif
            );
        }
        return 1;
    }
#endif

    template<typename T>
    void accumulate(const T &v, const T &threshold, int &count) {
        if (v >= threshold) {
            count++;
        } else {
            count = 0;
        }
    }

    void SelfMonitor::doRun() {
        doRun(ExitArgus);
    }

    void SelfMonitor::doRun(const std::function<void(int)> &fnExit) {
        int queueEmptyCount = 0;
        size_t loopCount = 0;
        while (!isThreadEnd()) {
            loopCount++;
            ProcessStat procStat;
            if (!GetProcessStat(mSelfPid, procStat)) {
                // LogWarn("get self-stat error");
                LogDebug("----------------------------------------------------------- {} s", mInterval.count());
                sleepFor(mInterval);
                continue;
            }
            auto now = steady_clock::now();
            if (mLastTime.time_since_epoch().count() > 0) {
                auto deltaCpu = Diff(procStat.total, mLastCpu);
                auto deltaMillis = duration_cast<std::chrono::milliseconds>(now - mLastTime).count();  // 单位秒
                double cpuPercent = static_cast<double>(deltaCpu.count()) / static_cast<double>(deltaMillis);
                LogDebug("mLastCpu: {} ms, nowCpu: {} ms", mLastCpu.count(), procStat.total.count());

                accumulate(cpuPercent, mLimit.cpuPercent, mCpuExceedCount);
                accumulate(procStat.rss, mLimit.rssMemory, mMemoryExceedCount);
                accumulate(procStat.fdNum, mLimit.fdCount, mFdExceedCount);
#if defined(ENABLE_CLOUD_MONITOR)
                auto chan = SingletonArgusManager::Instance()->GetChannel<CloudChannel>(CloudChannel::Name);
                if (chan && GetActiveModuleCount() > 0) {
                    accumulate(chan->GetQueueEmptyCount(), uint32_t(1), queueEmptyCount);
                } else {
                    queueEmptyCount = 0;
                }
#endif
                auto IsExceed = [=](int v) {
                    return mCpuExceedCount >= v ||
                           mMemoryExceedCount >= v ||
                           mFdExceedCount >= v ||
                           queueEmptyCount >= 2*v; // 发送队列监控次数拉长一点，避免太过敏感
                };
                // AURCR::Instance()->EnableRecording(IsExceed(1));
                bool willExit = IsExceed(mMaxCount);

                // FIXME: hancj.2023-07-01 奇怪这里css如果是匿名函数的话，输出的数值不正确，宏定义就没事
                // fmt的颜色输出不支持windows，只在linux上有效
#ifdef ENABLE_COVERAGE
#   define css(v) fmt::styled(v, fmt::fg((v) < mMaxCount ? fmt::color::light_green : fmt::color::red))
#else
#   define css(v) v
#endif
				// 这里原本是直接LogInfo的，但是在msvc下编译失败(可能宏太复杂)，只能先行格式好，再打日志
				std::string line = fmt::format("pid: {}, [{}] cpuUsage={:.2f}%[>={:.2f}% {}/{}]"
					", memory={:.3f}MB[>={:.2f}MB {}/{}]"
					", openFiles={}[>={} {}/{}]"
#if defined(ENABLE_CLOUD_MONITOR) || defined(ENABLE_COVERAGE)
					", outputChannelEmpty={}[{}/{}]"
#endif
					"{}", mSelfPid, loopCount,
					cpuPercent * 100.0, mLimit.cpuPercent * 100, css(mCpuExceedCount), mMaxCount,
					bytesToMB(procStat.rss), bytesToMB(mLimit.rssMemory), css(mMemoryExceedCount), mMaxCount,
					procStat.fdNum, mLimit.fdCount, css(mFdExceedCount), mMaxCount,
#if defined(ENABLE_CLOUD_MONITOR) || defined(ENABLE_COVERAGE)
					(queueEmptyCount > 0), css(queueEmptyCount), 2*mMaxCount,
#endif
					(willExit ? " => will exit" : "")
				);
				LogInfo(line);
#undef css

                if (willExit) {
#ifdef ENABLE_CLOUD_MONITOR
                    int count = SendResourceExceedsCms(std::vector<ResourceWaterLevel>{
                            {"cpu",       cpuPercent,              mLimit.cpuPercent,           mCpuExceedCount},
                            {"memory",    bytesToMB(procStat.rss), bytesToMB(mLimit.rssMemory), mMemoryExceedCount},
                            {"openFiles", (double) procStat.fdNum, (double) mLimit.fdCount,     mFdExceedCount},
                    });
#else
                    int count = SendResourceExceedsNonCms();
#endif
                    if (count > 0) {
                        sleepFor(3_s);  // 等待结果发送完成
                    }
                    // 主动退出
                    fnExit(EXIT_FAILURE);
                    return;
                }
            }
#if defined(__linux__)
            if(mMallocFreeCount>=60)
            {
                LogInfo("will malloc_trim");
                mMallocFreeCount=0;
                malloc_trim(0);//清除内存碎片
            }
            mMallocFreeCount++;
#endif
            mLastTime = now;
            mLastCpu = procStat.total;
            mAgentStatusCount++;
            if (mAgentStatusCount == mAgentStatusNumber) {
                SendResourceExceedsNonCms();
            }
            sleepFor(mInterval);
        }
    }

    int SelfMonitor::SendResourceExceedsNonCms()
    {
        int count = 0;
#if !defined(ENABLE_CLOUD_MONITOR)
        vector<CommonMetric> metrics;
        GetAllAgentStatusMetrics(metrics);
        LogDebug("send {} agent-status-metric(s) to monitor", metrics.size());

        vector<pair<string, string>> outputVector;
        outputVector.emplace_back("alimonitor", "");
        if (auto pManager = SingletonArgusManager::Instance()->getChannelManager()) {
            count = pManager->SendMetricsToNextModule(metrics, outputVector);
        }
#endif
        mAgentStatusCount = 0;
        return count;
    }

    void SelfMonitor::GetAllAgentStatusMetrics(std::vector<CommonMetric> &metrics) const {
        SingletonArgusManager::Instance()->GetStatus(metrics);
#ifndef DISABLE_TIANJI
        dataAccess::DataAccessStart::getDomainSocketListenStatus(metrics);
#endif
        CommonMetric detectMetric;
        SingletonDetectScheduleStatus::Instance()->GetStatus(detectMetric);
        metrics.push_back(detectMetric);
        CommonMetric agentMetric;
        GetAgentStatus(metrics, agentMetric);
        metrics.push_back(agentMetric);
        //增加额外的标签，包括上传到SLS和机器的基本数据
        for (auto it = metrics.begin(); it != metrics.end();) {
            if (it->name.empty()) {
                it = metrics.erase(it);
            } else {
                it->tagMap["SYS_DT"] = "4";
                it->tagMap["sn"] = mSn;
                it++;
            }
        }
    }

    void SelfMonitor::GetAgentStatus(const vector <CommonMetric> &metrics, CommonMetric &agentMetric) {
        double agent_status = 0;
        for (const auto &metric: metrics) {
            if (metric.name == "monitor_status") {
                auto itTmp = metric.tagMap.find("monitor");
                if (itTmp != metric.tagMap.end()) {
                    agentMetric.tagMap["monitor"] = itTmp->second;
                }
                agentMetric.tagMap["monitor_status"] = StringUtils::DoubleToString(metric.value);
                agent_status = metric.value;
            } else if (metric.name == "ingestion_status") {
                auto itTmp = metric.tagMap.find("ingestion_endpoint");
                if (itTmp != metric.tagMap.end()) {
                    agentMetric.tagMap["ingestion_endpoint"] = itTmp->second;
                }
                itTmp = metric.tagMap.find("ingestion2_endpoint");
                if (itTmp != metric.tagMap.end()) {
                    agentMetric.tagMap["ingestion2_endpoint"] = itTmp->second;
                }
                agentMetric.tagMap["ingestion_status"] = StringUtils::DoubleToString(metric.value);
            }
        }
        auto *agentStatus = SingletonAgentStatus::Instance();
        agentMetric.tagMap["agent_resource"] = convert(agentStatus->GetAgentStatus(AGENT_RESOURCE_STATUS));
        agentMetric.tagMap["agent_crash"] = convert(agentStatus->GetAgentStatus(AGENT_COREDUMP_STATUS));
        agentMetric.tagMap["agent_restart"] = convert(agentStatus->GetAgentStatus(AGENT_START_STATUS));
        agentMetric.tagMap["agent_version"] = string(getVersion(false));
        agentMetric.value = agent_status;
        agentMetric.name = "agent_status";
        agentMetric.timestamp = NowSeconds();

    }

}