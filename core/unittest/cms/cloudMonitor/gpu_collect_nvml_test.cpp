//
// Created by 韩呈杰 on 2024/1/22.
//
#include <gtest/gtest.h>
#include "cloudMonitor/gpu_collect.h"
#include "cloudMonitor/gpu_collect_nvml.h"

#include <boost/system/error_code.hpp>

#include "common/FilePathUtils.h"
#include "common/ThrowWithTrace.h"
#include "common/Logger.h"
#include "common/Defer.h"
#include "common/ModuleData.h"
#include "common/Common.h"

using namespace common;

struct MockSharedLibrary {
    struct data_t {
        std::string path;
        std::map<std::string, void *> mapFunc;
    };
    std::shared_ptr<data_t> data;

    explicit MockSharedLibrary(std::map<std::string, void *> &&src = std::map<std::string, void *>{}) {
        data = std::make_shared<data_t>();
        data->mapFunc = std::move(src);
    }

    template<typename ...ArgTypes>
    void load(const std::string &libPath, boost::system::error_code &ec, ArgTypes ...) const {
        ec.clear();
        ASSERT_TRUE((bool) data);

        if (data->mapFunc.empty()) {
            ec = boost::system::errc::make_error_code(boost::system::errc::device_or_resource_busy);
        } else {
            data->path = libPath;
        }
    }

    fs::path location() const {
        return data->path;
    }

    bool is_loaded() const {
        EXPECT_TRUE((bool) data);
        return !data->mapFunc.empty();
    }

    template<typename T>
    typename boost::disable_if_c<std::is_member_pointer<T>::value || std::is_reference<T>::value, T &>::type
    get(const std::string &symbolName) const;
};


template<typename T>
typename boost::disable_if_c<std::is_member_pointer<T>::value || std::is_reference<T>::value, T &>::type
MockSharedLibrary::get(const std::string &symbolName) const {
    auto it = data->mapFunc.find(symbolName);
    // LogDebug("T &: {}, data->mapFunc[\"{}\"] => {}", boost::core::demangle(typeid(T &).name()),
    //          symbolName, (it != data->mapFunc.end() ? it->second : nullptr));
    if (it != data->mapFunc.end()) {
        return *reinterpret_cast<T *>(it->second);
    }
    Throw<std::runtime_error>("no symbol found: {}", symbolName);
}

using namespace cloudMonitor;

namespace nvmlMock {
    extern "C" const char *ErrorString(nvmlReturn_t result) {
        std::map<nvmlReturn_t, const char *> mapNvmlError{
                {NVML_SUCCESS,           "The operation was successful"},
                {NVML_ERROR_NO_DATA,     "No data"},
                {NVML_ERROR_GPU_IS_LOST, "The GPU has fallen off the bus or has otherwise become inaccessible"},
                {NVML_ERROR_IN_USE, "An operation cannot be performed because the GPU is currently in use"},
                {NVML_ERROR_TIMEOUT, "User provided timeout passed"},
        };
        auto it = mapNvmlError.find(result);
        return it != mapNvmlError.end() ? it->second : nullptr;
    }

    nvmlDevice_t _gpu1;

    nvmlReturn_t initResult = NVML_SUCCESS;
    extern "C" nvmlReturn_t Init() {
        return initResult;
    }

    nvmlReturn_t shutdownResult = NVML_SUCCESS;
    extern "C" nvmlReturn_t Shutdown() {
        return shutdownResult;
    }

    nvmlUtilization_t utilizationRates;
    extern "C" nvmlReturn_t GetUtilizationRates(nvmlDevice_t, nvmlUtilization_t *p) {
        *p = utilizationRates;
        return NVML_SUCCESS;
    }

