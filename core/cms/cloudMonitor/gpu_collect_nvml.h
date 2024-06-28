//
// Created by 韩呈杰 on 2024/1/19.
//
#ifndef ARGUS_CMS_GPU_COLLECT_NVML_H
#define ARGUS_CMS_GPU_COLLECT_NVML_H

#include "gpu_collect.h"
#include <type_traits> // std::is_same, std::enable_if
#include <memory>
#include <string>
#include <utility>
#include <initializer_list>

#include "common/Logger.h"
#include "common/ThrowWithTrace.h"
#include "common/Config.h"

#include <boost/system.hpp>
#include <boost/dll/shared_library.hpp>

#define NVML_NO_UNVERSIONED_FUNC_DEFS // 必须在include nvml.h之前定义

#include "nvidia/gdk/version-12/nvml.h"

struct GpuException : public std::runtime_error {
    explicit GpuException(const std::string &msg) : std::runtime_error(msg) {
    }

    GpuException(const std::string &funcName, nvmlReturn_t r, const std::string &s) noexcept
            : std::runtime_error(fmt::format("{}() error: ({}) {}", funcName, (int) r, s)) {
    }
};

template<typename TSharedLibrary, typename Func>
class dll_function;  // undefined

template<typename TSharedLibrary>
class dll_function<TSharedLibrary, decltype(nvmlErrorString)> {
public:
    typedef decltype(nvmlErrorString) T;
    typedef std::function<T> Fn;

    const char *Name() const {
        return szFuncName;
    }

    dll_function() = default;

    explicit dll_function(const TSharedLibrary &d) {
        *this = d;
    }

    dll_function &operator=(const TSharedLibrary &d) {
        dl = d;
        fn = dl.template get<T>(Name());
        return *this;
    }

    dll_function(const dll_function &) = default;
    dll_function &operator=(const dll_function &) = default;

    explicit operator bool() const {
        return (bool) fn;
    }

    std::string operator()(nvmlReturn_t result) const {
        auto s = fn(result);
        return s ? s : fmt::format("unknown error from nvml: {}", (int) result);
    }

private:
    const char *szFuncName = "nvmlErrorString";
    TSharedLibrary dl;
    Fn fn;
};

template<typename TSharedLibrary, typename Rp, typename ...ArgTypes>
class dll_function<TSharedLibrary, Rp(ArgTypes...)> {
public:
    // typedef typename Rp(ArgTypes...) T;
    const char *Name() const {
        return szFuncName;
    }

    explicit dll_function(const char *szName) : szFuncName(szName) {
    }

    dll_function(const TSharedLibrary &d, const char *szName) : dll_function(szName) {
        *this = d;
    }

    dll_function &operator=(const TSharedLibrary &d) {
        dl = d;
        fn = dl.template get<Rp(ArgTypes...)>(Name());
        LogDebug("dl.get({}) success", Name());
        return *this;
    }

    dll_function(const dll_function &) = default;
    dll_function &operator=(const dll_function &) = default;

    explicit operator bool() const {
        return (bool) fn;
    }

    void operator()(ArgTypes ...args) const {
        nvmlReturn_t ret;
        try {
            ret = fn(std::forward<ArgTypes>(args)...);
        } catch (const std::exception &ex) {
            throw GpuException(Name(), (nvmlReturn_t) (-1), ex.what());
        }
        if (ret != NVML_SUCCESS) {
            dll_function<TSharedLibrary, decltype(nvmlErrorString)> fnErrorString(dl);
            throw GpuException(Name(), ret, fnErrorString(ret));
        }
    }

    std::function<Rp(ArgTypes...)> native() const {
        return fn;
    }

private:
    const char *szFuncName;
    TSharedLibrary dl;
    std::function<Rp(ArgTypes...)> fn;
};


#define DECLARE(F, N) dll_function<TSharedLibrary, decltype(F)> N{#F}
#define IGNORE_EXCEPT(expr) try { expr; } catch(const std::exception &) {}

template<typename T1, typename T2>
void checkAnyLoaded(const T1 &t1, const T2 &t2) {
    if (!t1 && !t2) {
        Throw<GpuException>("neither {} nor {} are found", t1.Name(), t2.Name());
    }
}

template<typename TSharedLibrary, typename TFunc>
class TAny {
public:
    typedef dll_function<TSharedLibrary, TFunc> Function;

    TAny(const std::initializer_list<Function> &list) : functions(list) {
    }

