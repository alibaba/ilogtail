#include "cpu_collect.h"

#include <functional>
#include <chrono>
#include <atomic>
#include <thread> // std::thread::hardware_concurrency()

#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"

#include "cloud_monitor_const.h"

using namespace std;
using namespace std::chrono;
using namespace common;

namespace cloudMonitor {
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CpuMetric
#define CPU_METRIC_KEY_ENTRY(member) FIELD_ENTRY(CpuMetric, member)
    static const FieldName<CpuMetric, double> cpuMetricKeyIndex[] = {
            CPU_METRIC_KEY_ENTRY(sys),
            CPU_METRIC_KEY_ENTRY(user),
            CPU_METRIC_KEY_ENTRY(wait),
            CPU_METRIC_KEY_ENTRY(idle),
            CPU_METRIC_KEY_ENTRY(other),
            CPU_METRIC_KEY_ENTRY(total),
    };
#undef CPU_METRIC_KEY_ENTRY
    static const size_t cpuMetricKeyCount = sizeof(cpuMetricKeyIndex) / sizeof(cpuMetricKeyIndex[0]);
    static_assert(cpuMetricKeyCount == sizeof(CpuMetric) / sizeof(double), "cpuMetricKeyIndex size unexpected");

    void enumerate(const std::function<void(const FieldName<CpuMetric, double> &)>& callback) {
        for (auto it = cpuMetricKeyIndex; it < cpuMetricKeyIndex + cpuMetricKeyCount; ++it) {
            callback(*it);
        }
    }

    CpuMetric::CpuMetric(const SicCpuPercent &cpuPercent) {
        sys = cpuPercent.sys;
        user = cpuPercent.user;
        idle = cpuPercent.idle;
        wait = cpuPercent.wait;
        other = cpuPercent.nice + cpuPercent.softIrq + cpuPercent.irq + cpuPercent.stolen;
        total = 1 - idle;
    }

    // /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CpuCollect
    std::atomic<uint32_t> cpuCollectInstanceCount{0};

    CpuCollect::CpuCollect() {
        auto count = ++cpuCollectInstanceCount;
        mTotalCount = 0;
        mCount = 0;
        mCoreNumber = 0;
        LogInfo("load cpu: {}, total instance(s): {}", (void *)this, count);
    }

    int CpuCollect::Init(int totalCount) {
        mTotalCount = totalCount;
        return 0;
    }

    CpuCollect::~CpuCollect() {
        mMetricCalculates.clear();
        auto count = --cpuCollectInstanceCount;
        LogInfo("unload cpu: {}, total instance(s): {}", (void *)this, count);
    }

    int CpuCollect::Collect(string &collectData) {
        steady_clock::time_point now = steady_clock::now();
        std::swap(mLastCpuStats, mCurrentCpuStats);
        mCurrentCpuStats.clear();
        if (GetAllCpuStat(mCurrentCpuStats) < 0) {
            return -1;
        }
        if (mLastCpuStats.empty()) {
            mCoreNumber = mCurrentCpuStats.size();
            LogInfo("CpuCollect first time, coreCount={}", mCoreNumber - 1);
            mMetricCalculates.resize(mCoreNumber);

            return 0;
        }
        if (mLastCpuStats.size() != mCurrentCpuStats.size()) {
            LogWarn("cpuCore changed,{}:{}", mLastCpuStats.size(), mCurrentCpuStats.size());
            return -1;
        }
        for (size_t i = 0; i < mCoreNumber; i++) {
            SicCpuPercent cpuPercent = mLastCpuStats[i] / mCurrentCpuStats[i];
            mMetricCalculates[i].AddValue(CpuMetric{cpuPercent});
        }
        mCount++;

        LogDebug("collect cpu spend {}", duration_cast<milliseconds>(steady_clock::now() - now));
        if (mCount < mTotalCount) {
            return 0;
        }

        GetCollectData(collectData);
        mCount = 0;
        for (size_t i = 0; i < mCoreNumber; i++) {
            mMetricCalculates[i].Reset();
        }
        return static_cast<int>(collectData.size());
    }

    void CpuCollect::GetCollectData(string &collectStr) {
        CollectData collectData;
        collectData.moduleName = mModuleName;
        string metricName{"system.cpu"};
        size_t cpuCount = 1; // system.cpu
#ifdef ENABLE_CPU_CORE
        cpuCount = mCoreNumber;
#endif
        for (size_t i = 0; i < cpuCount; i++) {
            CpuMetric max, min, avg;
            mMetricCalculates[i].Stat(max, min, avg);

            MetricData metricData;
            GetMetricData(max, min, avg, metricName, static_cast<int>(i > 0 ? (i - 1) : 0), i, metricData);
            collectData.dataVector.push_back(metricData);

            metricName = "system.cpuCore";
        }

        //兼容云监控老系统指标
        MetricData metricData;
        metricData.tagMap["metricName"] = "vm.CPUUtilization";
        metricData.tagMap["ns"] = "acs/ecs";
        metricData.valueMap["metricValue"] = mMetricCalculates[0].GetLastValue()->total * 100.0;
        collectData.dataVector.push_back(metricData);
        ModuleData::convertCollectDataToString(collectData, collectStr);
    }

    void CpuCollect::GetMetricData(const CpuMetric &max,
                                   const CpuMetric &min,
                                   const CpuMetric &avg,
                                   const string &metricName,
                                   int metricValue,
                                   size_t index,
                                   MetricData &metricData) const {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["metricValue"] = metricValue;
        metricData.valueMap["core_count"] = static_cast<int>(mCoreNumber - 1);
        if (index > 0) {
            metricData.valueMap["core"] = static_cast<double>(index - 1);
        }

        for (auto &it: cpuMetricKeyIndex) {
            metricData.valueMap[string("max_") + it.name] = it.value(max) * 100.0;
            metricData.valueMap[string("min_") + it.name] = it.value(min) * 100.0;
            metricData.valueMap[string("avg_") + it.name] = it.value(avg) * 100.0;
        }
    }
} // end of namespace cloudMonitor

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// export dll
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(cpu){
    LogDebug("cpu_init({})", (args == nullptr ? "" : args));

    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    return module::NewHandler<cloudMonitor::CpuCollect>(totalCount);
}
