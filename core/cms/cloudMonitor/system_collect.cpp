#include "system_collect.h"

#include <thread>

#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"

#include "cloud_monitor_const.h"

using namespace std;
using namespace common;

namespace cloudMonitor {
    const FieldName<SicLoadInformation> systemLoadMeta[] = {
            FIELD_ENTRY(SicLoadInformation, load1),
            FIELD_ENTRY(SicLoadInformation, load5),
            FIELD_ENTRY(SicLoadInformation, load15),
    };
    const size_t systemLoadMetaSize = sizeof(systemLoadMeta) / sizeof(systemLoadMeta[0]);
    const FieldName<SicLoadInformation> *const systemLoadMetaEnd = systemLoadMeta + systemLoadMetaSize;
    static_assert(systemLoadMetaSize == sizeof(SicLoadInformation)/sizeof(double), "systemLoadMeta unexpected");

    void enumerate(const std::function<void(const FieldName<SicLoadInformation> &)> &callback) {
        for (auto it = systemLoadMeta; it < systemLoadMetaEnd; ++it) {
            callback(*it);
        }
    }

    SystemCollect::SystemCollect() : mModuleName{"system"} {
        LogInfo("load system");
    }

    int SystemCollect::Init(int totalCount) {
        mTotalCount = totalCount;
        vector<SicCpuInformation> cpuStats;
        mCount = 0;
        return 0;
    }

    SystemCollect::~SystemCollect() {
        LogInfo("unload system");
    }

    int SystemCollect::Collect(string &data) {
#ifdef WIN32
        //windows环境无需采集
        return 0;
#else
        int64_t upTimeSeconds = GetUptime(false);
        TimeProfile timeProfile;
        SicLoadInformation loadStat;
        if (GetLoadStat(loadStat) != 0) {
            return -1;
        }
        mMetricCalculate.AddValue(loadStat);

        LogDebug("collect system spend {} ms", timeProfile.millis());

        mCount++;
        if (mCount < mTotalCount) {
            return 0;
        }
        SicLoadInformation minLoadStat, maxLoadStat, avgLoadStat, lastLoadStat;
        mMetricCalculate.Stat(maxLoadStat, minLoadStat, avgLoadStat, &lastLoadStat);

        mCount = 0;
        mMetricCalculate.Reset();

        //数据格式转化
        CollectData collectData;
        collectData.moduleName = mModuleName;
        MetricData metricData, metricData1, metricData5, metricData15;
        GetMetricData(minLoadStat, maxLoadStat, avgLoadStat, upTimeSeconds, metricData, "system.load");
        collectData.dataVector.push_back(metricData);
        MetricData metricDataPerCore;
        GetMetricDataPerCore(minLoadStat, maxLoadStat, avgLoadStat, "system.load.perCore", metricDataPerCore);
        collectData.dataVector.push_back(metricDataPerCore);
        GetMetricData("vm.LoadAverage", lastLoadStat.load1, "period", "1min", metricData1);
        collectData.dataVector.push_back(metricData1);
        GetMetricData("vm.LoadAverage", lastLoadStat.load5, "period", "5min", metricData5);
        collectData.dataVector.push_back(metricData5);
        GetMetricData("vm.LoadAverage", lastLoadStat.load15, "period", "15min", metricData15);
        collectData.dataVector.push_back(metricData15);
        ModuleData::convertCollectDataToString(collectData, data);
        return static_cast<int>(data.size());
#endif
    }

    int SystemCollect::GetMetricData(const SicLoadInformation &min,
                                     const SicLoadInformation &max,
                                     const SicLoadInformation &avg,
                                     int64_t uptimeSeconds,
                                     MetricData &metricData,
                                     const string &metricName) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["uptime"] = static_cast<double>(uptimeSeconds);

        for (auto it = systemLoadMeta; it < systemLoadMetaEnd; ++it) {
            metricData.valueMap[string("max_") + it->name] = it->value(max);
            metricData.valueMap[string("min_") + it->name] = it->value(min);
            metricData.valueMap[string("avg_") + it->name] = it->value(avg);
        }
        return 0;
    }

    int SystemCollect::GetMetricDataPerCore(const SicLoadInformation &min,
                                            const SicLoadInformation &max,
                                            const SicLoadInformation &avg,
                                            const string &metricName,
                                            MetricData &metricData) {
        auto cpuCoreCount = static_cast<double>(std::thread::hardware_concurrency());
        cpuCoreCount = cpuCoreCount < 1 ? 1 : cpuCoreCount;

        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["cpuCoreCount"] = cpuCoreCount;
        for (auto it = systemLoadMeta; it < systemLoadMetaEnd; ++it) {
            metricData.valueMap[string("max_") + it->name] = it->value(max) / cpuCoreCount;
            metricData.valueMap[string("min_") + it->name] = it->value(min) / cpuCoreCount;
            metricData.valueMap[string("avg_") + it->name] = it->value(avg) / cpuCoreCount;
        }
        return 0;
    }

    int SystemCollect::GetMetricData(const string &metricName,
                                     double metricValue,
                                     const string &tagName,
                                     const string &tagValue,
                                     MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap[tagName] = tagValue;
        metricData.valueMap["metricValue"] = metricValue;
        metricData.tagMap["ns"] = "acs/ecs";
        return 0;
    }
} // namespace cloudMonitor

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(system) {
    int totalCount = static_cast<int>(ToSeconds(cloudMonitor::kDefaultInterval));
    if (args != nullptr) {
        int count = convert<int>(args);
        if (count > 0) {
            totalCount = count;
        }
    }
    return module::NewHandler<cloudMonitor::SystemCollect>(totalCount);
}
