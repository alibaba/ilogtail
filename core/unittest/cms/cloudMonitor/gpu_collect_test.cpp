#include <gtest/gtest.h>
#include <iostream>
#include "cloudMonitor/gpu_collect.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/UnitTestEnv.h"
#include "common/FileUtils.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

class CmsGpuCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(CmsGpuCollectTest, CollectUnit) {
    GpuCollect pShared;
#ifdef WIN32
    const char *nvidia_smi = R"(C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe)";
#else
    const char *nvidia_smi = "/usr/bin/nvidia-smi";
#endif
    pShared.Init(nvidia_smi);
    string xmlContent = ReadFileContent((TEST_CONF_PATH / "conf" / "cloudMonitor" / "gpu" / "gpu.xml").string());
    GpuData gpuData;
    EXPECT_TRUE(pShared.ParserGpuData(xmlContent, gpuData));
    EXPECT_EQ(size_t(2), gpuData.gpus.size());
    EXPECT_EQ("418.39", gpuData.driverVersion);
    // for (auto &gpu: gpuData.gpus) {
    //     cout << "gpu_id=" << gpu.id << endl;
    //     cout << "gpu_bios_version=" << gpu.vBiosVersion << endl;
    //     map<string, string>::iterator it;
    //     for (it = gpu.temperatureMap.begin(); it != gpu.temperatureMap.end(); it++) {
    //         cout << "temperature_" + it->first << "=" << it->second << endl;
    //     }
    //     for (it = gpu.powMap.begin(); it != gpu.powMap.end(); it++) {
    //         cout << "pow_" + it->first << "=" << it->second << endl;
    //     }
    //     for (it = gpu.memMap.begin(); it != gpu.memMap.end(); it++) {
    //         cout << "mem_" + it->first << "=" << it->second << endl;
    //     }
    //     for (it = gpu.utilMap.begin(); it != gpu.utilMap.end(); it++) {
    //         cout << "util_" + it->first << "=" << it->second << endl;
    //     }
    // }
    std::cout << gpuData.toXml() << std::endl;

    CollectData collectData1, collectData2;
    pShared.PackageCollectData(gpuData, collectData1);
    collectData1.print();
    string data;
    ModuleData::convertCollectDataToString(collectData1, data);
    ModuleData::convertStringToCollectData(data, collectData2);
    collectData2.print();
}

TEST_F(CmsGpuCollectTest, Parse) {
    string xmlContent = ReadFileContent((TEST_CONF_PATH / "conf" / "cloudMonitor" / "gpu" / "gpu.xml").string());

    GpuData gpuData;
    EXPECT_TRUE(gpuData.Parse(xmlContent));

    EXPECT_EQ(gpuData.driverVersion, "418.39");
    EXPECT_EQ(gpuData.gpus.size(), size_t(2));

    auto print = [](const char *key, const std::map<std::string, std::string> &data) {
        std::cout << "  " << key << std::endl;
        for (const auto &tmpIt: data) {
            std::cout << "    " << tmpIt.first << ": " << tmpIt.second << std::endl;
        }
    };

    std::cout << "total " << gpuData.gpus.size() << " gpu(s)" << std::endl;
    int index = 0;
    for (const auto &curGpu: gpuData.gpus) {
        std::cout << std::endl << "gpu[" << (index++) << "], id: " << curGpu.id
                  << ", biosVersion: " << curGpu.vBiosVersion << std::endl;
        print("temperature", curGpu.temperatureMap);
        print("power_readings", curGpu.powMap);
        print("fb_memory_usage", curGpu.memMap);
        print("utilization", curGpu.utilMap);
    }

    CollectData cd;
    GpuCollect::PackageCollectData("gpu", gpuData, cd);
    ASSERT_EQ(cd.dataVector.size(), size_t(2));

    EXPECT_EQ(cd.dataVector[0].tagMap["temperature_gpu_temp"], "162");
    EXPECT_EQ(cd.dataVector[0].tagMap["power_readings_power_draw"], "131.43");
    EXPECT_EQ(cd.dataVector[0].tagMap["fb_memory_usage_free"], "1114594");
    EXPECT_EQ(cd.dataVector[0].tagMap["utilization_gpu_util"], "111");

    EXPECT_EQ(cd.dataVector[1].tagMap["temperature_gpu_temp"], "262");
    EXPECT_EQ(cd.dataVector[1].tagMap["power_readings_power_draw"], "231.43");
    EXPECT_EQ(cd.dataVector[1].tagMap["fb_memory_usage_free"], "214594");
    EXPECT_EQ(cd.dataVector[1].tagMap["utilization_gpu_util"], "221");
}

