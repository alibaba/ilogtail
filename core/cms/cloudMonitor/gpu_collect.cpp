#include "gpu_collect.h"

#include <atomic>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include "common/Logger.h"
#include "common/Common.h"
#include "common/StringUtils.h"
#include "common/ModuleData.h"
#include "common/TimeProfile.h"
#include "common/Config.h"

#ifdef WIN32
#include "sic/win32_system_information_collector.h" // GetSpecialDirectory
#endif

#include "gpu_collect_nvml.h"

using namespace std;
using namespace std::chrono;
using namespace common;

namespace cloudMonitor {
    constexpr const int defaultAttachedGpuCount = 8;
    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GpuData
    bool GpuData::Parse(const std::string &xmlText) {
        using namespace boost::property_tree;

        bool success = true;
        try {
            ptree tree;
            std::stringstream is(xmlText);
            xml_parser::read_xml(is, tree);

            auto root = tree.get_child("nvidia_smi_log"); // nvidia_smi_log是xml的根结点

            this->driverVersion = root.get<std::string>("driver_version");
            // optional时，即使key不存在也不抛异常

            this->gpus.clear();
            auto gpuCount = root.get_optional<int>("attached_gpus");
            this->gpus.reserve(gpuCount.has_value() && gpuCount.get() > 0 ? gpuCount.get() : defaultAttachedGpuCount);

            for (auto it = root.begin(); it != root.end(); ++it) {
                if (it->first != "gpu") {
                    continue;
                }
                auto &gpuChild = it->second;

                GpuInfo curGpu;
                curGpu.id = gpuChild.get<std::string>("<xmlattr>.id");
                curGpu.vBiosVersion = gpuChild.get<std::string>("vbios_version");

                auto readMap = [&](const char *nodeName, std::map<std::string, std::string> &data) {
                    if (auto child = gpuChild.get_child_optional(nodeName)) {
                        for (auto const &sub: *child) {
                            data[sub.first] = sub.second.data();
                        }
                    }
                };
                readMap("temperature", curGpu.temperatureMap);
                readMap("power_readings", curGpu.powMap);
                if (curGpu.powMap.empty()) {
                    // gpu 535.129.03中， 使用了新关键字(gpu_power_readings)
                    readMap("gpu_power_readings", curGpu.powMap);
                }
                readMap("fb_memory_usage", curGpu.memMap);
                readMap("utilization", curGpu.utilMap);
                this->gpus.push_back(curGpu);
            }
        } catch (const std::exception &ex) {
            LogWarn("Parse Gpu Data Error: {}", ex.what());
            success = false;
        }
        return success;
    }

    std::string GpuData::toXml() const {
        auto output = [](std::stringstream &ss, const std::string &name,
                         const std::map<std::string, std::string> &data) {
            ss << "        <" << name << ">" << std::endl;
            for (const auto &it: data) {
                ss << "            <" << it.first << ">" << it.second << "</" << it.first << ">" << std::endl;
            }
            ss << "        </" << name << ">" << std::endl;
        };

        const char *szXmlHeader = R"XX(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
)XX";
        std::stringstream ss;
        ss << szXmlHeader + 1;
        ss << fmt::format("    <timestamp>{:L%a %b %d %H:%M:%S %Y}</timestamp>\n", std::chrono::system_clock::now());
        ss << "    <driver_version>" << this->driverVersion << "</driver_version>" << std::endl;
        ss << "    <attached_gpus>" << this->gpus.size() << "</attached_gpus>" << std::endl;
        for (const auto &gpu: gpus) {
            ss << "    <gpu id=\"" << gpu.id << "\">" << std::endl;
            ss << "        <vbios_version>" << gpu.vBiosVersion << "</vbios_version>" << std::endl;

            output(ss, "temperature", gpu.temperatureMap);
            bool hasPowerManagement = (gpu.powMap.find("power_management") != gpu.powMap.end());
            output(ss, (hasPowerManagement ? "power_readings" : "gpu_power_readings"), gpu.powMap);
            output(ss, "fb_memory_usage", gpu.memMap);
            output(ss, "utilization", gpu.utilMap);

            ss << "    </gpu>" << std::endl;
        }

        ss << "</nvidia_smi_log>";

        return ss.str();
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    GpuCollect::GpuCollect() : mModuleName{"gpu"} {
        LogInfo("load gpu");
        outputGpuPerf = SingletonConfig::Instance()->GetValue<bool>("gpu.log.output.perf", false);
        enableNvml = SingletonConfig::Instance()->GetValue<bool>("nvidia.nvml.enabled", false);
    }

    std::string makeCmdLine(const std::initializer_list<std::string> &argv) {
        std::stringstream ss;
        if (argv.size() > 0) {
            auto it = argv.begin();
            const std::string &bin = *it;
            it++;

            if (!HasPrefix(bin, "\"") && !HasSuffix(bin, "\"")) {
                ss << "\"" << bin << "\"";
            }

            for (; it != argv.end(); ++it) {
                ss << " " << *it;
            }
        }
        return ss.str();
    }