    unsigned int encoderUtilization = 0;
    extern "C" nvmlReturn_t GetEncoderUtilization(nvmlDevice_t, unsigned int *utilization,
                                                  unsigned int *samplingPeriodUs) {
        *utilization = encoderUtilization;
        *samplingPeriodUs = 0;
        return NVML_SUCCESS;
    }
    unsigned int decoderUtilization = 0;
    extern "C" nvmlReturn_t GetDecoderUtilization(nvmlDevice_t, unsigned int *utilization,
                                                  unsigned int *samplingPeriodUs) {
        *utilization = decoderUtilization;
        *samplingPeriodUs = 0;
        return NVML_SUCCESS;
    }

    unsigned int temperature = 0;
    extern "C" nvmlReturn_t GetTemperature(nvmlDevice_t, nvmlTemperatureSensors_t,
                                           unsigned int *temp) {
        *temp = temperature;
        return NVML_SUCCESS;
    }
    std::map<nvmlTemperatureThresholds_t, unsigned int> mapTemperatureThreshold;
    extern "C" nvmlReturn_t GetTemperatureThreshold(nvmlDevice_t, nvmlTemperatureThresholds_t thresholdType,
                                                    unsigned int *temp) {
        auto it = mapTemperatureThreshold.find(thresholdType);
        if (it != mapTemperatureThreshold.end()) {
            *temp = it->second;
            return NVML_SUCCESS;
        }
        return NVML_ERROR_NO_DATA;
    }

    nvmlPstates_t powerState = NVML_PSTATE_0;
    extern "C" nvmlReturn_t GetPowerState(nvmlDevice_t, nvmlPstates_t *pState) {
        *pState = powerState;
        return NVML_SUCCESS;
    }
    unsigned int powerUsage = 0;
    extern "C" nvmlReturn_t GetPowerUsage(nvmlDevice_t, unsigned int *power) {
        *power = powerUsage;
        return NVML_SUCCESS;
    }
    unsigned int powerLimit = 0;
    extern "C" nvmlReturn_t GetPowerManagementLimit(nvmlDevice_t, unsigned int *limit) {
        *limit = powerLimit;
        return NVML_SUCCESS;
    }
    unsigned int powerMinLimit = 0;
    unsigned int powerMaxLimit;
    extern "C" nvmlReturn_t GetPowerManagementLimitConstraints(nvmlDevice_t,
                                                               unsigned int *minLimit, unsigned int *maxLimit) {
        *minLimit = powerMinLimit;
        *maxLimit = powerMaxLimit;
        return NVML_SUCCESS;
    }
    unsigned int powerDefaultLimit = 0;
    extern "C" nvmlReturn_t GetPowerManagementDefaultLimit(nvmlDevice_t, unsigned int *defaultLimit) {
        *defaultLimit = powerDefaultLimit;
        return NVML_SUCCESS;
    }
    unsigned int enforcePowerLimit = 0;
    extern "C" nvmlReturn_t GetEnforcedPowerLimit(nvmlDevice_t, unsigned int *limit) {
        *limit = enforcePowerLimit;
        return NVML_SUCCESS;
    }
    nvmlEnableState_t powerMode = NVML_FEATURE_ENABLED;
    extern "C" nvmlReturn_t GetPowerManagementMode(nvmlDevice_t, nvmlEnableState_t *mode) {
        *mode = powerMode;
        return NVML_SUCCESS;
    }
    nvmlMemory_t memory1;
    extern "C" nvmlReturn_t GetMemoryInfo(nvmlDevice_t, nvmlMemory_t *memory) {
        *memory = memory1;
        return NVML_SUCCESS;
    }
    nvmlMemory_v2_t memory2;
    extern "C" nvmlReturn_t GetMemoryInfo_v2(nvmlDevice_t, nvmlMemory_v2_t *memory) {
        *memory = memory2;
        return NVML_SUCCESS;
    }
    unsigned int deviceCount1 = 1;
    extern "C" nvmlReturn_t GetCount(unsigned int *deviceCount) {
        if (deviceCount1 > 0) {
            *deviceCount = deviceCount1;
            return NVML_SUCCESS;
        }
        return NVML_ERROR_GPU_IS_LOST;
    }
    unsigned int deviceCount2 = 1;
    extern "C" nvmlReturn_t GetCount_v2(unsigned int *deviceCount) {
        *deviceCount = deviceCount2;
        return NVML_SUCCESS;
    }
    const char *vbiosVersion = "95.02.66.00.02";
    extern "C" nvmlReturn_t GetVbiosVersion(nvmlDevice_t, char *version, unsigned int) {
        strcpy(version, vbiosVersion);
        return NVML_SUCCESS;
    }

