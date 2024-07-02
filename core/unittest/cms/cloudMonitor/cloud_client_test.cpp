#include <gtest/gtest.h>
#include "cloudMonitor/cloud_client.h"
#include "common/Logger.h"

#include <vector>
#include <thread>
#include <boost/json.hpp>
#include <boost/format.hpp>

#include "common/Config.h"
#include "common/FileUtils.h"
#include "common/ExceptionsHandler.h"
#include "common/JsonValueEx.h"
#include "common/UnitTestEnv.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/Chrono.h"

#if defined(__linux__) || defined(__unix__) || defined(WIN32)
#   include "common/SyncQueue.h"
#endif

#include "cloudMonitor/cloud_monitor_config.h"

#include "core/TaskManager.h"
#include "common/Uuid.h"
#include "sic/system_information_collector_util.h"
#include "common/ThreadUtils.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace argus;

const char *PREPUB_CLOUD_MONITOR = "http://cms-cloudmonitor.staging.aliyun-inc.com";

class Cms_CloudClientTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
        SingletonTaskManager::destroyInstance();
    }
};

TEST_F(Cms_CloudClientTest, Start) {
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    defer(SingletonConfig::Instance()->setBaseDir(oldBase));

    SingletonConfig::Instance()->setBaseDir((TEST_CONF_PATH / "conf/cloudMonitor/").string());
    CloudClient pClient;
    pClient.Start();
    this_thread::sleep_for(chrono::milliseconds{100});
}