TEST_F(CmsGpuCollectTest, Parse535) {
    fs::path path = (TEST_CONF_PATH / "conf" / "cloudMonitor" / "gpu" / "gpu-535.129.03.xml");
    string xmlContent = ReadFileContent(path.string());

    GpuData gpuData;
    EXPECT_TRUE(gpuData.Parse(xmlContent));

    EXPECT_EQ(gpuData.driverVersion, "535.129.03");
    EXPECT_EQ(gpuData.gpus.size(), size_t(1));
    EXPECT_EQ(gpuData.gpus.capacity(), size_t(8));  // attached_gpus是8

    auto print = [](const char *key, const std::map<std::string, std::string> &data) {
        std::cout << "  " << key << std::endl;
        for (const auto &tmpIt: data) {
            std::cout << "    " << tmpIt.first << ": " << tmpIt.second << std::endl;
        }
    };

    std::cout << "total " << gpuData.gpus.size() << " gpu(s)" << std::endl;
    int index = 0;
    for (const auto &curGpu: gpuData.gpus) {
        std::cout << std::endl << "gpu[" << (index++) << "], id: " << curGpu.id
                  << ", biosVersion: " << curGpu.vBiosVersion << std::endl;
        print("temperature", curGpu.temperatureMap);
        print("power_readings", curGpu.powMap);
        print("fb_memory_usage", curGpu.memMap);
        print("utilization", curGpu.utilMap);
    }

    CollectData cd;
    GpuCollect::PackageCollectData("gpu", gpuData, cd);
    ASSERT_EQ(cd.dataVector.size(), size_t(1));

    EXPECT_EQ(cd.dataVector[0].tagMap["temperature_gpu_temp"], "24");
    EXPECT_EQ(cd.dataVector[0].tagMap["power_readings_power_draw"], "32.56");
    EXPECT_EQ(cd.dataVector[0].tagMap["fb_memory_usage_free"], "45593");
    EXPECT_EQ(cd.dataVector[0].tagMap["utilization_gpu_util"], "0");

    std::stringstream ss;
    cd.print(ss);
    LogDebug("\n{}", ss.str());
}

TEST_F(CmsGpuCollectTest, ParseErrorWithoutVersion) {
    const char *xmlContent = R"(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
	<timestamp>Fri Oct 11 16:45:58 2019</timestamp>
</nvidia_smi_log>
)";
    GpuData gpuData;
    EXPECT_FALSE(gpuData.Parse(xmlContent));
}

TEST_F(CmsGpuCollectTest, ParseWithouGpu) {
    const char *xmlContent = R"(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
	<driver_version>418.39</driver_version>
</nvidia_smi_log>
)";
    GpuData gpuData;
    EXPECT_TRUE(gpuData.Parse(xmlContent));
    EXPECT_EQ(gpuData.driverVersion, "418.39");
    EXPECT_TRUE(gpuData.gpus.empty());
}


TEST_F(CmsGpuCollectTest, ParseErrorWithouGpuId) {
    const char *xmlContent = R"(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
	<driver_version>418.39</driver_version>
    <gpu>
    </gpu>
</nvidia_smi_log>
)";
    GpuData gpuData;
    EXPECT_FALSE(gpuData.Parse(xmlContent));
    EXPECT_EQ(gpuData.driverVersion, "418.39");
    EXPECT_TRUE(gpuData.gpus.empty());
}