    std::string pciInfo_v2_BusId = "00000000:5E:00.2";
    extern "C" nvmlReturn_t GetPciInfo_v2(nvmlDevice_t, nvmlPciInfo_t *pci) {
        strcpy(pci->busId, pciInfo_v2_BusId.c_str());
        return NVML_SUCCESS;
    }
    std::string pciInfo_v3_BusId = "00000000:5E:00.2";
    extern "C" nvmlReturn_t GetPciInfo_v3(nvmlDevice_t, nvmlPciInfo_t *pci) {
        strcpy(pci->busId, pciInfo_v3_BusId.c_str());
        return NVML_SUCCESS;
    }
    extern "C" nvmlReturn_t GetHandleByIndex(unsigned int index, nvmlDevice_t *device) {
        if (index == 0) {
            *device = _gpu1;
        }
        return NVML_SUCCESS;
    }
    extern "C" nvmlReturn_t GetHandleByIndex_v2(unsigned int index, nvmlDevice_t *device) {
        if (index == 0) {
            *device = _gpu1;
        }
        return NVML_SUCCESS;
    }
    const char *driverVersion = "535.129.03";
    extern "C" nvmlReturn_t GetDriverVersion(char *version, unsigned int /*length*/) {
        strcpy(version, driverVersion);
        return NVML_SUCCESS;
    }

    std::map<std::string, void *> apiTableV1{
            {"nvmlErrorString",                              (void *) &nvmlMock::ErrorString},
            {"nvmlShutdown",                                 (void *) &nvmlMock::Shutdown},
            {"nvmlDeviceGetUtilizationRates",                (void *) &nvmlMock::GetUtilizationRates},
            {"nvmlDeviceGetEncoderUtilization",              (void *) &nvmlMock::GetEncoderUtilization},
            {"nvmlDeviceGetDecoderUtilization",              (void *) &nvmlMock::GetDecoderUtilization},
            {"nvmlDeviceGetTemperature",                     (void *) &nvmlMock::GetTemperature},
            {"nvmlDeviceGetTemperatureThreshold",            (void *) &nvmlMock::GetTemperatureThreshold},
            {"nvmlDeviceGetPowerState",                      (void *) &nvmlMock::GetPowerState},
            {"nvmlDeviceGetPowerUsage",                      (void *) &nvmlMock::GetPowerUsage},
            {"nvmlDeviceGetPowerManagementLimit",            (void *) &nvmlMock::GetPowerManagementLimit},
            {"nvmlDeviceGetPowerManagementLimitConstraints", (void *) &nvmlMock::GetPowerManagementLimitConstraints},
            {"nvmlDeviceGetPowerManagementDefaultLimit",     (void *) &nvmlMock::GetPowerManagementDefaultLimit},
            {"nvmlDeviceGetEnforcedPowerLimit",              (void *) &nvmlMock::GetEnforcedPowerLimit},
            {"nvmlDeviceGetPowerManagementMode",             (void *) &nvmlMock::GetPowerManagementMode},
            {"nvmlDeviceGetVbiosVersion",                    (void *) &nvmlMock::GetVbiosVersion},
            {"nvmlDeviceGetPciInfo_v2",                      (void *) &nvmlMock::GetPciInfo_v2},
            {"nvmlDeviceGetPciInfo_v3",                      (void *) &nvmlMock::GetPciInfo_v3},
            {"nvmlSystemGetDriverVersion",                   (void *) &nvmlMock::GetDriverVersion},

            {"nvmlInit",                                     (void *) &nvmlMock::Init},
            {"nvmlDeviceGetCount",                           (void *) &nvmlMock::GetCount},
            {"nvmlDeviceGetMemoryInfo",                      (void *) &nvmlMock::GetMemoryInfo},
            {"nvmlDeviceGetHandleByIndex",                   (void *) &nvmlMock::GetHandleByIndex}
    };