    template<typename ...ArgTypes>
    void call(ArgTypes ...args) const {
        std::string errMsg;
        for (const auto &fn: functions) {
            if (fn) {
                try {
                    fn(std::forward<ArgTypes>(args)...);
                    return;
                } catch (const std::exception &ex) {
                    errMsg = ex.what();
                }
            }
        }
        if (errMsg.empty()) {
            std::stringstream ss;
            ss << "no function valid in: ";
            const char *sep = "";
            for (const auto &fn: functions) {
                ss << sep << fn.Name();
                sep = ", ";
            }
            errMsg = ss.str();
        }
        throw GpuException(errMsg);
    }

private:
    std::initializer_list<Function> functions;
};

template<typename TSharedLibrary, typename TFunc>
TAny<TSharedLibrary, TFunc> any(const std::initializer_list<dll_function<TSharedLibrary, TFunc>> &fns) {
    return TAny<TSharedLibrary, TFunc>(fns);
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
template<typename TSharedLibrary>
class GpuUtilization {
    DECLARE(nvmlDeviceGetUtilizationRates, fnGetUtilizationRates);
    DECLARE(nvmlDeviceGetEncoderUtilization, fnGetEncoderUtilization);
    DECLARE(nvmlDeviceGetDecoderUtilization, fnGetDecoderUtilization);
public:
    GpuUtilization &operator=(const TSharedLibrary &dl) {
        fnGetUtilizationRates = dl;
        fnGetEncoderUtilization = dl;
        fnGetDecoderUtilization = dl;
        return *this;
    }

    void Collect(nvmlDevice_t dev, std::map<std::string, std::string> &data) const {
        nvmlUtilization_t rates;
        fnGetUtilizationRates(dev, &rates);
        data["gpu_util"] = fmt::format("{} %", rates.gpu);
        data["memory_util"] = fmt::format("{} %", rates.memory);

        unsigned int utilization = 0;
        unsigned int us = 0;
        fnGetEncoderUtilization(dev, &utilization, &us);
        data["encoder_util"] = fmt::format("{} %", utilization);
        fnGetDecoderUtilization(dev, &utilization, &us);
        data["decoder_util"] = fmt::format("{} %", utilization);
    }
};

template<typename TSharedLibrary>
class GpuTemperature {
    DECLARE(nvmlDeviceGetTemperature, fnGetTemperature);
    DECLARE(nvmlDeviceGetTemperatureThreshold, fnGetTemperatureThreshold);
public:
    GpuTemperature &operator=(const TSharedLibrary &dl) {
        fnGetTemperature = dl;
        fnGetTemperatureThreshold = dl;
        return *this;
    }

    bool Collect(nvmlDevice_t dev, std::map<std::string, std::string> &data, int format) const {
        unsigned int temp = 0;
        fnGetTemperature(dev, NVML_TEMPERATURE_GPU, &temp);
        data["gpu_temp"] = convert(temp) + " C";
        data["memory_temp"] = "N/A"; // nvml没有该接口
        data["gpu_target_temperature"] = "N/A";

        struct {
            nvmlTemperatureThresholds_t thresholdType;
            const char *key[2];
        } thresholds[] = {
                {NVML_TEMPERATURE_THRESHOLD_SHUTDOWN,      {"gpu_temp_max_threshold",     "gpu_temp_tlimit"}},
                {NVML_TEMPERATURE_THRESHOLD_SLOWDOWN,      {"gpu_temp_slow_threshold",    "gpu_temp_slow_tlimit_threshold"}},
                {NVML_TEMPERATURE_THRESHOLD_MEM_MAX,       {"gpu_temp_max_mem_threshold", "gpu_temp_max_mem_tlimit_threshold"}},
                {NVML_TEMPERATURE_THRESHOLD_GPU_MAX,       {"gpu_temp_max_gpu_threshold", "gpu_temp_max_gpu_tlimit_threshold"}},
                {NVML_TEMPERATURE_THRESHOLD_ACOUSTIC_MIN,  {nullptr,                      nullptr}}, // 噪音?
                {NVML_TEMPERATURE_THRESHOLD_ACOUSTIC_CURR, {nullptr,                      nullptr}},
                {NVML_TEMPERATURE_THRESHOLD_ACOUSTIC_MAX,  {nullptr,                      nullptr}},
        };
        for (const auto &it: thresholds) {
            if (const char *key = it.key[format - 1]) {
                unsigned int temperature = 0;
                auto result = fnGetTemperatureThreshold.native()(dev, it.thresholdType, &temperature);
                if (result == NVML_SUCCESS) {
                    data[key] = convert(temperature) + " C";
                } else {
                    data[key] = "N/A";
                }
            }
        }

        return true;
    }
};

template<typename TSharedLibrary>
class GpuPowerReadings {
    DECLARE(nvmlDeviceGetPowerState, fnGetPowerState);  // power_state
    DECLARE(nvmlDeviceGetPowerUsage, fnGetPowerUsage);  // power_draw
    DECLARE(nvmlDeviceGetPowerManagementLimit, fnGetPowerManagementLimit); // power_limit
    // min_power_limit、max_power_limit
    DECLARE(nvmlDeviceGetPowerManagementLimitConstraints, fnGetPowerManagementLimitConstraints);
    // default_power_limit
    DECLARE(nvmlDeviceGetPowerManagementDefaultLimit, fnGetPowerManagementDefaultLimit);
    // (request_|enforced_)power_limit
    DECLARE(nvmlDeviceGetEnforcedPowerLimit, fnGetEnforcedPowerLimit);
    DECLARE(nvmlDeviceGetPowerManagementMode, fnGetPowerManagementMode); // power_management
public:
    GpuPowerReadings &operator=(const TSharedLibrary &dl) {
        fnGetPowerState = dl;
        fnGetPowerUsage = dl;
        fnGetPowerManagementLimit = dl;
        fnGetPowerManagementLimitConstraints = dl;
        fnGetPowerManagementDefaultLimit = dl;
        fnGetEnforcedPowerLimit = dl;
        fnGetPowerManagementMode = dl;
        return *this;
    }

    bool Collect(nvmlDevice_t dev, std::map<std::string, std::string> &data, int format) const {
        nvmlPstates_t state;
        fnGetPowerState(dev, &state);
        data["power_state"] = (state == NVML_PSTATE_UNKNOWN ? "N/A" : fmt::format("P{}", (int) state));

        if (format == 1) {
            nvmlEnableState_t mode;
            fnGetPowerManagementMode(dev, &mode);
            data["power_management"] = (mode == NVML_FEATURE_ENABLED ? "Supported" : "Unsupported");
        }

        execGet(fnGetPowerUsage, data, "power_draw", dev);
        execGet(fnGetPowerManagementLimit, data, (format == 2 ? "current_power_limit" : "power_limit"), dev);
        execGet(fnGetPowerManagementDefaultLimit, data, "default_power_limit", dev);
        execGet(fnGetEnforcedPowerLimit, data, (format == 2 ? "requested_power_limit" : "enforced_power_limit"), dev);

        unsigned int minLimit = 0, maxLimit = 0;
        fnGetPowerManagementLimitConstraints(dev, &minLimit, &maxLimit);
        data["min_power_limit"] = convertMilliWatts(minLimit);
        data["max_power_limit"] = convertMilliWatts(maxLimit);

        return true;
    }

private:
    static std::string convertMilliWatts(unsigned int v) {
        return fmt::format("{:.02f} W", v / 1000.0); // 小数点后两位
    }

    template<typename TF>
    static void execGet(const TF &fn, std::map<std::string, std::string> &data,
                        const std::string &key, nvmlDevice_t dev) {
        unsigned int value = 0;
        fn.native()(dev, &value);
        data[key] = convertMilliWatts(value);
    }
};

enum {
    MiB = 1024 * 1024,
};

template<int>
constexpr const char *byteUnit() = delete;

template<>
constexpr const char *byteUnit<MiB>() {
    return "MiB";
}

template<int Base, typename T>
std::string convertBytes(const T &n) {
    return fmt::format("{} {}", (n / Base), byteUnit<Base>());
}

template<typename TSharedLibrary>
class GpuMemory {
    DECLARE(nvmlDeviceGetMemoryInfo_v2, fnGetMemory2);
    DECLARE(nvmlDeviceGetMemoryInfo, fnGetMemory);
public:
    GpuMemory &operator=(const TSharedLibrary &dl) {
        IGNORE_EXCEPT(fnGetMemory2 = dl);
        IGNORE_EXCEPT(fnGetMemory = dl);
        checkAnyLoaded(fnGetMemory2, fnGetMemory);

        return *this;
    }

    static void convertTo(const nvmlMemory_t &memInfo, std::map<std::string, std::string> &data) {
        data["total"] = convertBytes<MiB>(memInfo.total);
        data["used"] = convertBytes<MiB>(memInfo.used);
        data["free"] = convertBytes<MiB>(memInfo.free);
    }

    static void convertTo(const nvmlMemory_v2_t &memInfo, std::map<std::string, std::string> &data) {
        data["total"] = convertBytes<MiB>(memInfo.total);
        data["reserved"] = convertBytes<MiB>(memInfo.reserved);
        data["used"] = convertBytes<MiB>(memInfo.used);
        data["free"] = convertBytes<MiB>(memInfo.free);
    }

    void Collect(nvmlDevice_t device, std::map<std::string, std::string> &data) const {
        if (fnGetMemory2) {
            nvmlMemory_v2_t memInfo = {0};
            memInfo.version = nvmlMemory_v2; // 必须初始化version, 否则返回13, Function Not Found
            fnGetMemory2(device, &memInfo);
            convertTo(memInfo, data);
            return;
        }
        nvmlMemory_t memInfo = {0};
        fnGetMemory(device, &memInfo);
        convertTo(memInfo, data);
    }
};


template<typename TSharedLibrary>
class GpuDevice {
    DECLARE(nvmlDeviceGetVbiosVersion, fnGetVbiosVersion);
	DECLARE(nvmlDeviceGetCount_v2, fnGetDeviceCount2);
	DECLARE(nvmlDeviceGetCount, fnGetDeviceCount);

    DECLARE(nvmlDeviceGetPciInfo_v2, fnGetPciInfo2);
    DECLARE(nvmlDeviceGetPciInfo_v3, fnGetPciInfo3);

	DECLARE(nvmlDeviceGetHandleByIndex_v2, fnGetHandleByIndex2);
	DECLARE(nvmlDeviceGetHandleByIndex, fnGetHandleByIndex);

    GpuTemperature<TSharedLibrary> temperature;
    GpuMemory<TSharedLibrary> memory;
    GpuPowerReadings<TSharedLibrary> powerReadings;
    GpuUtilization<TSharedLibrary> utilization;

public:
    GpuDevice &operator=(const TSharedLibrary &dl) {
        IGNORE_EXCEPT(fnGetPciInfo2 = dl)
        IGNORE_EXCEPT(fnGetPciInfo3 = dl)
        checkAnyLoaded(fnGetPciInfo2, fnGetPciInfo3);

        IGNORE_EXCEPT(fnGetDeviceCount2 = dl);
        IGNORE_EXCEPT(fnGetDeviceCount = dl);
        checkAnyLoaded(fnGetDeviceCount2, fnGetDeviceCount);

        IGNORE_EXCEPT(fnGetHandleByIndex2 = dl);
        IGNORE_EXCEPT(fnGetHandleByIndex = dl);
        checkAnyLoaded(fnGetHandleByIndex2, fnGetHandleByIndex);

        fnGetVbiosVersion = dl;
        temperature = dl;
        memory = dl;
        powerReadings = dl;
        utilization = dl;

        return *this;
    }

    bool Collect(cloudMonitor::GpuData &data, int format) const {
        unsigned int deviceCount = GetDeviceCount();
        if (deviceCount > 0) {
            data.gpus.reserve(deviceCount);
        }
        // LogDebug("Gpu<{}>, deviceCount: {}", V, deviceCount);
        for (unsigned int index = 0; index < deviceCount; index++) {
            nvmlDevice_t dev = GetDeviceByIndex(index);

            nvmlPciInfo_t pci;
            getPciInfo(dev, pci);

            cloudMonitor::GpuInfo info;
            info.id = pci.busId;
            // LogDebug("Gpu<{}>, [{}], id: {}", V, index, info.id);

            char vbiosVersion[NVML_DEVICE_VBIOS_VERSION_BUFFER_SIZE] = {0};
            fnGetVbiosVersion(dev, vbiosVersion, sizeof(vbiosVersion));
            info.vBiosVersion = vbiosVersion;
            // LogDebug("Gpu<{}>, [{}], vbios_version: {}", V, index, info.vBiosVersion);

            temperature.Collect(dev, info.temperatureMap, format);
            powerReadings.Collect(dev, info.powMap, format);
            memory.Collect(dev, info.memMap);
            utilization.Collect(dev, info.utilMap);

            data.gpus.push_back(info);
        }
        return true;
    }

    nvmlDevice_t GetDeviceByIndex(unsigned int index) const {
        nvmlDevice_t device = nullptr;
        any({fnGetHandleByIndex2, fnGetHandleByIndex}).call(index, &device);
        return device;
    }

    void getPciInfo(nvmlDevice_t dev, nvmlPciInfo_t &pci) const {
        any({fnGetPciInfo3, fnGetPciInfo2}).call(dev, &pci);
    }

    unsigned int GetDeviceCount() const {
        unsigned int deviceCount = 0;
        any({fnGetDeviceCount2, fnGetDeviceCount}).call(&deviceCount);
        return deviceCount;
    }
};

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
struct INvml {
    virtual ~INvml() = 0;
    virtual bool Collect(cloudMonitor::GpuData &, int) const = 0;

    virtual std::string CollectToXml(int format) const {
        std::string s;
        cloudMonitor::GpuData data;
        if (Collect(data, format)) {
            s = data.toXml();
        }
        return s;
    }
};

struct INvmlEx : public INvml {
    virtual bool IsOK() const = 0;
};

template<typename TSharedLibrary>
class CNvml : public INvmlEx {
    bool ok = false;
    TSharedLibrary dl;

    DECLARE(nvmlShutdown, fnShutdown);
    DECLARE(nvmlSystemGetDriverVersion, fnGetDriverVersion);

    GpuDevice<TSharedLibrary> device;
public:
    explicit CNvml(const TSharedLibrary &d) : dl(d) {
        try {
			DECLARE(nvmlInit_v2, fnInit2);
			DECLARE(nvmlInit, fnInit);

			IGNORE_EXCEPT(fnInit2 = dl);
            IGNORE_EXCEPT(fnInit = dl);
            checkAnyLoaded(fnInit2, fnInit);

            fnShutdown = dl;
            fnGetDriverVersion = dl;
            device = dl;

			any({ fnInit2, fnInit }).call();

            ok = true;
        } catch (const std::exception &ex) {
            LogWarn("load <{}> error: {}", dl.location().string(), ex.what());
        }
    }

    ~CNvml() override {
        if (fnShutdown) {
            fnShutdown();
        }
    }

    bool IsOK() const override {
        return ok;
    }

    // format: 1, 2
    bool Collect(cloudMonitor::GpuData &data, int format) const override {
        try {
            char version[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE] = {0};
            fnGetDriverVersion(version, sizeof(version));
            data.driverVersion = version;
            // LogDebug("Gpu<{}>, driverVersion: {}", V, data.driverVersion);

            return device.Collect(data, format);
        } catch (const std::exception &ex) {
            LogError("CNvml::Collect error: {}", ex.what());
        }
        return false;
    }
};

#undef IGNORE_EXCEPT
#undef DECLARE

namespace cloudMonitor {
    int toolCollectNvml(const char *argv0, int argc, const char *const *argv, std::ostream &os);
#ifdef WIN32
#   define DLL_NAME "C:/Program Files/NVIDIA Corporation/NVSMI/nvml.dll"
#elif defined(__linux__) || defined(__unix__)
#   define DLL_NAME "libnvidia-ml.so.1"
#elif defined(__APPLE__) || defined(__FreeBSD__)
#   define DLL_NAME "libnvidia-ml.dylib"
#else
#   define DLL_NAME ""
#endif

    template<typename TSharedLibrary>
    static std::shared_ptr<INvml> NewNvml(const TSharedLibrary &dl) {
        std::shared_ptr<INvmlEx> ret = std::make_shared<CNvml<TSharedLibrary>>(dl);
        if (ret->IsOK()) {
            LogInfo("load <{}> successfully", dl.location().string());
        }
        return ret->IsOK() ? ret : nullptr;
    }

    template<typename TSharedLibrary = boost::dll::shared_library>
    static std::shared_ptr<INvml> NewNvml(LogLevel level = LEVEL_WARN) {
        TSharedLibrary dl;
        std::string dllName = SingletonConfig::Instance()->GetValue("nvidia.nvml.path", DLL_NAME);
        if (!dllName.empty()) {
            boost::system::error_code ec;
            dl.load(dllName, ec, boost::dll::load_mode::search_system_folders);
            if (ec.failed()) {
                Log(level, "load <{}> failed: {}", dllName, ec.message());
            }
        }
        return dl.is_loaded() ? NewNvml(dl) : nullptr;
    }
}

#endif // !ARGUS_CMS_GPU_COLLECT_NVML_H