TEST_F(Cms_CloudClientTest, DealHeartBeatResponse) {
    string fileContent;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf/cloudMonitor/heartResponse.json").string(), fileContent);
    auto *pClient = new CloudClient();
    defer(delete pClient);
    pClient->DealHeartBeatResponse(fileContent);
    NodeItem nodeItem;
    SingletonTaskManager::Instance()->GetNodeItem(nodeItem);
    EXPECT_EQ(nodeItem.instanceId, "host-3n3zLcA3Qms");
    EXPECT_EQ(nodeItem.serialNumber, "6303c38e-f243-4262-96f2-907f2fc7962c");
    EXPECT_EQ(nodeItem.aliUid, "1052053700687882");
    EXPECT_EQ(nodeItem.hostName, "staragent-fenghua-coding");
    EXPECT_EQ(nodeItem.region, "unknown");
    EXPECT_EQ(nodeItem.operatingSystem, "Linux");
    auto ptr = SingletonTaskManager::Instance()->ProcessCollectItems().GetCopy();
    auto &processCollectItems = *ptr;
    ASSERT_EQ(processCollectItems.size(), size_t(2));
    EXPECT_EQ(processCollectItems[0].name, "argusagent");
    EXPECT_EQ(processCollectItems[1].name, "argusagent_ut");
    vector<MetricItem> metricItems = *SingletonTaskManager::Instance()->MetricItems().Get();
    ASSERT_EQ(metricItems.size(), size_t(2));
    EXPECT_EQ(metricItems[0].url, "https://metrichub-cms-cn-hangzhou.aliyuncs.com/agent/metrics/putLines");
    EXPECT_EQ(metricItems[0].gzip, false);
    EXPECT_EQ(metricItems[0].useProxy, false);
    EXPECT_EQ(metricItems[1].url, "https://cms-cloudmonitor.aliyun.com/agent/metrics/putLines");
    EXPECT_EQ(metricItems[1].gzip, false);
    EXPECT_EQ(metricItems[1].useProxy, true);
    vector<HttpItem> httpItems = SingletonTaskManager::Instance()->HttpItems().Values();
    EXPECT_EQ(httpItems.size(), size_t(5));
    for (auto &httpItem: httpItems) {
        if (httpItem.taskId == "536177") {
            EXPECT_EQ(httpItem.timeout, std::chrono::seconds{10});
        }
        if (httpItem.taskId == "559958") {
            EXPECT_EQ(httpItem.timeout, std::chrono::seconds{10});
        }

        if (httpItem.taskId == "559862") {
            EXPECT_EQ(httpItem.uri, "http://www.baidu.com");
            EXPECT_EQ(httpItem.method, "POST");
            EXPECT_TRUE(httpItem.negative);
            EXPECT_EQ(httpItem.keyword, "bad");
            EXPECT_EQ(httpItem.header, "UTF-8");
            EXPECT_EQ(httpItem.requestBody, "helloworld");
        }

        if (httpItem.taskId == "449815") {
            EXPECT_EQ(httpItem.uri,
                      "http://127.0.0.1/cms/v1.0/alikey?app_version=7.9.1&installation_id=1810221750133318&timestamp=1540458693&type=1&app_key=xdThhy2239daax&os_type=1&channel=AliClound&sign=149BE6B9197E6A28B8FCEE845ECBC9D668C87792");
        }
        if (httpItem.taskId == "465119") {
            EXPECT_EQ(httpItem.uri,
                      "http://127.0.0.1/cms/v1.0/ad/config?installation_id=1810221750133318&app_version=7.8.7&sign=149BE6B9197E6A28B8FCEE845ECBC9D668C87792&os_type=1&device_id=5ea25c6b3296505&bssid=d0%3A76%3Ae7%3A98%3A9d%3A4b&app_key=xdThhy2239daax&gcid=6ddb322c0fa84e8c6602b635798dfca2&mac_address=02%3A00%3A00%3A00%3A00%3A00&imei=866479020121793&timestamp=1540458693&channel=AliClound&os_version=6.0");
        }
    }
    vector<TelnetItem> telnetItems = SingletonTaskManager::Instance()->TelnetItems().Values();
    ASSERT_EQ(telnetItems.size(), size_t(1));
    EXPECT_EQ(telnetItems[0].taskId, "536176");
    EXPECT_EQ(telnetItems[0].uri, "telnet://10.137.71.2:22");
    std::vector<PingItem> pingItems = SingletonTaskManager::Instance()->PingItems().Values();
    ASSERT_EQ(pingItems.size(), size_t(1));
    EXPECT_EQ(pingItems[0].taskId, "536175");
    EXPECT_EQ(pingItems[0].host, "10.137.71.2");
    HpcClusterItem hpcClusterItem;
    SingletonTaskManager::Instance()->GetHpcClusterItem(hpcClusterItem);
    EXPECT_EQ(hpcClusterItem.clusterId, "hpc-uf671uizyvln4qc9qudt");
    EXPECT_EQ(hpcClusterItem.regionId, "cn-shanghai");
    EXPECT_EQ(hpcClusterItem.version, "1F2F5ECFD8AF2810F4F36112C9E2C0BC");
    EXPECT_TRUE(hpcClusterItem.isValid);
    EXPECT_EQ(hpcClusterItem.hpcNodeInstances.size(), size_t(1));
    if (hpcClusterItem.hpcNodeInstances.size() == size_t(1)) {
        EXPECT_EQ(hpcClusterItem.hpcNodeInstances[0].instanceId, "i-uf6ef1y46xhgd77hgt5u");
        EXPECT_EQ(hpcClusterItem.hpcNodeInstances[0].ip, "200.0.134.2");
    }
}

