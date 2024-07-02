#include <gtest/gtest.h>
#include <iostream>
#include "cloudMonitor/memory_collect.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/Defer.h"

#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

DECLARE_CMS_EXTERN_MODULE(memory);

class CmsMemoryCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
        p_shared = new MemoryCollect();
    }

    void TearDown() override {
        delete p_shared;
        p_shared = nullptr;
    }

    MemoryCollect *p_shared = nullptr;
};

TEST_F(CmsMemoryCollectTest, Collect) {
    string data;
    int collectLen = p_shared->Collect(data);
    EXPECT_EQ(collectLen > 0, true);
    EXPECT_EQ(collectLen, int(data.size()));
    cout << "collectResult:" << data << endl;
    CollectData collectData;
    ModuleData::convertStringToCollectData(data, collectData);
    EXPECT_EQ(collectData.moduleName, p_shared->mModuleName);
#ifdef ENABLE_MEM_SWAP
    size_t count = 3;
#else
    size_t count = 2;
#endif
    EXPECT_EQ(collectData.dataVector.size(), count);
    int index = 0;
    EXPECT_EQ(collectData.dataVector[index++].valueMap.size(), size_t(9));
#ifdef ENABLE_MEM_SWAP
    EXPECT_EQ(collectData.dataVector[index++].valueMap.size(), size_t(7));
#endif
    EXPECT_EQ(collectData.dataVector[index++].valueMap.size(), size_t(1));
}

TEST_F(CmsMemoryCollectTest, memory) {
    EXPECT_EQ(-1, cloudMonitor_memory_collect(nullptr, nullptr));
    auto *handler = cloudMonitor_memory_init("10");
    defer(cloudMonitor_memory_exit(handler));
    EXPECT_NE(nullptr, cloudMonitor_memory_init("10"));
    char *buf = nullptr;
    EXPECT_GT(cloudMonitor_memory_collect(handler, &buf), 0);
    cout << "result:" << buf << endl;
}
