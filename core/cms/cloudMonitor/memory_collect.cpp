#include "memory_collect.h"
#include <chrono>
#include "common/Logger.h"
#include "common/ModuleData.h"

using namespace std;
using namespace std::chrono;
using namespace common;

namespace cloudMonitor {
    MemoryCollect::MemoryCollect() : mModuleName("memory") {
        LogInfo("load memory");
    }

    MemoryCollect::~MemoryCollect() {
        LogInfo("unload memory");
    }

    int MemoryCollect::Collect(string &data) {
        auto start = steady_clock::now();

        SicMemoryInformation memoryStat;
        SicSwapInformation swapStat;
        int ret = -1;
        if (GetMemoryStat(memoryStat) == 0 && GetSwapStat(swapStat) == 0) {
            CollectData collectData;
            collectData.moduleName = mModuleName;

            MetricData metricDataMemory;
            GetMemoryMetricData(memoryStat, "system.mem", metricDataMemory);
            collectData.dataVector.push_back(metricDataMemory);

            // 该指标已不被metrichub支持，metrichub会自动discard掉
#ifdef ENABLE_MEM_SWAP
            MetricData metricDataSwap;
            GetSwapMetricData(swapStat, "system.swap", metricDataSwap);
            collectData.dataVector.push_back(metricDataSwap);
#endif
            MetricData metricDataOld;
            GetOldMetricData(memoryStat.usedPercent, "vm.MemoryUtilization", metricDataOld);
            collectData.dataVector.push_back(metricDataOld);

            ModuleData::convertCollectDataToString(collectData, data);
            ret = static_cast<int>(data.size());

            LogDebug("collect memory spend {}", duration_cast<milliseconds>(steady_clock::now() - start));
        }
        return ret;
    }

#define Double(expr) static_cast<double>(expr)

    void MemoryCollect::GetMemoryMetricData(const SicMemoryInformation &memoryStat, const string &metricName,
                                            MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = 0;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["ram"] = Double(memoryStat.ram);
        metricData.valueMap["total"] = Double(memoryStat.total);
        metricData.valueMap["used"] = Double(memoryStat.used);
        metricData.valueMap["free"] = Double(memoryStat.free);
        metricData.valueMap["actual_used"] = Double(memoryStat.actualUsed);
        metricData.valueMap["actual_free"] = Double(memoryStat.actualFree);
        metricData.valueMap["used_percent"] = memoryStat.usedPercent;
        metricData.valueMap["free_percent"] = memoryStat.freePercent;
    }

    void MemoryCollect::GetSwapMetricData(const SicSwapInformation &swapStat, const string &metricName,
                                          MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.tagMap["ns"] = "acs/host";
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["total"] = Double(swapStat.total);
        metricData.valueMap["used"] = Double(swapStat.used);
        metricData.valueMap["free"] = Double(swapStat.free);
        metricData.valueMap["page_in"] = Double(swapStat.pageIn);
        metricData.valueMap["page_out"] = Double(swapStat.pageOut);
        metricData.valueMap["util"] =
                swapStat.total == 0 ? 0.0 : (Double(swapStat.used) * 100 / Double(swapStat.total));
    }

#undef Double

    void MemoryCollect::GetOldMetricData(double usedPecent, const string &metricName, MetricData &metricData) {
        metricData.tagMap["metricName"] = metricName;
        metricData.valueMap["metricValue"] = usedPecent;
        metricData.tagMap["ns"] = "acs/ecs";
    }
}


// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DLL
#include "cloud_module_macros.h"

IMPLEMENT_CMS_MODULE(memory) {
    return module::NewHandler<cloudMonitor::MemoryCollect>();
}