TEST_F(CmsGpuCollectTest, ParseErrorWithouVBiosVersion) {
    const char *xmlContent = R"(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
	<driver_version>418.39</driver_version>
    <gpu id="00000000:5E:00.0">
    </gpu>
</nvidia_smi_log>
)";
    GpuData gpuData;
    EXPECT_FALSE(gpuData.Parse(xmlContent));
    EXPECT_EQ(gpuData.driverVersion, "418.39");
    EXPECT_TRUE(gpuData.gpus.empty());
}

TEST_F(CmsGpuCollectTest, ParseNoMetric) {
    const char *xmlContent = R"(
<?xml version="1.0" ?>
<!DOCTYPE nvidia_smi_log SYSTEM "nvsmi_device_v10.dtd">
<nvidia_smi_log>
	<driver_version>418.39</driver_version>
    <gpu id="00000000:5E:00.0">
        <vbios_version>90.04.38.00.03</vbios_version>
    </gpu>
</nvidia_smi_log>
)";
    GpuData gpuData;
    EXPECT_TRUE(gpuData.Parse(xmlContent));
    EXPECT_EQ(gpuData.driverVersion, "418.39");
    EXPECT_EQ(gpuData.gpus.size(), size_t(1));
    EXPECT_FALSE(gpuData.gpus[0].vBiosVersion.empty());

    EXPECT_TRUE(gpuData.gpus[0].temperatureMap.empty());
    EXPECT_TRUE(gpuData.gpus[0].powMap.empty());
    EXPECT_TRUE(gpuData.gpus[0].memMap.empty());
    EXPECT_TRUE(gpuData.gpus[0].utilMap.empty());
}

TEST_F(CmsGpuCollectTest, CollectWithNvml) {
    Config newCfg;
    newCfg.Set("nvidia.nvml.enabled", "1");
    auto *cfg = SingletonConfig::swap(&newCfg);
    defer(SingletonConfig::swap(cfg));

    GpuCollect collector;
    EXPECT_TRUE(collector.enableNvml);
    std::string data;
    EXPECT_EQ(-1, collector.Collect(data));
}

TEST_F(CmsGpuCollectTest, Init) {
    {
        GpuCollect collector;
        EXPECT_EQ(0, collector.Init(nullptr));
    }
#ifndef WIN32
    {
        GpuCollect collector;
        EXPECT_EQ(0, collector.Init("/bin/ls"));
    }
#endif
}

TEST_F(CmsGpuCollectTest, GetXmlData) {
    GpuCollect collector;
    std::string s;
    EXPECT_FALSE(collector.GetGpuXmlData("/bin/ls", s));
}

TEST_F(CmsGpuCollectTest, Collect) {
    {
        GpuCollect collector;
        std::string data;
        EXPECT_EQ(-1, collector.Collect(data));
    }

    struct GpuCollectStub : GpuCollect {
        bool result;
        std::string xmlData;

#ifdef WIN32
#       define GPU_BIN "C:\\Windows\\explorer.exe";
#else
#       define GPU_BIN "/bin/ls";
#endif
        explicit GpuCollectStub(bool r = true): result(r) {
            this->mGpuBinPath = GPU_BIN;
        }

        bool GetGpuXmlData(const std::string &, std::string &data) const override {
            data = xmlData;
            return result;
        }
    };
    {
        GpuCollectStub collector(false);
        std::string data;
        EXPECT_EQ(-2, collector.Collect(data));
    }
    {
        GpuCollectStub collector;
        std::string data;
        EXPECT_EQ(-3, collector.Collect(data));
    }
    {
        GpuCollectStub collector;
        collector.xmlData = ReadFileContent(TEST_CONF_PATH / "conf" / "cloudMonitor" / "gpu" / "gpu.xml");
        std::string data;
        EXPECT_GT(collector.Collect(data), 0);
    }
}
