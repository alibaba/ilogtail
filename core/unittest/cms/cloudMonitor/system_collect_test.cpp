#include <gtest/gtest.h>
#include "cloudMonitor/system_collect.h"

#include <iostream>
#include <chrono>
#include <thread>

#include "common/Config.h"
#include "common/ConsumeCpu.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/Defer.h"

#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace std::chrono;
using namespace common;
using namespace cloudMonitor;

DECLARE_CMS_EXTERN_MODULE(system);

class Cms_SystemCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();

        p_shared = new SystemCollect();
        p_shared->Init(15);
    }

    void TearDown() override {
        delete p_shared;
        p_shared = nullptr;
    }

    SystemCollect *p_shared{nullptr};
};

TEST_F(Cms_SystemCollectTest, Collect) {
    p_shared->mTotalCount = 2;
    for (int i = 0; i < p_shared->mTotalCount; i++) {
        string collectStr;
#ifdef WIN32
        EXPECT_EQ(p_shared->Collect(collectStr), 0);
#else
        auto startTime = steady_clock::now();
        int64_t start = duration_cast<microseconds>(startTime.time_since_epoch()).count();
        volatile int64_t sum = 0;
        for (int64_t counter = 0; counter < start / 10000000; counter++) {
            sum += counter * start;
        }
        auto endTime = steady_clock::now();
        std::this_thread::sleep_for(seconds{ 1 } - duration_cast<microseconds>(endTime - startTime));
        int collectLen = p_shared->Collect(collectStr);
        if (i < p_shared->mTotalCount - 1) {
            EXPECT_EQ(collectLen, 0);
        } else {
            EXPECT_EQ(collectLen > 0, true);
            cout << "collectLen= " << collectLen << endl;
            cout << "collectResult= " << collectStr << endl;
            CollectData collectData;
            ModuleData::convertStringToCollectData(collectStr, collectData);
            EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
            EXPECT_EQ(collectData.dataVector.size(), size_t(5));
        }
#endif
    }
}

TEST_F(Cms_SystemCollectTest, cpu) {
    EXPECT_EQ(-1, cloudMonitor_system_collect(nullptr, nullptr));
    auto *handler = cloudMonitor_system_init("10");
    defer(cloudMonitor_system_exit(handler));
    EXPECT_NE(nullptr, handler);
    char *buf = nullptr;
#ifdef WIN32
    // Windows环境无需采集
    EXPECT_EQ(0, cloudMonitor_system_collect(handler, &buf));
#else
    for (int i = 0; i < 9; i++) {
        consumeCpu(111);
        EXPECT_EQ(0, cloudMonitor_system_collect(handler, &buf));
    }
    consumeCpu();
    {
        EXPECT_GT(cloudMonitor_system_collect(handler, &buf), 0);
        cout << "result:" << buf << endl;
    }
    for (int i = 0; i < 9; i++) {
        consumeCpu(111);
        EXPECT_EQ(0, cloudMonitor_system_collect(handler, &buf));
    }
    consumeCpu();
    {
        EXPECT_GT(cloudMonitor_system_collect(handler, &buf), 0);
        cout << "result:" << buf << endl;
    }
#endif
}