    std::map<std::string, void *> apiTableV2{
            {"nvmlInit_v2",                   (void *) &nvmlMock::Init},
            {"nvmlDeviceGetMemoryInfo_v2",    (void *) &nvmlMock::GetMemoryInfo_v2},
            {"nvmlDeviceGetCount_v2",         (void *) &nvmlMock::GetCount_v2},
            {"nvmlDeviceGetHandleByIndex_v2", (void *) &nvmlMock::GetHandleByIndex_v2},
    };
}

template<int V = 1>
std::map<std::string, void *> makeNvmlApi() {
    return nvmlMock::apiTableV1;
}

template<>
std::map<std::string, void *> makeNvmlApi<2>() {
    auto data = makeNvmlApi<1>();
    data.insert(nvmlMock::apiTableV2.begin(), nvmlMock::apiTableV2.end());
    return data;
}

TEST(CmsGpuNvmlTest, NewNvml_NoSo) {
    {
        auto nvml = NewNvml<MockSharedLibrary>();
        EXPECT_FALSE((bool) nvml);
    }
    {
        MockSharedLibrary sl(makeNvmlApi<1>());
        sl.data->path = "libnvml.so.xxx";
        EXPECT_TRUE((bool) NewNvml(sl));  // 初始化为v1
        sl.data->mapFunc.erase(sl.data->mapFunc.begin());
        EXPECT_FALSE((bool) NewNvml(sl)); // 初始化失败
    }
    {
        MockSharedLibrary sl(makeNvmlApi<1>());
        boost::system::error_code ec;
        sl.load("libnvml.so.1", ec);
        EXPECT_FALSE(ec.failed());
        EXPECT_TRUE(sl.is_loaded());
        sl.data->mapFunc = makeNvmlApi<2>();

        auto nvml = NewNvml(sl);
        EXPECT_TRUE((bool) nvml);
    }
}

TEST(CmsGpuNvmlTest, NvmlCallError) {
    dll_function<MockSharedLibrary, decltype(nvmlShutdown)> shutdown("nvmlShutdown");

    EXPECT_THROW(shutdown(), GpuException);
}

TEST(CmsGpuNvmlTest, GpuException) {
    auto old = nvmlMock::initResult;
    nvmlMock::initResult = NVML_ERROR_UNINITIALIZED;
    defer(nvmlMock::initResult = old);

    MockSharedLibrary sl(makeNvmlApi<2>());
    dll_function<MockSharedLibrary, decltype(nvmlInit_v2)> fnInit("nvmlInit_v2");
    fnInit = sl;
    EXPECT_THROW(fnInit(), GpuException);
    EXPECT_TRUE(fnInit.native().operator bool());

    try {
        fnInit();
        EXPECT_FALSE(true); // 应该抛出异常，而不是走到此处
    } catch (const GpuException &ex) {
        LogDebug("{}: {}", common::typeName(&ex), ex.what());
        EXPECT_NE(std::string::npos, std::string{ex.what()}.find("nvmlInit_v2"));

        GpuException ex2(ex);
        EXPECT_STREQ(ex2.what(), ex.what());
    }
}