TEST_F(Cms_CloudClientTest, GetPingItemsTest) {
    auto pingItems = std::make_shared<std::unordered_map<std::string, PingItem>>();
    TaskManager *pTask = SingletonTaskManager::Instance();
    //set ping
    (*pingItems)["ping001"] = PingItem("ping001", "www.baidu.com", std::chrono::seconds{10});
    (*pingItems)["ping002"] = PingItem("ping002", "www.taobao.com", std::chrono::seconds{10});
    (*pingItems)["ping003"] = PingItem("ping003", "10.137.71.5", std::chrono::seconds{10});
    pTask->SetPingItems(pingItems);
    //set telnet
    std::unordered_map<std::string, TelnetItem> telnetItems;
    telnetItems["telnet001"] = TelnetItem("telnet001", "www.baidu.com:80", std::chrono::seconds{10});
    telnetItems["telnet002"] = TelnetItem("telnet002", "www.taobao.com:80", std::chrono::seconds{10});
    telnetItems["telnet003"] = TelnetItem("telnet003", "10.137.71.5:80", std::chrono::seconds{10});
    pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));
    //set http
    std::unordered_map<std::string, HttpItem> httpItems;
    httpItems["http001"] = HttpItem("http001", "http://www.baidu.com", std::chrono::seconds{10});
    httpItems["http002"] = HttpItem("http002", "http://www.taobao.com", std::chrono::seconds{10});
    httpItems["http003"] = HttpItem("http003", "http://10.137.71.5", std::chrono::seconds{10});
    pTask->SetHttpItems(std::make_shared<std::unordered_map<std::string, HttpItem>>(httpItems));

    auto now = std::chrono::steady_clock::now(); // Now<ByMicros>();
    std::vector<PingItem> localPingItems = {
            PingItem("ping001", "www.baidu.com", std::chrono::seconds{10}),
            PingItem("ping002", "www.taobao.com", std::chrono::seconds{10}),
            PingItem("ping004", "10.137.71.5", std::chrono::seconds{10}),
    };
    localPingItems[0].lastScheduleTime = now;
    localPingItems[1].lastScheduleTime = now;
    std::vector<TelnetItem> localTelnetItem = {
            TelnetItem("telnet001", "www.baidu.com:80", std::chrono::seconds{10}),
            TelnetItem("telnet002", "www.taobao.com:80", std::chrono::seconds{10}),
            TelnetItem("telnet004", "10.137.71.5:80", std::chrono::seconds{10}),
    };
    localTelnetItem[0].lastScheduleTime = now;
    localTelnetItem[1].lastScheduleTime = now;
    std::vector<HttpItem> localHttpItem = {
            HttpItem("http001", "http://www.baidu.com", std::chrono::seconds{10}),
            HttpItem("http002", "http://www.taobao.com", std::chrono::seconds{10}),
            HttpItem("http004", "http://10.137.71.5", std::chrono::seconds{10}),
    };
    localHttpItem[0].lastScheduleTime = now;
    localHttpItem[1].lastScheduleTime = now;

    localPingItems = pTask->PingItems().Values();
    localTelnetItem = pTask->TelnetItems().Values();
    localHttpItem = pTask->HttpItems().Values();
    int count = 0;
    for (auto &pingItem: localPingItems) {
        if (pingItem.taskId == "ping001") {
            ++count;
            EXPECT_TRUE(IsZero(pingItem.lastScheduleTime));
        }
        if (pingItem.taskId == "ping002") {
            ++count;
            EXPECT_TRUE(IsZero(pingItem.lastScheduleTime));
        }
        if (pingItem.taskId == "ping003") {
            ++count;
            EXPECT_TRUE(IsZero(pingItem.lastScheduleTime));
        }
    }
    EXPECT_EQ(count, 3);
    count = 0;
    for (auto &telnetItem: localTelnetItem) {
        if (telnetItem.taskId == "telnet001") {
            ++count;
            EXPECT_TRUE(IsZero(telnetItem.lastScheduleTime));
        }
        if (telnetItem.taskId == "telnet002") {
            ++count;
            EXPECT_TRUE(IsZero(telnetItem.lastScheduleTime));
        }
        if (telnetItem.taskId == "telnet003") {
            ++count;
            EXPECT_TRUE(IsZero(telnetItem.lastScheduleTime));
        }
    }
    EXPECT_EQ(count, 3);
    count = 0;
    for (auto &httpItem: localHttpItem) {
        if (httpItem.taskId == "http001") {
            ++count;
            EXPECT_TRUE(IsZero(httpItem.lastScheduleTime));
        } else if (httpItem.taskId == "http002") {
            ++count;
            EXPECT_TRUE(IsZero(httpItem.lastScheduleTime));
        } else if (httpItem.taskId == "http003") {
            ++count;
            EXPECT_TRUE(IsZero(httpItem.lastScheduleTime));
        }
    }
    EXPECT_EQ(count, 3);
}

