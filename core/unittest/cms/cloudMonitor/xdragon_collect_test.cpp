#include <gtest/gtest.h>
#include "cloudMonitor/xdragon_collect.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/UnitTestEnv.h"

#include "cloudMonitor/cloud_monitor_config.h"
#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

DECLARE_CMS_EXTERN_MODULE(xdragon);

class Cms_XDragonCollectTest : public testing::Test {
protected:
    void SetUp() override {
        pShared = new XDragonCollect();
    }

    void TearDown() override {
        delete pShared;
        pShared = nullptr;
    }

    XDragonCollect *pShared = nullptr;
};

#ifndef WIN32

TEST_F(Cms_XDragonCollectTest, CheckEvn) {
    pShared->mIniFile = (TEST_CONF_PATH / "conf/cloudMonitor/xdragon/xdragon.ini").string();
    EXPECT_TRUE(pShared->CheckEvn());
    EXPECT_EQ(size_t(10), pShared->mTotalCount);
    EXPECT_EQ("/bin/echo", pShared->mBinFile);
}

TEST_F(Cms_XDragonCollectTest, Collect) {
    pShared->mIniFile = (TEST_CONF_PATH / "conf/cloudMonitor/xdragon/xdragon.ini").string();
    EXPECT_EQ(0, pShared->Init());
    EXPECT_EQ(decltype(pShared->mTotalCount)(10), pShared->mTotalCount);
    pShared->mBinFile = "/bin/echo helloworld";
    for (int i = 1; i <= 100; i++) {
        string data;
        int ret = pShared->Collect(data);
        if (i % 10 == 0) {
            EXPECT_EQ(ret > 0, true);
            CollectData collectData;
            ModuleData::convertStringToCollectData(data, collectData);
            EXPECT_EQ(collectData.moduleName, pShared->mModuleName);
            collectData.print();
        } else {
            EXPECT_EQ(ret, 0);
        }
    }
}

TEST_F(Cms_XDragonCollectTest, GetCollectResult) {
    XDragonCollect &collector = *pShared;
    std::string result;

    collector.mBinFile = "";
    EXPECT_FALSE(collector.GetCollectResult(result));
    EXPECT_TRUE(result.empty());

    collector.mBinFile = "/bin/ls";
    EXPECT_TRUE(collector.GetCollectResult(result));
    EXPECT_FALSE(result.empty());
}

#endif

TEST_F(Cms_XDragonCollectTest, xdragon) {
    EXPECT_EQ(-1, cloudMonitor_xdragon_collect(nullptr, nullptr));
    EXPECT_EQ(nullptr, cloudMonitor_xdragon_init(nullptr));
    cloudMonitor_xdragon_exit(nullptr); // 不抛异常即为OK
}