TEST(CmsGpuNvmlTest, ErrorString) {
    dll_function<MockSharedLibrary, decltype(nvmlErrorString)> errorString0; // 缺省构造
    EXPECT_STREQ(errorString0.Name(), "nvmlErrorString");
    EXPECT_FALSE((bool) errorString0);

    dll_function<MockSharedLibrary, decltype(nvmlErrorString)> errorString;
    ASSERT_STREQ(errorString.Name(), "nvmlErrorString");
    {
        MockSharedLibrary sl;
        sl.data->mapFunc[errorString.Name()] = (void *) &nvmlMock::ErrorString;

        errorString = sl;
        EXPECT_TRUE((bool) errorString);
        LogDebug("errorString: NVML_SUCCESS: {}", errorString(NVML_SUCCESS));
        EXPECT_NE(std::string::npos, errorString(NVML_SUCCESS).find("successful"));

        dll_function<MockSharedLibrary, decltype(nvmlErrorString)> other(sl);
        EXPECT_TRUE((bool) other);

        errorString = other;
        EXPECT_TRUE((bool) errorString);
    }
    EXPECT_TRUE((bool) errorString);
    std::cout << "NVML_SUCCESS: " << errorString(NVML_SUCCESS) << std::endl;
    EXPECT_EQ(errorString(NVML_SUCCESS), "The operation was successful");
    std::cout << "NVML_ERROR_UNKNOWN: " << errorString(NVML_ERROR_UNKNOWN) << std::endl;
    const auto expectUnknown = fmt::format("unknown error from nvml: {}", (int) NVML_ERROR_UNKNOWN);
    EXPECT_EQ(errorString(NVML_ERROR_UNKNOWN), expectUnknown);
}

TEST(CmsGpuNvmlTest, GetPciInfo) {
    MockSharedLibrary sl(makeNvmlApi<1>());
    {
        GpuDevice<MockSharedLibrary> device;
        device = sl;
        nvmlPciInfo_t pciInfo;
        device.getPciInfo(nullptr, pciInfo);
        EXPECT_STREQ(pciInfo.busId, nvmlMock::pciInfo_v3_BusId.c_str());
    }
    sl.data->mapFunc.erase("nvmlDeviceGetPciInfo_v3");
    {
        GpuDevice<MockSharedLibrary> device;
        device = sl;
        nvmlPciInfo_t pciInfo;
        device.getPciInfo(nullptr, pciInfo);
        EXPECT_STREQ(pciInfo.busId, nvmlMock::pciInfo_v2_BusId.c_str());
    }
    sl.data->mapFunc.erase("nvmlDeviceGetPciInfo_v2");
    {
        try {
            GpuDevice<MockSharedLibrary> device;
            device = sl;
            EXPECT_FALSE(true); // unreachable
        } catch (const std::exception &ex) {
            EXPECT_STREQ(ex.what(), "neither nvmlDeviceGetPciInfo_v2 nor nvmlDeviceGetPciInfo_v3 are found");
        }
    }
}

template<typename T>
class CTmp {
    T &value;
    T old;
public:
    CTmp(T &v, const T &newValue) : value(v) {
        old = v;
        v = newValue;
    }

    ~CTmp() {
        value = old;
    }
};

#define TMP_ASSIGN_02(V, N, num) CTmp<decltype(V)> __CTmp__ ## num ## __(V, N)
#define TMP_ASSIGN_01(V, N, num) TMP_ASSIGN_02(V, N, num)
#define TMP_ASSIGN(V, N) TMP_ASSIGN_01(V, N, __LINE__) // 临时赋值，出scope后，恢复原值

TEST(CmsGpuNvmlTest, GpuUtilization) {
    GpuUtilization<MockSharedLibrary> utilization;
    utilization = MockSharedLibrary{makeNvmlApi<1>()};

    nvmlUtilization_t rates;
    rates.gpu = 1;
    rates.memory = 2;
    TMP_ASSIGN(nvmlMock::utilizationRates, rates);
    TMP_ASSIGN(nvmlMock::encoderUtilization, 3);
    TMP_ASSIGN(nvmlMock::decoderUtilization, 4);

    std::map<std::string, std::string> data;
    utilization.Collect(nullptr, data);
    LogInfo("{}: {}", typeName(&utilization), boost::json::serialize(boost::json::value_from(data)));
    std::map<std::string, std::string> expect{
            {"gpu_util",     "1 %"},
            {"memory_util",  "2 %"},
            {"encoder_util", "3 %"},
            {"decoder_util", "4 %"},
    };
    EXPECT_EQ(data, expect);
}

