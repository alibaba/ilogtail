#include <gtest/gtest.h>
#include "cloudMonitor/net_collect.h"

#include <iostream>
#include <thread>

#include "common/Config.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/ChronoLiteral.h"
#include "common/Defer.h"

#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace std::chrono;

DECLARE_CMS_EXTERN_MODULE(net);

class CmsNetCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
        p_shared = new NetCollect();
        p_shared->Init(15);
    }

    void TearDown() override {
        delete p_shared;
        p_shared = nullptr;
    }

    NetCollect *p_shared;
};

TEST_F(CmsNetCollectTest, Collect) {
    string collectStr;

    p_shared->mTotalCount = 2;
    for (int i = 0; i <= p_shared->mTotalCount; i++) {
        //int64_t start = NowMicros();
        //int64_t sum = 0;
        //for (int64_t j = 0; j < start / 10000000; j++) {
        //    sum += j * start;
        //}
        //int64_t end = NowMicros();
        // cout << "time:" << end - start << endl;
        std::this_thread::sleep_for(1_s); // -(end - start)});
        int collectLen = p_shared->Collect(collectStr);
        if (i < p_shared->mTotalCount) {
            EXPECT_EQ(collectLen, 0);
        } else {
            EXPECT_EQ(collectLen > 0, true);
            cout << "collectLen= " << collectLen << endl;
            cout << "collectResult= " << collectStr << endl;
            CollectData collectData;
            ModuleData::convertStringToCollectData(collectStr, collectData);
            EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
            //EXPECT_EQ(collectData.dataVector.size(),p_shared->mInterfaceConfigMap.size()*3);
        }
    }
}

TEST_F(CmsNetCollectTest, net) {
    EXPECT_EQ(-1, cloudMonitor_net_collect(nullptr, nullptr));

    auto *handler = cloudMonitor_net_init("10");
    defer(cloudMonitor_net_exit(handler));
    EXPECT_NE(nullptr, handler);

    for (int i = 0; i < 10; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        EXPECT_EQ(0, cloudMonitor_net_collect(handler, nullptr));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
    {
        char *buf = nullptr;
        EXPECT_GT(cloudMonitor_net_collect(handler, &buf), 0);
        cout << "result:" << buf << endl;
    }
    for (int t = 1; t < 5; t++) {
        for (int i = 0; i < 9; i++) {
            std::this_thread::sleep_for(std::chrono::milliseconds{50});
            EXPECT_EQ(0, cloudMonitor_net_collect(handler, nullptr));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{50});
        {
            char *buf = nullptr;
            EXPECT_GT(cloudMonitor_net_collect(handler, &buf), 0);
            cout << "result:" << buf << endl;
        }
    }
}