TEST_F(Cms_CloudClientTest, InitProxyManagerTest) {
    fs::path dir = TEST_CONF_PATH / "conf/cloudMonitor";
    {
        auto *old = SingletonConfig::swap(newCmsConfig((dir / "cloudMonitorWithHttpProxy.conf").string()));
        defer(delete SingletonConfig::swap(old));

        // CloudMonitorConfig *pConfig = SingletonCloudMonitorConfig::Instance();
        // http proxy
        // pConfig->Init((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithHttpProxy.conf").string());
        auto pHttpProxyClient = std::make_shared<CloudClient>();
        pHttpProxyClient->InitProxyManager();
        EXPECT_EQ(pHttpProxyClient->mCloudAgentInfo->proxyUrl, "http://127.0.0.1:3128");
        // delete pHttpProxyClient;
        // SingletonCloudMonitorConfig::destroyInstance();
    }

    {
        auto *old = SingletonConfig::swap(newCmsConfig((dir / "cloudMonitorWithSocks5Proxy.conf").string()));
        defer(delete SingletonConfig::swap(old));
        // // pConfig = SingletonCloudMonitorConfig::Instance();
        // auto pConfig = std::make_shared<Config>();
        // // http proxy
        // // pConfig->Init((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5Proxy.conf").string());
        // pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5Proxy.conf").string());
        auto pSocks5ProxyClient = std::make_shared<CloudClient>();
        pSocks5ProxyClient->InitProxyManager();
        EXPECT_EQ(pSocks5ProxyClient->mCloudAgentInfo->proxyUrl, "socks5://127.0.0.1:1080");
        // delete pSocks5ProxyClient;
        // SingletonCloudMonitorConfig::destroyInstance();
    }

    {
        auto *old = SingletonConfig::swap(newCmsConfig((dir / "cloudMonitorWithSocks5Proxy.conf").string()));
        defer(delete SingletonConfig::swap(old));
        // // pConfig = SingletonCloudMonitorConfig::Instance();
        // auto pConfig = std::make_shared<Config>();
        // // http proxy
        // // pConfig->Init((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5Proxy.conf").string());
        // pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5Proxy.conf").string());
        auto pMixedProxyClient = std::make_shared<CloudClient>();
        pMixedProxyClient->InitProxyManager();
        EXPECT_EQ(pMixedProxyClient->mCloudAgentInfo->proxyUrl, "socks5://127.0.0.1:1080");
        // delete pMixedProxyClient;
        // SingletonCloudMonitorConfig::destroyInstance();
    }
}

TEST_F(Cms_CloudClientTest, ParseFileStoreInfo) {
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH / "conf/cloudMonitor");

#ifdef WIN32
    fs::path fileTestStore = (TEST_CONF_PATH / "tmp" / "testStore");
#else
    fs::path fileTestStore = "/tmp/testStore";
#endif
    const char *jsonTpl = R"({"fileStore": [{"filePath":%1%,"content":"Iy9iaW4vYmFzaAplY2hvICJoZWxsbyB3b3JsZCI=","user":"yunjian.ph"}]})";
    string jsonStr = (boost::format(jsonTpl) %
                      boost::json::serialize(boost::json::string{fileTestStore.string()})).str();
    std::cout << jsonStr << std::endl;
    json::Value jsonValue = json::parse(jsonStr);
    EXPECT_FALSE(jsonValue.isNull());
    if (!jsonValue.isNull()) {
        cloudMonitor::CloudClient::ParseFileStoreInfo(jsonValue.asObject());
        string fileContent;
        EXPECT_EQ(FileUtils::ReadFileContent(fileTestStore.string(), fileContent), 29);
        EXPECT_EQ(fileContent, "#/bin/bash\necho \"hello world\"");
    }

    string jsonStr1 = R"({"fileStore": [{"filePath":"tianjimon-script/myapp/myscript.sh","content":"YWZ0ZXIgZWRpdAohQCMkJV4mKigpXys=="}]})";
    json::Object jsonValue1 = json::parseObject(jsonStr1);
    EXPECT_FALSE(jsonValue1.isNull());
    if (!jsonValue1.isNull()) {
        cloudMonitor::CloudClient::ParseFileStoreInfo(jsonValue1);
        string fileContent;
        std::string targetPath = (TEST_CONF_PATH / "conf/cloudMonitor/tianjimon-script/myapp/myscript.sh").string();
        EXPECT_EQ(FileUtils::ReadFileContent(targetPath, fileContent), 23);
        EXPECT_EQ(fileContent, "after edit\n!@#$%^&*()_+");
    }

    fs::remove(fileTestStore);
    EXPECT_FALSE(fs::exists(fileTestStore));

    fs::path path = TEST_CONF_PATH / "conf/cloudMonitor/tianjimon-script/myapp/myscript.sh";
    fs::remove(path);
    EXPECT_FALSE(fs::exists(path));

    SingletonConfig::Instance()->setBaseDir(oldBase);
}