TEST(CmsGpuNvmlTest, GpuTemperature) {
    TMP_ASSIGN(nvmlMock::temperature, 1);
    std::map<nvmlTemperatureThresholds_t, unsigned int> mapNew{
            {NVML_TEMPERATURE_THRESHOLD_SHUTDOWN, 2},
            {NVML_TEMPERATURE_THRESHOLD_SLOWDOWN, 3},
            {NVML_TEMPERATURE_THRESHOLD_MEM_MAX,  4},
            // {NVML_TEMPERATURE_THRESHOLD_GPU_MAX, 5},  // N/A
    };
    TMP_ASSIGN(nvmlMock::mapTemperatureThreshold, mapNew);

    {
        GpuTemperature<MockSharedLibrary> gpuTemperature;
        gpuTemperature = MockSharedLibrary(makeNvmlApi<2>());

        std::map<std::string, std::string> data;
        gpuTemperature.Collect(nullptr, data, 2);
        LogInfo("{}: {}", typeName(&gpuTemperature), boost::json::serialize(boost::json::value_from(data)));
        std::map<std::string, std::string> expect{
                {"gpu_temp",                          "1 C"},
                {"memory_temp",                       "N/A"},
                {"gpu_target_temperature",            "N/A"},
                {"gpu_temp_tlimit",                   "2 C"},
                {"gpu_temp_slow_tlimit_threshold",    "3 C"},
                {"gpu_temp_max_mem_tlimit_threshold", "4 C"},
                {"gpu_temp_max_gpu_tlimit_threshold", "N/A"},
        };
        EXPECT_EQ(data, expect);
    }
    {
        GpuTemperature<MockSharedLibrary> gpuTemperature;
        gpuTemperature = MockSharedLibrary(makeNvmlApi<1>());

        std::map<std::string, std::string> data;
        gpuTemperature.Collect(nullptr, data, 1);
        LogInfo("{}: {}", typeName(&gpuTemperature), boost::json::serialize(boost::json::value_from(data)));
        std::map<std::string, std::string> expect{
                {"gpu_temp", "1 C"},
                {"memory_temp", "N/A"},
                {"gpu_target_temperature", "N/A"},
                {"gpu_temp_max_threshold", "2 C"},
                {"gpu_temp_slow_threshold", "3 C"},
                {"gpu_temp_max_mem_threshold", "4 C"},
                {"gpu_temp_max_gpu_threshold", "N/A"},
        };
        EXPECT_EQ(data, expect);
    }
}

