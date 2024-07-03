#include <gtest/gtest.h>
#include "cloudMonitor/detect_collect.h"

#include <thread>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/ExceptionsHandler.h"
#include "common/ModuleTool.h"
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "core/TaskManager.h"
#include "detect/detect_schedule.h"

#include "cloudMonitor/cloud_monitor_config.h"
#include "cloudMonitor/cloud_module_macros.h"

using namespace std;
using namespace std::chrono;
using namespace common;
using namespace cloudMonitor;
using namespace argus;

class Cms_DetectCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
        Delete(pShared);

        SingletonTaskManager::destroyInstance();
        // SingletonCloudMonitorConfig::destroyInstance();
    }

    DetectCollect *pShared = nullptr;
};

DECLARE_CMS_EXTERN_MODULE(detect);

TEST_F(Cms_DetectCollectTest, HttpCollect)
{
    safeRun([&]{
        TaskManager *pTask = SingletonTaskManager::Instance();

        unique_ptr<DetectCollect> pShared(new DetectCollect());
        pShared->Init();
        //
        // std::this_thread::sleep_for(seconds{1});

        std::unordered_map<std::string, PingItem> pingItems;
        //set ping
        pingItems["ping001"] = PingItem("ping001", "www.baidu.com", std::chrono::seconds{1});
        pingItems["ping002"] = PingItem("ping002", "www.taobao.com", std::chrono::seconds{1});
        pingItems["ping003"] = PingItem("ping003", "10.137.71.5", std::chrono::seconds{1});
        pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
        //set telnet
        std::unordered_map<std::string, TelnetItem> telnetItems;
        telnetItems["telnet001"] = TelnetItem("telnet001", "www.baidu.com:80", seconds{1});
        telnetItems["telnet002"] = TelnetItem("telnet002", "www.taobao.com:80", seconds{1});
        telnetItems["telnet003"] = TelnetItem("telnet003", "10.137.71.5:80", seconds{1});
        pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));
        //set http
        std::unordered_map<std::string, HttpItem> httpItems;
        httpItems["http001"] = HttpItem("http001", "http://www.baidu.com", seconds{1}, seconds{1});
        httpItems["http002"] = HttpItem("http002", "http://www.taobao.com", seconds{1}, seconds{1});
        httpItems["http003"] = HttpItem("http003", "http://10.137.71.5", seconds{1}, seconds{1});
        pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>(httpItems));

        std::this_thread::sleep_for(microseconds{1000 * 1000 * 3 + 1000});
        string collectStr;
        CollectData collectData;
        EXPECT_GT(pShared->Collect(collectStr), 0);
        ModuleData::convertStringToCollectData(collectStr, collectData);
        EXPECT_EQ(collectData.moduleName, pShared->mModuleName);
        collectData.print(std::cout);

        //set ping
        pingItems["ping0001"] = PingItem("ping0001", "www.baidu.com", std::chrono::seconds{1});
        pingItems["ping0002"] = PingItem("ping0002", "www.taobao.com", std::chrono::seconds{1});
        pingItems["ping0003"] = PingItem("ping0003", "10.137.71.5", std::chrono::seconds{1});
        pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
        //set telnet
        telnetItems["telnet0001"] = TelnetItem("telnet0001", "www.baidu.com:80", seconds{1});
        telnetItems["telnet0002"] = TelnetItem("telnet0002", "www.taobao.com:80", seconds{1});
        telnetItems["telnet0003"] = TelnetItem("telnet0003", "10.137.71.5:80", seconds{1});
        pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));
        //set http
        httpItems["http0001"] = HttpItem("http0001", "http://www.baidu.com", seconds{1}, seconds{1});
        httpItems["http0002"] = HttpItem("http0002", "http://www.taobao.com", seconds{1}, seconds{1});
        httpItems["http0003"] = HttpItem("http0003", "http://10.137.71.5", seconds{1}, seconds{1});
        pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>(httpItems));

        // std::this_thread::sleep_for(microseconds{1000 * 1000 * 15 + 1000});
        std::this_thread::sleep_for(microseconds{1000 * 1000 * 2 + 1000});
        EXPECT_EQ(pShared->Collect(collectStr) > 0, true);
        ModuleData::convertStringToCollectData(collectStr, collectData);
        EXPECT_EQ(collectData.moduleName, pShared->mModuleName);
        collectData.print();

        pingItems.clear();
        telnetItems.clear();
        httpItems.clear();

        LogInfo("----- clear ------------------------------------------------");
        pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>());
        pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>());
        pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>());
        while(pShared->mPDetectSchedule->TaskCount()) {
            std::this_thread::sleep_for(milliseconds{500});
        }
        std::this_thread::sleep_for(seconds{2});

        // pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
        // pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));
        // pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>(httpItems));
        // std::this_thread::sleep_for(seconds{2});

        EXPECT_EQ(pShared->Collect(collectStr), 0);
    });
}
TEST_F(Cms_DetectCollectTest, detect)
{
    // std::vector<char> buf(102400);
    // int bufLen = 102400;

    std::unordered_map<std::string, PingItem> pingItems;
    TaskManager *pTask = SingletonTaskManager::Instance();
    //set ping
    pingItems["ping001"] = PingItem("ping001", "www.baidu.com", std::chrono::seconds{1});
    pingItems["ping002"] = PingItem("ping002", "www.taobao.com", std::chrono::seconds{1});
    pingItems["ping003"] = PingItem("ping003", "10.137.71.5", std::chrono::seconds{1});
    pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
    //set telnet
    std::unordered_map<std::string, TelnetItem> telnetItems;
    telnetItems["telnet001"] = TelnetItem("telnet001", "www.baidu.com:80", seconds{1});
    telnetItems["telnet002"] = TelnetItem("telnet002", "www.taobao.com:80", seconds{1});
    telnetItems["telnet003"] = TelnetItem("telnet003", "10.137.71.5:80", seconds{1});
    pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));
    //set http
    std::unordered_map<std::string, HttpItem> httpItems;
    httpItems["http001"] = HttpItem("http001", "http://www.baidu.com", seconds{1}, seconds{1});
    httpItems["http002"] = HttpItem("http002", "http://www.taobao.com", seconds{1}, seconds{1});
    httpItems["http003"] = HttpItem("http003", "http://10.137.71.5", seconds{1}, seconds{1});
    pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>(httpItems));
    EXPECT_EQ(-1, cloudMonitor_detect_collect(nullptr, nullptr));
    auto *handler = cloudMonitor_detect_init(nullptr);
    EXPECT_NE(handler, nullptr);
    defer(cloudMonitor_detect_exit(handler));
    // EXPECT_EQ(0, cloudMonitor_detect_init(""));
    std::this_thread::sleep_for(microseconds{1000 * 1500});

    {
        char *collectStr = nullptr;
        EXPECT_GT(cloudMonitor_detect_collect(handler, &collectStr), 0);

        CollectData collectData;
        ModuleData::convertStringToCollectData(collectStr, collectData);
        EXPECT_EQ(collectData.moduleName, "detect");
        collectData.print();
        // EXPECT_EQ(0, cloudMonitor_detect_exit());
    }
}

TEST_F(Cms_DetectCollectTest, InitFail) {
    auto *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    {
        DetectCollect collector;
        EXPECT_FALSE(collector.mPDetectSchedule);
        std::string data;
        EXPECT_EQ(-1, collector.Collect(data)); // 未初始化
    }

    SingletonConfig::Instance()->Set("agent.enable.detect.module", "0");
    {
        DetectCollect collector;
        EXPECT_EQ(-1, collector.Init());
    }

    SingletonConfig::Instance()->Set("agent.enable.detect.module", "true");
    SingletonConfig::Instance()->Set("cms.detect.thread", "200");
    SingletonConfig::Instance()->Set("cms.detect.max.thread", "20");
    {
        DetectCollect collector;
        EXPECT_EQ(0, collector.Init());
        EXPECT_EQ(size_t(20), collector.mPDetectSchedule->mThreadPool->_maxThreadCount);
#ifdef ENABLE_CLOUD_MONITOR
        constexpr const size_t defaultMinThread = 1;
#else
        constexpr const size_t defaultMinThread = 10;
#endif
        EXPECT_EQ(defaultMinThread, collector.mPDetectSchedule->mThreadPool->_initSize);
    }
}