namespace cloudMonitor {
    extern std::string makeCoreDumpJsonBody(const tagCoreDumpFile &dumpFile, const argus::CloudAgentInfo &agentInfo);
    extern std::string makeThreadDumpJsonBody(const std::vector<ResourceWaterLevel> &vecResources,
                                              const std::vector<ResourceConsumption> &topTasks,
#if !defined(WITHOUT_MINI_DUMP)
                                              const std::list<tagThreadStack> &lstThreadStack,
#endif
                                              const argus::CloudAgentInfo &agentInfo);
}

TEST_F(Cms_CloudClientTest, makeMiniDumpJsonBody) {
    tagCoreDumpFile dumpFile;
    dumpFile.dumpTime = "2023-05-31 11:50:31";
    dumpFile.filename = "argus-agent.SIG-USER.dump";
    dumpFile.callstackType = "Text";
    dumpFile.callstack = R"(00# __wrap___gxx_personality_v0 at ./src/common/gcc_hook.cpp:127
01# _Unwind_RaiseException in /lib64/libgcc_s.so.1
02# __cxa_throw in ./bin/argus-agent_ut)";

    argus::CloudAgentInfo agentInfo;
    agentInfo.serialNumber = NewUuid();
    std::string body = cloudMonitor::makeCoreDumpJsonBody(dumpFile, agentInfo);
    std::cout << body << std::endl;
    EXPECT_EQ(body[0], '{');
    EXPECT_EQ(*body.rbegin(), '}');
}

// 非这两种OS，未开启core dump
#if defined(__linux__) || defined(__unix__) || defined(WIN32)
TEST_F(Cms_CloudClientTest, SendCoreDump) {
    DumpCache dumpClear(0); // 清理所有dump文件
    // 生成CoreDump文件
    std::thread thread(safeRun, [&]() {
        SetupThread("DumpTest");
        throw std::out_of_range("ByDesign");
    });
    thread.join(); // 等待线程退出

#ifndef ONE_AGENT
    DumpCache dump(1, true);
    ASSERT_EQ(size_t(1), dump.DumpFiles().size());
    LogInfo("dumped: {}", dump.DumpFiles().at(0).string());
    auto lastCoreDown = dump.GetLastCoreDown(std::chrono::seconds{60 * 60});
    EXPECT_FALSE(lastCoreDown.filename.empty());

    CloudClient client;
    client.InitProxyManager();
    {
        CloudAgentInfo agentInfo;
        agentInfo.serialNumber = client.mCloudAgentInfo->serialNumber;
        client.mCloudAgentInfo = std::make_shared<CloudAgentInfo>(agentInfo);
        client.mCloudAgentInfo->HeartbeatUrl = PREPUB_CLOUD_MONITOR;
    }
    EXPECT_TRUE(client.SendCoreDump());
    {
        std::string response;
        if (client.SendHeartbeat(response)) {
            client.DealHeartBeatResponse(response);
        }
        EXPECT_TRUE(client.SendCoreDump());
    }
#endif
}