    static std::string FindGpu() {
#ifdef WIN32
        fs::path nvidiaPath = GetSpecialDirectory(EnumDirectoryType::System32) / "nvidia-smi.exe";
        if (fs::exists(nvidiaPath)) {
            return nvidiaPath.string();
        }

        auto programFiles = GetSpecialDirectory(EnumDirectoryType::ProgramFiles);
        if (!programFiles.empty()) {
            nvidiaPath = programFiles / R"(NVIDIA Corporation\NVSMI\nvidia-smi.exe)";
        }
#else
        const fs::path nvidiaPath{"/usr/bin/nvidia-smi"};
#endif
        return fs::exists(nvidiaPath) ? nvidiaPath.string() : std::string{};
    }

    int GpuCollect::Init(const char *args) {
        if (args != nullptr) {
            mGpuBinPath = TrimSpace(args);
        }
        if (mGpuBinPath.empty()) {
            mGpuBinPath = FindGpu();
        }
        if (mGpuBinPath.empty()) {
            LogInfo("gpuBinPath is empty");
            return 0;
        }
        if (!fs::exists(mGpuBinPath)) {
            LogInfo("gpuBinPath({}) is not exist", mGpuBinPath);
        } else {
            string cmdline = makeCmdLine({mGpuBinPath, "-pm", "1"});
            std::string output;
            int ret = getCmdOutput(cmdline, output);
            Log((ret != 0 ? LEVEL_WARN : LEVEL_INFO), "exec [{}], output: {}", cmdline, output);
        }
        return 0;
    }

    GpuCollect::~GpuCollect() {
        LogInfo("unload gpu");
    }

    int GpuCollect::Collect(string &data) const {
        TimeProfile tp;
        GpuData gpuData;

        if (enableNvml) {
            auto nvml = NewNvml();
            if (!nvml || !nvml->Collect(gpuData, 1)) {
                return -1;
            }
        } else {
            fs::path nvidiaBinPath = mGpuBinPath.empty() ? FindGpu() : mGpuBinPath;
            if (nvidiaBinPath.empty() || !fs::exists(nvidiaBinPath)) {
                return -1;
            }

            string xmlData;
            if (!GetGpuXmlData(nvidiaBinPath.string(), xmlData)) {
                return -2;
            }
            Log((outputGpuPerf ? LEVEL_INFO : LEVEL_DEBUG),
                "GetGpuXmlData spend {}, xmlData.size() = {}, nvidiaPath: {}",
                tp.cost(), xmlData.size(), nvidiaBinPath.string());

            if (!ParserGpuData(xmlData, gpuData)) {
                return -3;
            }
        }

        CollectData collectData;
        PackageCollectData(gpuData, collectData);
        ModuleData::convertCollectDataToString(collectData, data);

        const auto level = (outputGpuPerf ? LEVEL_INFO : LEVEL_DEBUG);
        Log(level, "collect gpu total spend {}, gpu count: ", tp.cost(), gpuData.gpus.size());
        return static_cast<int>(data.size());
    }

    static std::atomic<uint32_t> gGetGpuXmlDataCounter(0);

    bool GpuCollect::GetGpuXmlData(const std::string &nvidiaPath, string &xmlData) const {
        string cmdline = makeCmdLine({nvidiaPath, "--query", "-x"});
        uint32_t count = gGetGpuXmlDataCounter++;
        if (0 == (count % 240)) {
            // 1分钟4次，1小时为240次
            LogInfo("[{}] {}", count, cmdline);
        }
        return 0 == getCmdOutput(cmdline, xmlData);
    }

    bool GpuCollect::ParserGpuData(const string &xmlData, GpuData &gpuData) {
        return gpuData.Parse(xmlData);
    }

    void GpuCollect::PackageCollectData(const std::string &moduleName,
                                        const GpuData &gpuData,
                                        CollectData &collectData) {
        collectData.moduleName = moduleName;
        for (const auto &gpu: gpuData.gpus) {
            MetricData metricData;
            metricData.valueMap["metricValue"] = 0;
            metricData.tagMap["metricName"] = "gpu";
            metricData.tagMap["ns"] = "acs/gpu";
            metricData.tagMap["id"] = gpu.id;
            metricData.tagMap["driver_version"] = gpuData.driverVersion;
            metricData.tagMap["vbios_version"] = gpu.vBiosVersion;
            for (const auto &it: gpu.temperatureMap) {
                metricData.tagMap[string("temperature_") + it.first] = GetGpuValue(it.second);
            }
            for (const auto &it: gpu.powMap) {
                metricData.tagMap[string("power_readings_") + it.first] = GetGpuValue(it.second);
            }
            for (const auto &it: gpu.memMap) {
                metricData.tagMap[string("fb_memory_usage_") + it.first] = GetGpuValue(it.second);
            }
            for (const auto &it: gpu.utilMap) {
                metricData.tagMap[string("utilization_") + it.first] = GetGpuValue(it.second);
            }
            collectData.dataVector.push_back(metricData);
        }
    }

    string GpuCollect::GetGpuValue(const string &value) {
        if (value == "N/A") {
            return "0";
        }
        string suffixes[] = {" C", " W", " %", " MiB"};
        for (const auto &suffix: suffixes) {
            if (StringUtils::EndWith(value, suffix)) {
                return value.substr(0, value.size() - suffix.size());
            }
        }
        return value;

    }
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DLL
#include "cloud_module_macros.h"

using cloudMonitor::GpuCollect;

IMPLEMENT_CMS_MODULE(gpu) {
    string gpuBinPath = SingletonConfig::Instance()->GetValue("cms.agent.nvidia.path", "");
    return module::NewHandler<GpuCollect>(gpuBinPath.c_str());
}
