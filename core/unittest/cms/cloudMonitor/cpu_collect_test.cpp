#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include "cloudMonitor/cpu_collect.h"
#include "common/Config.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/ConsumeCpu.h"
#include "common/ModuleTool.h"
#include "common/Defer.h"
#include "cloudMonitor/cloud_module_macros.h"

#ifdef min
#	undef min
#endif

using namespace std;
using namespace common;
using namespace cloudMonitor;

DECLARE_CMS_EXTERN_MODULE(cpu);

namespace cloudMonitor {
    extern atomic<uint32_t> cpuCollectInstanceCount;;
}
class Cms_CpuCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(Cms_CpuCollectTest, Collect)
{
    const int maxCount = 3;
    CpuCollect collector;
    collector.Init(maxCount);
    auto p_shared = &collector;

    string collectStr;

    for (int i = 0; i < maxCount + 1; i++) {
        auto start = chrono::system_clock::now();
        volatile uint64_t sum = 0;
        int64_t micros = chrono::duration_cast<chrono::microseconds>(start.time_since_epoch()).count();
        for (int64_t j = 0; j < micros / 10000000; j++) {
            sum += j * micros;
        }
        auto dura = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - start);
        LogDebug("time[{}]: {} ms", i, dura.count());
        std::this_thread::sleep_for(chrono::milliseconds(500) - dura);

        int collectLen = p_shared->Collect(collectStr);
        if (i < maxCount) {
            EXPECT_EQ(collectLen, 0);
        } else {
            EXPECT_EQ(collectLen > 0, true);
            cout << "collectLen= " << collectLen << endl;
            // cout << "collectResult= " << collectStr << endl;
            CollectData collectData;
            ModuleData::convertStringToCollectData(collectStr, collectData);
            EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
            EXPECT_EQ(collectData.dataVector.size(), p_shared->mCoreNumber + 1);
        }
    }
}

TEST_F(Cms_CpuCollectTest, cpu) {
    {
        char *buf = nullptr;
        EXPECT_EQ(-1, cloudMonitor_cpu_collect(nullptr, &buf));
        EXPECT_EQ(buf, nullptr);
    }

    IHandler *handler = cloudMonitor_cpu_init("10");
    EXPECT_NE(nullptr, handler);
    defer(cloudMonitor_cpu_exit(handler));

    for (int i = 0; i < 10; i++) {
        consumeCpu();
        char *buf = nullptr;
        EXPECT_EQ(0, cloudMonitor_cpu_collect(handler, &buf));
        EXPECT_NE(buf, nullptr);
    }
    consumeCpu();
    {
        char *buf = nullptr;
        EXPECT_GT(cloudMonitor_cpu_collect(handler, &buf), 0);
        cout << "result[:512]:" << std::string(buf, std::min(size_t(512), strlen(buf))) << endl;
    }
    for (int i = 0; i < 9; i++) {
        consumeCpu();
        char *buf = nullptr;
        EXPECT_EQ(0, cloudMonitor_cpu_collect(handler, &buf));
    }
    consumeCpu();
    {
        char *buf = nullptr;
        EXPECT_GT(cloudMonitor_cpu_collect(handler, &buf), 0);
        cout << "result[:512]:" << std::string(buf).substr(0, 512) << endl;
    }

    EXPECT_EQ(uint32_t(1), cpuCollectInstanceCount.load());
}