TEST(CmsGpuNvmlTest, GpuPowerReadings_v1) {
    TMP_ASSIGN(nvmlMock::powerMode, NVML_FEATURE_ENABLED);

    TMP_ASSIGN(nvmlMock::powerState, NVML_PSTATE_UNKNOWN);
    TMP_ASSIGN(nvmlMock::powerUsage, 1024);
    TMP_ASSIGN(nvmlMock::powerLimit, 1035);  // 1035 / 1000.0 != 1.035 ≈ 1.03499999999
    TMP_ASSIGN(nvmlMock::powerDefaultLimit, 1043);
    TMP_ASSIGN(nvmlMock::enforcePowerLimit, 1052);
    TMP_ASSIGN(nvmlMock::powerMinLimit, 1066);
    TMP_ASSIGN(nvmlMock::powerMaxLimit, 1077);

    std::map<std::string, std::string> expect{
            {"power_state",          "N/A"},
            {"power_draw",           "1.02 W"},
            {"power_management",     "Supported"},
            {"power_limit",          "1.03 W"},
            {"default_power_limit",  "1.04 W"},
            {"enforced_power_limit", "1.05 W"},
            {"min_power_limit",      "1.07 W"},
            {"max_power_limit",      "1.08 W"},
    };

    GpuPowerReadings<MockSharedLibrary> powerReadings;
    powerReadings = MockSharedLibrary(makeNvmlApi());

    {
        std::map<std::string, std::string> data;
        powerReadings.Collect(nullptr, data, 1);
        LogInfo("{}: {}", typeName(&powerReadings), boost::json::serialize(boost::json::value_from(data)));
        EXPECT_EQ(data, expect);
    }

    TMP_ASSIGN(nvmlMock::powerMode, NVML_FEATURE_DISABLED);
    expect["power_management"] = "Unsupported";
    {
        std::map<std::string, std::string> data;
        powerReadings.Collect(nullptr, data, 1);
        LogInfo("{}: {}", typeName(&powerReadings), boost::json::serialize(boost::json::value_from(data)));
        EXPECT_EQ(data, expect);
    }
}

TEST(CmsGpuNvmlTest, GpuPowerReadings_v2) {
    TMP_ASSIGN(nvmlMock::powerState, NVML_PSTATE_8);
    TMP_ASSIGN(nvmlMock::powerUsage, 1024);
    TMP_ASSIGN(nvmlMock::powerLimit, 1035); // current_power_limit
    TMP_ASSIGN(nvmlMock::powerDefaultLimit, 1043);
    TMP_ASSIGN(nvmlMock::enforcePowerLimit, 1052);
    TMP_ASSIGN(nvmlMock::powerMinLimit, 1066);
    TMP_ASSIGN(nvmlMock::powerMaxLimit, 1077);

    GpuPowerReadings<MockSharedLibrary> powerReadings;
    powerReadings = MockSharedLibrary(makeNvmlApi<2>());

    std::map<std::string, std::string> data;
    powerReadings.Collect(nullptr, data, 2);
    LogInfo("{}: {}", typeName(&powerReadings), boost::json::serialize(boost::json::value_from(data)));
    std::map<std::string, std::string> expect{
            {"power_state",           "P8"},
            {"power_draw",            "1.02 W"},
            {"current_power_limit",   "1.03 W"},
            {"default_power_limit",   "1.04 W"},
            {"requested_power_limit", "1.05 W"},
            {"min_power_limit",       "1.07 W"},
            {"max_power_limit",       "1.08 W"},
    };
    EXPECT_EQ(data, expect);
}

TEST(CmsGpuNvmlTest, GpuMemory_v1) {
    nvmlMemory_t memory;
    memory.total = 3 * MiB;
    memory.free = 2 * MiB;
    memory.used = 1 * MiB;
    TMP_ASSIGN(nvmlMock::memory1, memory);

    GpuMemory<MockSharedLibrary> gpuMemory;
    gpuMemory = MockSharedLibrary(makeNvmlApi());

    std::map<std::string, std::string> data;
    gpuMemory.Collect(nullptr, data);
    LogInfo("{}: {}", typeName(&gpuMemory), boost::json::serialize(boost::json::value_from(data)));
    std::map<std::string, std::string> expect{
            {"total", "3 MiB"},
            {"used",  "1 MiB"},
            {"free",  "2 MiB"},
    };
    EXPECT_EQ(data, expect);
}

