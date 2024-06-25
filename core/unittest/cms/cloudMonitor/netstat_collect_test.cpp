#include <gtest/gtest.h>
#include <iostream>
#include "cloudMonitor/netstat_collect.h"

#include "common/Config.h"
#include "common/SystemUtil.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/Defer.h"

#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

DECLARE_CMS_EXTERN_MODULE(netstat);

size_t countSimpleTcpState();  // implemented in netstat_collect.cpp

class Cms_NetStatCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
        p_shared = new NetStatCollect();
    }

    void TearDown() override {
        delete p_shared;
        p_shared = nullptr;
    }

    NetStatCollect *p_shared;
};

TEST_F(Cms_NetStatCollectTest, Collect) {
    string data;
    int collectLen = p_shared->Collect(data);
    EXPECT_EQ(collectLen > 0, true);
    EXPECT_EQ(collectLen, data.size());
    cout << "collectResult:" << data << endl;
    CollectData collectData;
    ModuleData::convertStringToCollectData(data, collectData);
    EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
    int expectCount = 8;
#ifdef ENABLE_UDP_SESSION
    expectCount++;
#endif
    EXPECT_EQ(collectData.dataVector.size(), expectCount);
    for (int i = 0; i < expectCount; i++) {
        if (i < 8) {
            EXPECT_EQ(collectData.dataVector[i].valueMap.size(), 1);
            EXPECT_EQ(collectData.dataVector[i].tagMap.size(), 3);
        } else {
            EXPECT_EQ(collectData.dataVector[i].valueMap.size(), 2);
            EXPECT_EQ(collectData.dataVector[i].tagMap.size(), 2);
        }
    }
}

TEST_F(Cms_NetStatCollectTest, netstat) {
    EXPECT_EQ(-1, cloudMonitor_netstat_collect(nullptr, nullptr));
    auto *handler = cloudMonitor_netstat_init("10");
    EXPECT_NE(nullptr, handler);
    defer(cloudMonitor_netstat_exit(handler));

    char *buf = nullptr;
    EXPECT_EQ(cloudMonitor_netstat_collect(handler, &buf) > 0, true);
    cout << "result:" << buf << endl;
}

TEST_F(Cms_NetStatCollectTest, countSimpleTcpState) {
    EXPECT_EQ(countSimpleTcpState(), 4);
}