TEST_F(Cms_CloudClientTest, makeThreadDumpJsonBody) {
    SyncQueue<bool> enter;
    SyncQueue<bool> exit;

    std::thread thread(safeRun, [&](){
        SetupThread("thr-test");

        enter << true;
        exit.Wait();
    });
    enter.Wait();

#if !defined(WITHOUT_MINI_DUMP)
    std::list<tagThreadStack> lstStack;
    WalkThreadStack(lstStack);
    EXPECT_FALSE(lstStack.empty());
#endif

    std::vector<ResourceConsumption> topTasks;
    ResourceConsumption record;
    record.ThreadId = 1;
    record.TaskName = "hello";
    record.Millis = std::chrono::milliseconds{30};
    topTasks.push_back(record);

    std::vector<ResourceWaterLevel> resources;
    ResourceWaterLevel rwl;
    rwl.name = ">cpu<";
    rwl.value = 6;
    rwl.threshold = 5;
    resources.push_back(rwl);

    argus::CloudAgentInfo agentInfo;
    agentInfo.serialNumber = NewUuid();
    std::string body = makeThreadDumpJsonBody(resources, topTasks, 
#if !defined(WITHOUT_MINI_DUMP)
        lstStack, 
#endif
    agentInfo);
    LogInfo("body: {}", body);
    EXPECT_EQ(body[0], '{');
    EXPECT_EQ(*body.rbegin(), '}');
    EXPECT_NE(body.find(rwl.name), std::string::npos);

    exit << true;
    thread.join();

    CloudClient client;
    {
        client.InitProxyManager();

        CloudAgentInfo tmp;
        tmp.serialNumber = client.mCloudAgentInfo->serialNumber;
        client.mCloudAgentInfo = std::make_shared<CloudAgentInfo>(tmp);
    }
    client.mCloudAgentInfo->HeartbeatUrl = PREPUB_CLOUD_MONITOR;

    std::string instanceIds[] = {"hi-i1248", ""};
    for (const std::string &instanceId: instanceIds) {
        NodeItem nodeItem;
        SingletonTaskManager::Instance()->GetNodeItem(nodeItem);

        NodeItem newNodeItem = nodeItem;
        newNodeItem.instanceId = instanceId;
        newNodeItem.serialNumber = client.mCloudAgentInfo->serialNumber;
        SingletonTaskManager::Instance()->SetNodeItem(newNodeItem);
        defer(SingletonTaskManager::Instance()->SetNodeItem(nodeItem));

        bool ok = client.SendThreadsDump(resources, topTasks
#if !defined(WITHOUT_MINI_DUMP)
            , lstStack
#endif
        );
        Log((ok ? LEVEL_INFO : LEVEL_ERROR), "RESULT: {}", ok);
        EXPECT_TRUE(ok);
    }
}
#endif // defined(__linux__) || defined(__unix__) || defined(WIN32)

TEST_F(Cms_CloudClientTest, GetHeartbeatJsonString) {
    CloudClient client;
    client.InitProxyManager();

    std::string json;
    client.GetHeartbeatJsonString(json);
    LogInfo("{}", json);
}

TEST_F(Cms_CloudClientTest, SendHeartbeatFail) {
    CloudClient client;
    client.mCloudAgentInfo = std::make_shared<argus::CloudAgentInfo>();
    client.mCloudAgentInfo->HeartbeatUrl = "https://argus-test-metrichub.com";

    std::string resp;
    EXPECT_FALSE(client.SendHeartbeat("", resp));
}

#if 0
// Mock某个实例，去获取argus要关的任务
TEST_F(Cms_CloudClientTest, MockInstance) {
    CloudClient client;
    client.InitProxyManager();

    // "userId":"1240792566512190","instanceId":"i-wz975oy3sg0tz36278fv"  显著的爬坡
    // "userId":"35927884","instanceId":"i-2ze7889ommtwbfbcyqo5", 有迅速超200M的特性
    boost::json::object object{
            {"systemInfo",  {
                                    {"serialNumber", "9ea175c5-d68b-47fa-b850-0272f62941c8"},
                                    {"hostname", "vehicle-stream-processor-prod"},
                                    {"localIPs", {"47.107.108.242", "172.18.200.199"}},
                                    {"name", "CentOS  7.4 64位"},
                                    {"version", "7.4"},
                                    {"arch", "x86_64"},
                                    {"freeSpace", 18037356},
                            },
            },
            {"versionInfo", {
                                    {"version",      "3.5.10"},
                            },
            },
    };
    std::string json = boost::json::serialize(object);
    std::cout << json << std::endl;

    std::string response;
    EXPECT_TRUE(client.SendHeartbeat(json, response));
    std::cout << response << std::endl;
}
#endif