TEST(CmsGpuNvmlTest, GpuMemory_v2) {
    nvmlMemory_v2_t memory;
    memory.total = 6 * MiB;
    memory.free = 2 * MiB;
    memory.used = 1 * MiB;
    memory.reserved = 3 * MiB;
    TMP_ASSIGN(nvmlMock::memory2, memory);

    GpuMemory<MockSharedLibrary> gpuMemory;
    gpuMemory = MockSharedLibrary(makeNvmlApi<2>());

    std::map<std::string, std::string> data;
    gpuMemory.Collect(nullptr, data);
    LogInfo("{}: {}", typeName(&gpuMemory), boost::json::serialize(boost::json::value_from(data)));
    std::map<std::string, std::string> expect{
            {"total",    "6 MiB"},
            {"used",     "1 MiB"},
            {"free",     "2 MiB"},
            {"reserved", "3 MiB"}
    };
    EXPECT_EQ(data, expect);
}

TEST(CmsGpuNvmlTest, Collect_v1) {
    TMP_ASSIGN(nvmlMock::deviceCount1, 2);
    MockSharedLibrary sl(makeNvmlApi<1>());
    auto nvml = NewNvml(sl);

    cloudMonitor::GpuData data;
    nvml->Collect(data, 1);

    std::string xmlText = nvml->CollectToXml(1);
    EXPECT_FALSE(xmlText.empty());
    std::cout << xmlText << std::endl;

    CollectData collectData;
    cloudMonitor::GpuCollect::PackageCollectData("gpu", data, collectData);
    std::string s;
    ModuleData::convertCollectDataToString(collectData, s);
    std::cout << s << std::endl;

    EXPECT_NE(std::string::npos, s.find("gpu 2"));
    EXPECT_NE(std::string::npos, s.find(std::string("driver_version ") + nvmlMock::driverVersion));
    EXPECT_NE(std::string::npos, s.find(std::string("vbios_version ") + nvmlMock::vbiosVersion));
    EXPECT_NE(std::string::npos, s.find("power_readings_power_management Supported"));
}

TEST(CmsGpuNvmlTest, CollectFail) {
    TMP_ASSIGN(nvmlMock::deviceCount1, 0);  // 获取失败，抛出异常

    auto nvml = NewNvml(MockSharedLibrary(makeNvmlApi<1>()));
    ASSERT_TRUE((bool) nvml);

    cloudMonitor::GpuData data;
    EXPECT_FALSE(nvml->Collect(data, 1));
}

TEST(CmsGPuNvmlTest, toolCollectToXml) {
    cloudMonitor::toolCollectNvml(nullptr, 0, nullptr, std::cout);
}

TEST(CmsGpuNvmlTest, AnyCall) {
    dll_function<MockSharedLibrary, decltype(nvmlInit)> fnInit("nvmlInit");
    dll_function<MockSharedLibrary, decltype(nvmlShutdown)> fnShutdown("nvmlShutdown");

    // 未初始化，全部为false，导致call没能调用到任何函数
    try {
        any({fnInit, fnShutdown}).call();
        EXPECT_TRUE(false); // 不应到达此处
    } catch (const std::exception&ex) {
        LogError("{}: {}", typeName(&ex), ex.what());
        EXPECT_NE(std::string::npos, std::string{ex.what()}.find("no function valid"));
    }

    // 初始化
    MockSharedLibrary sl(makeNvmlApi<1>());
    fnInit = sl;
    fnShutdown = sl;

    // 常规调用
    EXPECT_NO_THROW(any({fnInit, fnShutdown}).call());

    // fnInit失败, fnShutdown成功
    TMP_ASSIGN(nvmlMock::initResult, NVML_ERROR_TIMEOUT);
    EXPECT_NO_THROW(any({fnInit, fnShutdown}).call());

    // 全部失败，最后抛出fnShutdown的异常
    TMP_ASSIGN(nvmlMock::shutdownResult, NVML_ERROR_IN_USE);
    try {
        any({fnInit, fnShutdown}).call();
        EXPECT_TRUE(false); // 不应到达此处
    } catch (const std::exception&ex) {
        LogError("{}: {}", typeName(&ex), ex.what());
        EXPECT_NE(std::string::npos, std::string{ex.what()}.find("in use"));
    }
}

TEST(CmsGpuNvmlTest, Collect) {
    GpuCollect collector;

}