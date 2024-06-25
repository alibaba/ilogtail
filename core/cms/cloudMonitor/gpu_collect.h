#ifndef _GPU_COLLECT_H_
#define _GPU_COLLECT_H_

#include <string>
#include <vector>
#include <map>
#include "base_collect.h"

namespace cloudMonitor {
    struct GpuInfo {
        std::string id;//nvidia_smi_log.id,类型为attr
        std::string vBiosVersion;//nvidia_smi_log.gpu.nvidia_smi_log
        std::map<std::string, std::string> temperatureMap;//nvidia_smi_log.gpu.temperature
        std::map<std::string, std::string> powMap;//nvidia_smi_log.gpu.power_readings
        std::map<std::string, std::string> memMap;//nvidia_smi_log.gpu.fb_memory_usage
        std::map<std::string, std::string> utilMap;//nvidia_smi_log.gpu.utilization
    };

    struct GpuData {
        std::vector<GpuInfo> gpus;
        std::string driverVersion;//nvidia_smi_log.driver_version

        bool Parse(const std::string &xmlText);
        std::string toXml() const;
    };

#include "common/test_support"
class GpuCollect {
public:
    GpuCollect();

    int Init(const char *args);

    VIRTUAL ~GpuCollect();

    int Collect(std::string &data) const;

private:
    VIRTUAL bool GetGpuXmlData(const std::string &nvidiaPath, std::string &xmlData) const;
    static void PackageCollectData(const std::string &moduleName,
                                   const GpuData &gpuData,
                                   common::CollectData &collectData);

    void PackageCollectData(const GpuData &gpuData, common::CollectData &collectData) const {
        PackageCollectData(mModuleName, gpuData, collectData);
    }

    static bool ParserGpuData(const std::string &xmlData, GpuData &gpuData);
    static std::string GetGpuValue(const std::string &value);

private:
    const std::string mModuleName;
    bool outputGpuPerf = false;
    std::string mGpuBinPath;
    bool enableNvml = false;
};
#include "common/test_support"

}
#endif

