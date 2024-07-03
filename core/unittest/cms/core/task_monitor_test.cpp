#include  <gtest/gtest.h>
#include "core/task_monitor.h"
#include "core/argus_manager.h"
#include "common/Logger.h"
#include "common/Config.h"
#include "common/SystemUtil.h"
#include "common/FileUtils.h"
#include "common/StringUtils.h"
#include "common/Md5.h"
#include "common/UnitTestEnv.h"
#include "common/Chrono.h"
#include "common/ChronoLiteral.h"

using namespace common;
using namespace argus;
using namespace std;

class Core_TaskMonitorTest : public testing::Test {
protected:
    TaskMonitor *pShared = nullptr;

    void SetUp() override {
        Config *cfg = SingletonConfig::Instance();
        string taskFile = (TEST_CONF_PATH / "conf" / "task" / "moduleTask.json").string();
        cfg->setModuleTaskFile(taskFile);
        taskFile = (TEST_CONF_PATH / "conf" / "task" / "scriptTask.json").string();
        cfg->setScriptTaskFile(taskFile);
        taskFile = (TEST_CONF_PATH / "conf" / "task" / "receiveTask.json").string();
        cfg->setReceiveTaskFile(taskFile);
        pShared = new TaskMonitor();
    }

    void TearDown() override {
        delete pShared;
        pShared = nullptr;
        SingletonTaskManager::destroyInstance();
    }
};

TEST_F(Core_TaskMonitorTest, LoadModuleTaskFromFile) {
    TaskManager tm;
    ArgusManager am(&tm);
    auto *old = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(old));

    LogInfo("ModuleTask: {}", pShared->mModuleTaskFile);
    EXPECT_EQ(2, pShared->LoadModuleTaskFromFile());
    // map<string,ModuleItem> moduleCollectItemMap;
    TaskManager *pM = SingletonTaskManager::Instance();
    std::shared_ptr<std::map<std::string, ModuleItem>> moduleCollectItemMap = pM->ModuleItems();
    EXPECT_EQ(size_t(2), moduleCollectItemMap->size());
    for (const auto &it: *moduleCollectItemMap) {
        string mid = it.first;
        ModuleItem item;
        if (mid.find("cpu") != string::npos) {
            item = (*moduleCollectItemMap)[mid];
            EXPECT_EQ(item.scheduleExpr, "[* * * * * ?]");
            EXPECT_EQ(item.scheduleInterval.count(), 10000);
            EXPECT_EQ(item.outputVector.size(), size_t(2));
            EXPECT_EQ(item.moduleArgs, "test para");
            EXPECT_EQ(item.name, "cpu");
            EXPECT_EQ(item.module, "cpu");
            EXPECT_TRUE(item.isSoType);
        } else if (mid.find("memory") != string::npos) {
            item = (*moduleCollectItemMap)[mid];
            EXPECT_EQ(item.scheduleInterval.count(), 20000);
            EXPECT_EQ(item.scheduleExpr, "[* * * * * ?]");
            EXPECT_EQ(item.outputVector.size(), size_t(2));
            EXPECT_EQ(item.moduleArgs, "");
            EXPECT_EQ(item.name, "memory_sec");
            EXPECT_EQ(item.module, "memory");
            EXPECT_FALSE(item.isSoType);
            EXPECT_EQ(0, pShared->LoadModuleTaskFromFile());
        }
    }
}

TEST_F(Core_TaskMonitorTest, LoadScriptTaskFromFile) {
    LogInfo("ScriptTaskFile: {}", pShared->mScriptTaskFile);

    std::cout << "-------------------------------------------------------------------------------" << std::endl;
    EXPECT_EQ(3, pShared->LoadScriptTaskFromFile());
    TaskManager *pM = SingletonTaskManager::Instance();
    auto moduleScriptItemMap = pM->ScriptItems().Get();
    EXPECT_EQ(size_t(3), moduleScriptItemMap->size());
    for (const auto &it: *moduleScriptItemMap) {
        cout << it.first << endl;
    }

    string mid;

    mid = "staragent_1.0_788826c2b2ea9891845a3c52ac8c3eef";
    EXPECT_NE(moduleScriptItemMap->find(mid), moduleScriptItemMap->end());
    ScriptItem item = (*moduleScriptItemMap)[mid];
    EXPECT_FALSE(item.isAliScript);
    EXPECT_EQ(item.scheduleIntv, 2_min);
    EXPECT_EQ(item.timeOut, 20_s);
    EXPECT_EQ(item.outputVector.size(), size_t(1));
    EXPECT_EQ(ToSeconds(item.firstSche), 1000);
    EXPECT_TRUE(item.reportStatus);
    EXPECT_EQ(item.duration, 6_min); // std::chrono::seconds{360});
    EXPECT_EQ(item.scheduleExpr, "[* * * * * ?]");
    EXPECT_EQ(item.scriptUser, "root");
    EXPECT_EQ(item.scriptUser, "root");
    EXPECT_EQ(item.resultFormat, PROMETHEUS_FORMAT);
    EXPECT_EQ(item.labelAddInfos.size(), size_t(1));

    mid = "argusagent_1.0_1bef6717483c7d6b5ee958bafbcab9c4";
    EXPECT_TRUE(moduleScriptItemMap->find(mid) != moduleScriptItemMap->end());
    item = (*moduleScriptItemMap)[mid];
    EXPECT_EQ(item.scheduleIntv, 100_s);
    EXPECT_EQ(item.timeOut, 20_s);
    EXPECT_EQ(item.outputVector.size(), size_t(1));
    EXPECT_EQ(item.firstSche.time_since_epoch().count(), 0);
    EXPECT_FALSE(item.reportStatus);
    EXPECT_EQ(item.duration, std::chrono::seconds::zero());
    EXPECT_EQ(item.scheduleExpr, "[* * * * * ?]");
    EXPECT_EQ(item.resultFormat, RAW_FORMAT);
    EXPECT_EQ(item.labelAddInfos.size(), size_t(0));
    EXPECT_EQ(0, pShared->LoadScriptTaskFromFile());

    mid = "staragent_invalid_1.0_c1de9ee15dbdc6f0c58a24fabf9c6cc0";
    EXPECT_TRUE(moduleScriptItemMap->find(mid) != moduleScriptItemMap->end());
    item = (*moduleScriptItemMap)[mid];
    EXPECT_EQ(item.scheduleIntv, 10_s);
    EXPECT_EQ(item.timeOut, 10_s);
}

TEST_F(Core_TaskMonitorTest, LoadAliScriptTaskFromFile) {
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir((TEST_CONF_PATH / "conf").string());
    unique_ptr<TaskMonitor> pMonitor(new TaskMonitor());
    EXPECT_EQ(pMonitor->mAliScriptTaskFile,
              (TEST_CONF_PATH / "conf" / "local_data" / "conf" / "aliScriptTask.json").string());
    pMonitor->mAliScriptTaskFile = (TEST_CONF_PATH / "conf" / "task" / "aliScriptTask.json").string();
    // map<string,ScriptItem> ScriptItemMap;
    std::shared_ptr<map<string, ScriptItem>> prevAliScriptItems;
    TaskManager *pM = SingletonTaskManager::Instance();
    EXPECT_EQ(1, pMonitor->LoadAliScriptTaskFromFile());
    EXPECT_TRUE(pM->AliScriptItems().Get(prevAliScriptItems));
    map<string, ScriptItem> ScriptItemMap = *pM->AliScriptItems().Get();
    EXPECT_FALSE(pM->AliScriptItems().Get(prevAliScriptItems));
    ASSERT_EQ(size_t(1), ScriptItemMap.size());
    auto it = ScriptItemMap.begin();
    // const char *szMid = "ut-script-1_1624254278518_AliScript_9d029999345e714ef9be2f4fc1862f74";
    const char *szExpectMid = "ut-script-1_1624254278518_AliScript_1e82350f64421320435376fe354445c2";
    EXPECT_EQ(it->first, szExpectMid);
    ScriptItem item = it->second;
    EXPECT_TRUE(item.isAliScript);
    EXPECT_EQ(item.mid, szExpectMid);
    EXPECT_EQ(item.name, "ut-script-1");
    EXPECT_EQ(item.collectUrl,
              "/home/staragent/plugins/Argus/local_data/libexec/alimonitor/check_xgw_table.sh 127.0.0.1");
    EXPECT_EQ(item.scheduleExpr, "[* * * * * ?]");
    EXPECT_EQ(item.scheduleIntv, std::chrono::seconds{300});
    EXPECT_EQ(item.labelAddInfos.size(), size_t(5));
    LabelAddInfo labelAddInfo = item.labelAddInfos[1];
    EXPECT_EQ(labelAddInfo.name, "__script_name__");
    EXPECT_EQ(labelAddInfo.value,
              "/home/staragent/plugins/Argus/local_data/libexec/alimonitor/check_xgw_table.sh 127.0.0.1");
    EXPECT_EQ(labelAddInfo.type, 2);
    EXPECT_EQ(item.timeOut, std::chrono::seconds{90});
    EXPECT_EQ(item.resultFormat, ALIMONITOR_JSON_FORMAT);
    EXPECT_TRUE(item.reportStatus);
    EXPECT_EQ(item.scriptUser, "root");
    EXPECT_EQ(item.metricMetas.size(), size_t(2));
    MetricMeta metricMeta = item.metricMetas[0];
    EXPECT_EQ(metricMeta.name, "table");
    EXPECT_EQ(metricMeta.type, 2);
    EXPECT_TRUE(metricMeta.isMetric);
    metricMeta = item.metricMetas[1];
    EXPECT_EQ(metricMeta.name, "result");
    EXPECT_EQ(metricMeta.type, 1);
    EXPECT_FALSE(metricMeta.isMetric);
    cout << "exit LoadAliScriptTaskFromFile" << endl;
}

TEST_F(Core_TaskMonitorTest, LoadReceiveTaskFromFile) {
    EXPECT_EQ(1, pShared->LoadReceiveTaskFromFile());
    TaskManager *pM = SingletonTaskManager::Instance();
    auto ItemMap = pM->ReceiveItems().Get();
    EXPECT_EQ(size_t(1), ItemMap->size());
    int mid = 1;
    EXPECT_TRUE(ItemMap->find(mid) != ItemMap->end());
    ReceiveItem item = (*ItemMap)[mid];
    EXPECT_EQ(item.outputVector.size(), size_t(1));
    EXPECT_EQ(item.name, "staragent");
    EXPECT_EQ(0, pShared->LoadReceiveTaskFromFile());
}

TEST_F(Core_TaskMonitorTest, LoadExporterTaskFromFile) {
    cout << "in LoadExporterTaskFromFile" << endl;
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir((TEST_CONF_PATH / "conf").string());
    auto pMonitor = std::make_shared<TaskMonitor>();
    EXPECT_EQ(pMonitor->mExporterTaskFile,
              (TEST_CONF_PATH / "conf" / "local_data" / "conf" / "exporterTask.json").string());
    pMonitor->mExporterTaskFile = (TEST_CONF_PATH / "conf" / "task" / "exporterTask.json").string();
    EXPECT_EQ(1, pMonitor->LoadExporterTaskFromFile());
    TaskManager *pM = SingletonTaskManager::Instance();
    // EXPECT_TRUE(pM->ExporterItemsChanged());
    vector<ExporterItem> items = *pM->ExporterItems();
    EXPECT_EQ(items.size(), size_t(1));
    ExporterItem item = items[0];
    EXPECT_EQ(item.target, "localhost:8080");
    EXPECT_EQ(item.interval, std::chrono::seconds{15});
    EXPECT_EQ(item.timeout, std::chrono::seconds{10});
    EXPECT_EQ(item.metricFilterInfos.size(), size_t(3));
    MetricFilterInfo metricFilterInfo = item.metricFilterInfos[0];
    EXPECT_EQ(metricFilterInfo.name, "container_memory_rss");
    EXPECT_EQ(metricFilterInfo.tagMap.size(), size_t(1));
    EXPECT_EQ(metricFilterInfo.tagMap["id"],
              "/docker/8e0100346912f327f0cb1bd66589443a9b1d0f5ea188c117c0191c9c470a3d2e");
    EXPECT_EQ(metricFilterInfo.metricName, "container_memory_rss_rename");
    EXPECT_EQ(item.labelAddInfos.size(), size_t(3));
    LabelAddInfo labelAddInfo = item.labelAddInfos[1];
    EXPECT_EQ(labelAddInfo.name, "instance");
    EXPECT_EQ(labelAddInfo.value, "INSTANCE");
    EXPECT_EQ(labelAddInfo.type, 1);
    cout << "exit LoadExporterTaskFromFile" << endl;
}

TEST_F(Core_TaskMonitorTest, LoadShennongExporterTaskFromFile) {
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir((TEST_CONF_PATH / "conf").string());
    auto pMonitor = std::make_shared<TaskMonitor>();
    EXPECT_EQ(pMonitor->mShennongExporterTaskFile,
              (TEST_CONF_PATH / "conf" / "local_data" / "conf" / "shennongExporterTask.json").string());
    pMonitor->mShennongExporterTaskFile = (TEST_CONF_PATH / "conf" / "task" / "exporterTask.json").string();

    EXPECT_EQ(1, pMonitor->LoadShennongExporterTaskFromFile());
    TaskManager *pM = SingletonTaskManager::Instance();
    EXPECT_TRUE((bool)pM->ShennongExporterItems());

    vector<ExporterItem> items = *pM->ShennongExporterItems();
    // EXPECT_FALSE((bool)pM->ShennongExporterItems());
    EXPECT_EQ(items.size(), size_t(1));
    ExporterItem item = items[0];
    EXPECT_EQ(item.target, "localhost:8080");
    EXPECT_EQ(item.interval, std::chrono::seconds{15});
    EXPECT_EQ(item.timeout, std::chrono::seconds{10});
    EXPECT_EQ(item.metricFilterInfos.size(), size_t(3));
    MetricFilterInfo metricFilterInfo = item.metricFilterInfos[0];
    EXPECT_EQ(metricFilterInfo.name, "container_memory_rss");
    EXPECT_EQ(metricFilterInfo.tagMap.size(), size_t(1));
    EXPECT_EQ(metricFilterInfo.tagMap["id"],
              "/docker/8e0100346912f327f0cb1bd66589443a9b1d0f5ea188c117c0191c9c470a3d2e");
    EXPECT_EQ(metricFilterInfo.metricName, "container_memory_rss_rename");
    EXPECT_EQ(item.labelAddInfos.size(), size_t(3));
    LabelAddInfo labelAddInfo = item.labelAddInfos[1];
    EXPECT_EQ(labelAddInfo.name, "instance");
    EXPECT_EQ(labelAddInfo.value, "INSTANCE");
    EXPECT_EQ(labelAddInfo.type, 1);
    cout << "exit LoadExporterTaskFromFile" << endl;
}

TEST_F(Core_TaskMonitorTest, LoadHttpReceiveTaskFromFile) {
    cout << "in LoadHttpReceiveTaskFromFiles" << endl;
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    auto pMonitor = std::make_shared<TaskMonitor>();
    EXPECT_EQ(pMonitor->mHttpReceiveTaskFile,
              (TEST_CONF_PATH / "conf" / "local_data" / "conf" / "httpReceiveTask.json").string());
    pMonitor->mHttpReceiveTaskFile = (TEST_CONF_PATH / "conf" / "task" / "httpReceiveTask.json").string();
    EXPECT_EQ(1, pMonitor->LoadHttpReceiveTaskFromFile());
    EXPECT_EQ(0, pMonitor->LoadHttpReceiveTaskFromFile()); // 重复加载，直接返回
    std::shared_ptr<HttpReceiveItem> prevHttpReceiveItem;
    TaskManager *pM = SingletonTaskManager::Instance();
    EXPECT_TRUE(pM->GetHttpReceiveItem().Get(prevHttpReceiveItem));
    HttpReceiveItem item = *pM->GetHttpReceiveItem().Get();
    EXPECT_FALSE(pM->GetHttpReceiveItem().Get(prevHttpReceiveItem));
    EXPECT_EQ(item.outputVector.size(), size_t(1));
    EXPECT_EQ(item.labelAddInfos.size(), size_t(3));
    cout << "exit LoadHttpReceiveTaskFromFiles" << endl;
}

TEST_F(Core_TaskMonitorTest, ParseBaseMetricInfo) {
    string json;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "baseMetric.json").string(), json);
    auto pMonitor = std::make_shared<TaskMonitor>();
    pMonitor->ParseBaseMetricInfo(json);
    TaskManager *pTask = SingletonTaskManager::Instance();
    std::shared_ptr<map<std::string, AliModuleItem>> prevAliMap;
    EXPECT_TRUE(pTask->AliModuleItems().Get(prevAliMap));
    map<std::string, AliModuleItem> aliMap = *pTask->AliModuleItems().Get();
    EXPECT_FALSE(pTask->AliModuleItems().Get(prevAliMap));
    EXPECT_EQ(aliMap.size(), size_t(1));
    for (const auto &it: aliMap) {
        EXPECT_EQ(it.second.blacklist.size(), size_t(1));
        EXPECT_EQ(it.second.moduleName, "cpu");
        cout << "key=" << it.first
             << ", module: " << it.second.moduleName
             << ", addLabelSize=" << it.second.labelAddInfos.size()
             << endl;
    }
}


TEST_F(Core_TaskMonitorTest, ParseBaseMetricInfoFail) {
    {
        TaskMonitor taskMonitor;
        EXPECT_EQ(-1, taskMonitor.ParseBaseMetricInfo("{}", false));
    }

    {
        TaskManager tm;
        auto *origin = SingletonTaskManager::swap(&tm);
        defer(SingletonTaskManager::swap(origin));

        SingletonLogger::Instance()->switchLogCache(true);
        defer(SingletonLogger::Instance()->switchLogCache(false));

        const char *szJson = R"XX([{
"interval": 0
}])XX";
        TaskMonitor taskMonitor;
        EXPECT_EQ(0, taskMonitor.ParseBaseMetricInfo(szJson, false));
        std::string cache = SingletonLogger::Instance()->getLogCache();
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "invalid"));
    }
}

TEST_F(Core_TaskMonitorTest, ParseUnifiedConfig) {
    cout << "in ParseUnifiedConfig" << endl;
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    auto pMonitor = std::make_shared<TaskMonitor>();
    pMonitor->mModuleTaskFile = (TEST_CONF_PATH / "conf" / "moduleTask.json").string();
    pMonitor->mScriptTaskFile = (TEST_CONF_PATH / "conf" / "scriptTask.json").string();
    pMonitor->mExporterTaskFile = (TEST_CONF_PATH / "conf" / "exporterTask.json").string();
    pMonitor->mHttpReceiveTaskFile = (TEST_CONF_PATH / "conf" / "httpReceiveTask.json").string();
    pMonitor->mBaseMetricFile = (TEST_CONF_PATH / "conf" / "baseMetric.json").string();
    string json;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "unifiedConfig.json").string(), json);
    EXPECT_EQ(0, pMonitor->ParseUnifiedConfig(json));
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "unifiedConfig.json").string(), json);
    EXPECT_EQ(0, pMonitor->ParseUnifiedConfig(json));

    TaskManager *pM = SingletonTaskManager::Instance();
    std::shared_ptr<vector<ExporterItem>> prevExporterItems;
    EXPECT_TRUE(pM->ExporterItems());
    vector<ExporterItem> exporterItems = *pM->ExporterItems();
    EXPECT_EQ(exporterItems.size(), size_t(2));
    std::shared_ptr<HttpReceiveItem> prevHttpReceiveItem;
    EXPECT_TRUE(pM->GetHttpReceiveItem().Get(prevHttpReceiveItem));
    // EXPECT_TRUE(pM->ModuleItemChanged());
    EXPECT_EQ(pM->ModuleItems()->size(), size_t(2));
    //与customTask等平级的scritpTask的解析是不需要的，被注释掉了。
    /*
    EXPECT_TRUE(pM->ScriptItemChanged());
    EXPECT_EQ(pM->GetScriptItemsNum(),2);
    */
    std::shared_ptr<std::map<std::string, AliModuleItem>> prevAliModuleItems;
    EXPECT_TRUE(pM->AliModuleItems().Get(prevAliModuleItems));
    std::map<std::string, AliModuleItem> alimonitorItemMap = *pM->AliModuleItems().Get();
    EXPECT_EQ(alimonitorItemMap.size(), size_t(3));

    boost::system::error_code ec;
    fs::remove(pMonitor->mModuleTaskFile, ec);
    // fs::remove(pMonitor->mScriptTaskFile, ec);
    fs::remove(pMonitor->mExporterTaskFile, ec);
    fs::remove(pMonitor->mHttpReceiveTaskFile, ec);
    fs::remove(pMonitor->mBaseMetricFile, ec);
}

TEST_F(Core_TaskMonitorTest, ParseUnifiedConfig_ExporterTask) {
    cout << "in ParseUnifiedConfig" << endl;
    string json;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "unify.json").string(), json);
    unique_ptr<TaskMonitor> pMonitor(new TaskMonitor());
    EXPECT_EQ(0, pMonitor->ParseUnifiedConfig(json));
    TaskManager *pM = SingletonTaskManager::Instance();
    std::shared_ptr<vector<ExporterItem>> prevExporterItems;
    EXPECT_TRUE(pM->ExporterItems());
    vector<ExporterItem> items = *pM->ExporterItems();
    EXPECT_EQ(size_t(4), items.size());
    if (items.size() > 3) {
        EXPECT_EQ(items[0].name, "pod_collect1");
        EXPECT_EQ(items[1].name, "alimetrics");
        EXPECT_EQ(items[2].name, "pod_collect11");
        EXPECT_EQ(items[3].name, "pod_collect22");
    }
    boost::system::error_code ec;
    fs::remove(TEST_CONF_PATH / "conf" / "local_data" / "conf" / "exporterTask.json", ec);
    fs::remove(TEST_CONF_PATH / "conf" / "local_data" / "conf" / "baseMetric.json", ec);
}

TEST_F(Core_TaskMonitorTest, ParseUnifiedConfig_ScriptTask) {
    cout << "in ParseUnifiedConfig" << endl;
    string json;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "unify_script.json").string(), json);
    unique_ptr<TaskMonitor> pMonitor(new TaskMonitor());
    EXPECT_EQ(0, pMonitor->ParseUnifiedConfig(json));
    TaskManager *pM = SingletonTaskManager::Instance();
    // EXPECT_TRUE(pM->AliScriptItemChanged());
    std::shared_ptr<map<string, ScriptItem>> ScriptItemMap = pM->AliScriptItems().Get();
    EXPECT_EQ(size_t(2), ScriptItemMap->size());
    if (ScriptItemMap->size() > 1) {
        auto it = ScriptItemMap->begin();
        ScriptItem item = it->second;
        EXPECT_EQ(item.name, "ut-script-1");
        EXPECT_EQ(item.scheduleExpr, "[* * 2-8 * * ?]");
        EXPECT_EQ(item.outputVector.size(), size_t(1));
        if (!item.outputVector.empty()) {
            EXPECT_EQ(item.outputVector[0], make_pair(string("alimonitor"), string("")));
        }
        EXPECT_EQ(item.labelAddInfos.size(), size_t(6));
        if (!item.labelAddInfos.empty()) {
            EXPECT_EQ(item.labelAddInfos[0].name, "__task_name__");
            EXPECT_EQ(item.labelAddInfos[0].value, "ut-script-1");
            EXPECT_EQ(item.labelAddInfos[0].type, 2);
        }
        EXPECT_EQ(item.collectUrl, "/home/staragent/plugins/raptor-agent/cpu_iss.sh 127.0.0.1");
        EXPECT_EQ(item.firstSche.time_since_epoch().count(), 0);
        EXPECT_EQ(item.timeOut, std::chrono::seconds{90});
        EXPECT_EQ(item.scheduleIntv, std::chrono::seconds{300});
        EXPECT_EQ(item.name, "ut-script-1");
        EXPECT_EQ(item.resultFormat, ALIMONITOR_JSON_FORMAT);
        EXPECT_TRUE(item.reportStatus);
        EXPECT_EQ(item.scriptUser, "root");
        EXPECT_EQ(item.metricMetas.size(), size_t(2));
        MetricMeta metricMeta = item.metricMetas[0];
        EXPECT_EQ(metricMeta.name, "table");
        EXPECT_EQ(metricMeta.type, 2);
        EXPECT_TRUE(metricMeta.isMetric);
        metricMeta = item.metricMetas[1];
        EXPECT_EQ(metricMeta.name, "result");
        EXPECT_EQ(metricMeta.type, 1);
        EXPECT_FALSE(metricMeta.isMetric);
        it++;
        item = it->second;
        EXPECT_EQ(item.name, "ut-script-2");
    }

    boost::system::error_code ec;
    fs::remove(TEST_CONF_PATH / "conf/local_data/conf/aliScriptTask.json", ec);
}

TEST_F(Core_TaskMonitorTest, testErrorFormat) {
    string json;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "error" / "baseMetric.json").string(), json);
    auto pMonitor = std::make_shared<TaskMonitor>();
    EXPECT_EQ(0, pMonitor->ParseBaseMetricInfo(json));
    json.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "error" / "exporterTask.json").string(), json);
    EXPECT_EQ(1, pMonitor->ParseExporterTaskInfo(json));
    json.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "error" / "httpReceiveTask.json").string(), json);
    EXPECT_EQ(1, pMonitor->ParseHttpReceiveTaskInfo(json));
    json.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "error" / "scriptTask.json").string(), json);
    EXPECT_EQ(0, pMonitor->ParseScriptTaskInfo(json));
    json.clear();
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "error" / "moduleTask.json").string(), json);
    auto fnSave = [](const std::shared_ptr<std::map<std::string, ModuleItem>> &d) {
        SingletonTaskManager::Instance()->SetModuleItems(d);
    };
    EXPECT_EQ(1, pMonitor->ParseModuleTaskInfo(json, fnSave));
}

TEST_F(Core_TaskMonitorTest, ParseModuleTaskInfo) {
    auto discard = [](const std::shared_ptr<std::map<std::string, ModuleItem>> &) {};
    EXPECT_EQ(-1, pShared->ParseModuleTaskInfo("[]", discard));

    std::shared_ptr<std::map<std::string, ModuleItem>> data;
    auto save = [&](const std::shared_ptr<std::map<std::string, ModuleItem>> &d) {
        data = d;
    };
    EXPECT_EQ(0, pShared->ParseModuleTaskInfo(R"({})", discard)); // no moduleItem
    EXPECT_EQ(0, pShared->ParseModuleTaskInfo(R"({"moduleItem":[{}]})", save));  // no name
    EXPECT_EQ(0, pShared->ParseModuleTaskInfo(R"({"moduleItem":[{"name":"test","cron":"[* * ?]"}]})",
                                              save));  // invalid cron
    EXPECT_EQ(0, pShared->ParseModuleTaskInfo(R"({"moduleItem":[{"name":"test","cron":"[* * * * * ?]"}]})",
                                              save));  // no interval
    EXPECT_EQ(0, pShared->ParseModuleTaskInfo(
            R"({"moduleItem":[{"name":"test","cron":"[* * * * * ?]","interval":15,"output":null}]})",
            save));  // no output
    EXPECT_EQ(1, pShared->ParseModuleTaskInfo(
            R"({"moduleItem":[{"name":"test","cron":"[* * * * * ?]","interval":15,"output":[{"type":"prometheus"}]}]})",
            save));
    EXPECT_EQ(data->size(), size_t(1));
    std::cout << "mid: " << data->begin()->first << std::endl;
    EXPECT_EQ(data->begin()->second.outputVector.size(), size_t(1));
    EXPECT_EQ(data->begin()->second.outputVector[0].first, "prometheus");
}

TEST_F(Core_TaskMonitorTest, GetModuleItemMid) {
    pShared->mModuleTaskFile = (TEST_CONF_PATH / "conf" / "task" / "moduleTask1.json").string();
    EXPECT_EQ(1, pShared->LoadModuleTaskFromFile());
    pShared->mModuleTaskFile = (TEST_CONF_PATH / "conf" / "task" / "moduleTask.json").string();
    // map<string,ModuleItem> moduleCollectItemMap;
    TaskManager *pM = SingletonTaskManager::Instance();
    auto moduleCollectItemMap = pM->ModuleItems();
    EXPECT_EQ(size_t(1), moduleCollectItemMap->size());
}

TEST_F(Core_TaskMonitorTest, GetScriptItemMid) {
    pShared->mScriptTaskFile = (TEST_CONF_PATH / "conf" / "task" / "scriptTask1.json").string();
    EXPECT_EQ(1, pShared->LoadScriptTaskFromFile());
    pShared->mScriptTaskFile = (TEST_CONF_PATH / "conf" / "task" / "scriptTask.json").string();
    // map<string,ScriptItem> scriptItemMap;
    TaskManager *pM = SingletonTaskManager::Instance();
    std::shared_ptr<map<string, ScriptItem>> scriptItemMap = pM->ScriptItems().Get();
    EXPECT_EQ(size_t(1), scriptItemMap->size());
}

TEST_F(Core_TaskMonitorTest, GetExporterItemMid) {
    pShared->mExporterTaskFile = (TEST_CONF_PATH / "conf" / "task/exporterTask1.json").string();
    EXPECT_EQ(2, pShared->LoadExporterTaskFromFile());
    pShared->mExporterTaskFile = (TEST_CONF_PATH / "conf" / "task/exporterTask.json").string();
    TaskManager *pM = SingletonTaskManager::Instance();
    vector<ExporterItem> exporterItems = *pM->ExporterItems();
    EXPECT_EQ(size_t(2), exporterItems.size());
    if (exporterItems.size() > 1) {
        EXPECT_EQ(exporterItems[0].mid, exporterItems[1].mid);
    }
}

TEST_F(Core_TaskMonitorTest, ParseCmsDetectTaskInfo) {
    string jsonStr;
    FileUtils::ReadFileContent((TEST_CONF_PATH / "conf" / "task" / "cmsDetect.json").string(), jsonStr);
    EXPECT_EQ(0, pShared->ParseCmsDetectTaskInfo(jsonStr));
    map<string, string> addTags;
    SingletonTaskManager::Instance()->GetCmsDetectAddTags(addTags);
    EXPECT_EQ(size_t(7), addTags.size());
    EXPECT_EQ("1153704480835126", addTags["userId"]);
    EXPECT_EQ("instanceId", addTags["UDF_RENAME_SN"]);
    EXPECT_EQ("1", addTags["UDF_CC_NOCHECK"]);
    EXPECT_EQ("7", addTags["SYS_DT"]);
    EXPECT_EQ("1130", addTags["SYS_CCI"]);
    EXPECT_EQ("1130:7", addTags["SYS_DT_CCI"]);
    EXPECT_EQ(SystemUtil::getSn(), addTags["sn"]);
    vector<PingItem> pingItems = SingletonTaskManager::Instance()->PingItems().Values();
    vector<TelnetItem> telnetItems = SingletonTaskManager::Instance()->TelnetItems().Values();
    vector<HttpItem> httpItems = SingletonTaskManager::Instance()->HttpItems().Values();
    EXPECT_EQ(size_t(2), pingItems.size());
    if (pingItems.size() == size_t(2)) {
        int index = ("cms-cloudmonitor.aliyun.com" == pingItems[0].host ? 0 : 1);
        EXPECT_EQ("cms-cloudmonitor.aliyun.com", pingItems[index].host);
        EXPECT_EQ("1476145651891863555", pingItems[index].taskId);
        EXPECT_EQ("[* * * * * 1-7]", pingItems[index].effective.string());
        EXPECT_FALSE(pingItems[index].effective.timePeriod.empty());

        index = (index + 1) % 2;
        EXPECT_EQ("www.baidu.com", pingItems[index].host);
        EXPECT_EQ("980384", pingItems[index].taskId);
        EXPECT_EQ("", pingItems[index].effective.string());
        EXPECT_TRUE(pingItems[index].effective.timePeriod.empty());
    }
    EXPECT_EQ(size_t(3), httpItems.size());
    if (httpItems.size() == size_t(3)) {
        auto find = [&](const std::string &host) {
            for (size_t i = 0; i < httpItems.size(); i++) {
                if (httpItems[i].uri == host) {
                    return i;
                }
            }
            return std::string::npos;
        };
        {
            size_t index = find("http://www.baidx.com");
            ASSERT_NE(index, std::string::npos);
            EXPECT_EQ("GET", httpItems[index].method);
            EXPECT_EQ("http://www.baidx.com", httpItems[index].uri);
            EXPECT_EQ("140", httpItems[index].taskId);
        }
        {
            size_t index = find("http://www.baidz.com");
            ASSERT_NE(index, std::string::npos);
            EXPECT_EQ("GET", httpItems[index].method);
            EXPECT_EQ("http://www.baidz.com", httpItems[index].uri);
            EXPECT_EQ("1002824", httpItems[index].taskId);
        }
        {
            size_t index = find("http://www.baidu.com");
            ASSERT_NE(index, std::string::npos);
            EXPECT_EQ("GET", httpItems[index].method);
            EXPECT_EQ("http://www.baidu.com", httpItems[index].uri);
            EXPECT_EQ("1476145651891863554", httpItems[index].taskId);
        }
    }
    EXPECT_EQ(size_t(1), telnetItems.size());
    if (telnetItems.size() == size_t(1)) {
        EXPECT_EQ("telnet://10.137.69.8:22", telnetItems[0].uri);
        EXPECT_EQ("1476145651891863556", telnetItems[0].taskId);
    }
}

// TEST_F(Core_TaskMonitorTest, CalcMid_ScriptItem) {
//     ScriptItem item;
//     item.metricFilterInfos.resize(1);
//     item.metricFilterInfos[0].tagMap["instanceId"] = "i-123";
//     item.metricFilterInfos[0].tagMap["userId"] = "123456";
//
//     std::string mid = TaskMonitor::CalcMid(item);
//     LogInfo("mid: {}", mid);
//     EXPECT_EQ(mid, "cd7d6c376ebac8c03de1f47ab524ea16");
// }

TEST_F(Core_TaskMonitorTest, ParseProcessInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    auto parse = [](const char *szJson) -> json::Object {
        LogInfo("json: {}", szJson);
        std::string errMsg;
        json::Object root = json::parseObject(szJson, errMsg);
        EXPECT_EQ(errMsg, "");
        return root;
    };

    {
        TaskMonitor::ParseProcessInfo(parse("{}"));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no collectConfig"));
    }
    {
        const char *szJson = R"XX({
"collectConfig": {},
})XX";
        TaskMonitor::ParseProcessInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no processConfigs"));
    }
    {
        const char *szJson = R"XX({
"collectConfig": {
    "processConfigs": [
    "hello", {
        "command":"unknown"
    }, {
        "command":"number",
        "name": "test",
    }],
},
})XX";
        TaskMonitor::ParseProcessInfo(parse(szJson));
        const auto &collectItems = SingletonTaskManager::Instance()->ProcessCollectItems();
        Sync(collectItems)
                {
                    EXPECT_EQ(collectItems->size(), size_t(1));
                    EXPECT_EQ((*collectItems)[0].name, "test");
                }}}
    }
}

TEST_F(Core_TaskMonitorTest, ParseTelnetInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    auto parse = [](const char *szJson) -> json::Object {
        LogInfo("json: {}", szJson);
        std::string errMsg;
        json::Object root = json::parseObject(szJson, errMsg);
        EXPECT_EQ(errMsg, "");
        return root;
    };

    {
        const char *szJson = R"XX({
"TELNET":{} // not array
})XX";
        TaskMonitor::ParseTelnetInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no TELNET"));
    }
    {
        const char *szJson = R"XX({
    "TELNET":["[]"]
})XX";
        TaskMonitor::ParseTelnetInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "telnet item is invalid"));
    }

    EXPECT_TRUE(tm.TelnetItems().IsEmpty());
}

TEST_F(Core_TaskMonitorTest, ParsePingInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    auto parse = [](const char *szJson) -> json::Object {
        LogInfo("json: {}", szJson);
        std::string errMsg;
        json::Object root = json::parseObject(szJson, errMsg);
        EXPECT_EQ(errMsg, "");
        return root;
    };

    {
        const char *szJson = R"XX({
"PING":{} // not array
})XX";
        TaskMonitor::ParsePingInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no PING"));
    }
    {
        const char *szJson = R"XX({
    "PING":["null"]
})XX";
        TaskMonitor::ParsePingInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "ping item is invalid"));
    }

    EXPECT_TRUE(tm.PingItems().IsEmpty());
}

TEST_F(Core_TaskMonitorTest, ParseHttpInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    auto parse = [](const char *szJson) -> json::Object {
        LogInfo("json: {}", szJson);
        std::string errMsg;
        json::Object root = json::parseObject(szJson, errMsg);
        EXPECT_EQ(errMsg, "");
        return root;
    };

    {
        const char *szJson = R"XX({
"HTTP":{} // not array
})XX";
        TaskMonitor::ParseHttpInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no HTTP"));
    }
    {
        const char *szJson = R"XX({
    "HTTP":["null"]
})XX";
        TaskMonitor::ParseHttpInfo(parse(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "http item is invalid"));
    }

    EXPECT_TRUE(tm.HttpItems().IsEmpty());
}

TEST_F(Core_TaskMonitorTest, ParseCmsTopNTaskInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    {
        EXPECT_EQ(-1, TaskMonitor::ParseCmsTopNTaskInfo(""));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "invalid"));
    }
    {
        const char *szJson = R"XX({
    "count": 2,
})XX";
        EXPECT_EQ(0, TaskMonitor::ParseCmsTopNTaskInfo(szJson));
        TopNItem item;
        tm.GetTopNItem(item);
        EXPECT_EQ(item.topN, 2);
    }
}

TEST_F(Core_TaskMonitorTest, ParseCmsProcessTaskInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    {
        EXPECT_EQ(-1, TaskMonitor::ParseCmsProcessTaskInfo(""));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "invalid"));
    }
    {
        const char *szJson = R"XX({
    "add_labels": [{
        "name": "a",
        "value": "b",
        "type": 2,  // 0-内置宏，1-环境变量,2-使用value
    }],
})XX";
        EXPECT_EQ(0, TaskMonitor::ParseCmsProcessTaskInfo(szJson));
        std::map<std::string, std::string> tags;
        tm.GetCmsProcessAddTags(tags);
        EXPECT_EQ(size_t(1), tags.size());
        EXPECT_EQ(tags["a"], "b");
    }
}

TEST_F(Core_TaskMonitorTest, ParseCmsDetectTaskInfoFail) {
    EXPECT_EQ(-1, TaskMonitor::ParseCmsDetectTaskInfo(""));
    EXPECT_EQ(-1, TaskMonitor::ParseCmsDetectTaskInfo("[]"));
}

TEST_F(Core_TaskMonitorTest, ParseCustomTaskInfo) {
    {
        std::unordered_map<std::string, std::vector<std::string>> content;
        TaskMonitor taskMonitor;
        EXPECT_EQ(-1, taskMonitor.ParseCustomTaskInfo("", content));
        EXPECT_EQ(-1, taskMonitor.ParseCustomTaskInfo("{}", content));
    }

    TaskMonitor taskMonitor;
    taskMonitor.mCmsDetectTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "detectTaskFile.json").string();
    taskMonitor.mCmsProcessTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "processTaskFile.json").string();
    taskMonitor.mCmsTopNTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "topTaskFile.json").string();
    std::vector<std::string> files{
            taskMonitor.mCmsDetectTaskFile,
            taskMonitor.mCmsProcessTaskFile,
            taskMonitor.mCmsTopNTaskFile,
            taskMonitor.mCmsDetectTaskFile,
    };
    defer(for (const auto &file: files) { RemoveFile(file); });

    const char *szJson = R"XX([{
    "type": "cmsDetectTask",
    "content": {},
},{
    "type": "cmsProcessTask",
    "content": {},
},{
    "type": "cmsTopNTask",
    "content": {},
},{
    "type": "udf",
    "content": {},
}])XX";
    std::unordered_map<std::string, std::vector<std::string>> content;
    EXPECT_EQ(0, taskMonitor.ParseCustomTaskInfo(szJson, content));
    EXPECT_EQ(size_t(1), content.size());
    EXPECT_NE(content.end(), content.find("udf"));
    EXPECT_EQ(content["udf"], std::vector<std::string>{"{}"});
}

TEST_F(Core_TaskMonitorTest, ParseUnifiedConfigFail) {
    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    {
        TaskMonitor taskMonitor;
        taskMonitor.mUnifiedConfigMd5 = "123";
        EXPECT_EQ(0, taskMonitor.ParseUnifiedConfig("", "123"));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "not changed"));
    }
    {
        const char *szJson = R"XX([])XX";
        TaskMonitor taskMonitor;
        EXPECT_EQ(-1, taskMonitor.ParseUnifiedConfig(szJson));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "invalid"));
    }
}

TEST_F(Core_TaskMonitorTest, ParseUnifiedConfig_loggerLokiTask) {
    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    const char *szJson = R"XX({
    "loggerTask": {},
    "lokiTask": {},
})XX";

    TaskMonitor taskMonitor;
    taskMonitor.mLoggerTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "loggerTaskFile.json").string();
    taskMonitor.mLokiTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "lokiTaskFile.json").string();
    std::vector<std::string> files{
            taskMonitor.mLoggerTaskFile,
            taskMonitor.mLokiTaskFile,
            taskMonitor.mCmsDetectTaskFile,
    };
    defer(for (const auto &file: files) { RemoveFile(file); });
    EXPECT_EQ(0, taskMonitor.ParseUnifiedConfig(szJson, "123"));
    EXPECT_NE(taskMonitor.mLoggerTaskMd5, "");
    EXPECT_NE(taskMonitor.mLokiTaskMd5, "");
    EXPECT_EQ(taskMonitor.mUnifiedConfigMd5, "123");
}

TEST_F(Core_TaskMonitorTest, LoadBaseMetricFromFile) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    {
        TaskMonitor taskMonitor;
        taskMonitor.mBaseMetricFile = (TEST_CONF_PATH / "conf" / "task" / "baseMetric~@~@.json").string();
        EXPECT_EQ(-1, taskMonitor.LoadBaseMetricFromFile());
    }
    {
        TaskMonitor taskMonitor;
        taskMonitor.mBaseMetricFile = (TEST_CONF_PATH / "conf" / "task" / "baseMetric.json").string();
        EXPECT_GT(taskMonitor.LoadBaseMetricFromFile(), 0);
        EXPECT_EQ(taskMonitor.LoadBaseMetricFromFile(), 0);
    }
}

TEST_F(Core_TaskMonitorTest, ParseAliScriptTaskInfo_FormatError) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    struct {
        const char *szJson;
        const int nExpect;
        const char *szExpect;
    } simpleTestCases[] = {
            {"",                          -1, "format error"},
            {R"XX({"scriptItem": {}})XX", -2, "no script task"},
            {R"XX({"scriptItem": []})XX", 0,  nullptr},
    };
    for (const auto &tc: simpleTestCases) {
        fmt::println("{:*>10} [{}] {}", "", (&tc - simpleTestCases), tc.szJson);
        TaskMonitor taskMonitor;
        EXPECT_EQ(tc.nExpect, taskMonitor.ParseAliScriptTaskInfo(tc.szJson, false));
        string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        if (tc.szExpect) {
            EXPECT_TRUE(StringUtils::Contain(cache, tc.szExpect));
        }
    }
}

TEST_F(Core_TaskMonitorTest, ParseAliScriptTaskInfo_ScriptItemError) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    using namespace boost::json;
    struct {
        const char *key;
        const boost::json::value val;
        const char *expect;
    } testCases[] = {
            {"name",         value_from(std::string{""}),              "no name"},
            {"name",         value_from(std::string{"abc"}),           "version error"},
            {"version",      value_from(std::string{"2"}),             nullptr},
            {"cron",         value_from(std::string{"abc"}),           "cron error"},
            {"cron",         value_from(std::string{"[* * * * * ?]"}), "cmd error"},
            {"cmd",          value_from(std::string{"ls -l"}),         "interval"},
            {"interval",     value_from(30),                           "timeout"},
            {"timeout",      value_from(std::string{"40"}),            "timeout(40s)>"},
            {"resultFormat", value_from(std::string{"1"}),             "outputItem is 0"},
            {"metrics",      object(),                                 nullptr},
    };
    std::map<std::string, boost::json::value> obj;
    const char *szJsonPattern = R"XX({{"scriptItem":[{}]}})XX";
    for (const auto &it: testCases) {
        obj[it.key] = it.val;
        std::string strJson = fmt::format(szJsonPattern, (obj.empty() ? "" : json::toString(obj)));
        // 10个前置星号
        fmt::println("{:*>10} [{}] {}", "", (&it - testCases), strJson);
        if (it.expect) {
            TaskMonitor taskMonitor;
            EXPECT_EQ(0, taskMonitor.ParseAliScriptTaskInfo(strJson, false));
            std::string cache = SingletonLogger::Instance()->getLogCache(true);
            std::cout << cache;
            EXPECT_TRUE(StringUtils::Contain(cache, it.expect));
        }
    }
}

TEST_F(Core_TaskMonitorTest, ParseAliScriptTaskInfo_MetricFilters) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    const char *szJson = R"XX({"scriptItem":[{
"cmd":"ls -l","cron":"[* * * * * ?]","interval":30,"name":"abc","resultFormat":"1","timeout":"40","version":"2",
"output":[{"type":"alimonitor"}],
"metrics":[{"name":"hello"}],
"metricMeta":[{"name":""}]
}]})XX";
    TaskMonitor taskMonitor;
    int exitCode = taskMonitor.ParseAliScriptTaskInfo(szJson, false);
    string cache = SingletonLogger::Instance()->getLogCache(true);
    std::cout << cache;

    ASSERT_EQ(1, exitCode);
    auto aliScriptItems = tm.AliScriptItems().Get();
    EXPECT_EQ(size_t(1), aliScriptItems->size());
    const auto &filters = aliScriptItems->begin()->second.metricFilterInfos;
    EXPECT_EQ(size_t(1), filters.size());
    EXPECT_EQ(filters[0].name, "hello");

    EXPECT_TRUE(StringUtils::Contain(cache, "invalid metricMeta"));
}

TEST_F(Core_TaskMonitorTest, ParseShennongExporterTaskInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    // SingletonLogger::Instance()->switchLogCache(true);
    // defer(SingletonLogger::Instance()->switchLogCache(false));

    TaskMonitor taskMonitor;
    EXPECT_EQ(-1, taskMonitor.ParseShennongExporterTaskInfo("{}", false));
    EXPECT_EQ(-1, taskMonitor.ParseShennongExporterTaskInfo(R"XX({"interval":0})XX", false));

    taskMonitor.mShennongExporterTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "shennong-exporter-task.json").string();
    {
        const char *szJson = R"XX([{
"interval": 2,
"timeout": 2,
"metrics_path":"",
"output": [{"type":""}],  // 返回0项
}])XX";
        RemoveFile(taskMonitor.mShennongExporterTaskFile);
        ASSERT_FALSE(fs::exists(taskMonitor.mShennongExporterTaskFile));
        defer(RemoveFile(taskMonitor.mShennongExporterTaskFile));
        EXPECT_EQ(0, taskMonitor.ParseShennongExporterTaskInfo(szJson, true));
        EXPECT_TRUE(fs::exists(taskMonitor.mShennongExporterTaskFile));
    }
    {
        const char *szJson = R"XX([{
"interval": 2,
"timeout": 2,
}])XX";
        RemoveFile(taskMonitor.mShennongExporterTaskFile);
        ASSERT_FALSE(fs::exists(taskMonitor.mShennongExporterTaskFile));
        defer(RemoveFile(taskMonitor.mShennongExporterTaskFile));
        EXPECT_EQ(1, taskMonitor.ParseShennongExporterTaskInfo(szJson, true));
        EXPECT_TRUE(fs::exists(taskMonitor.mShennongExporterTaskFile));
    }
}

TEST_F(Core_TaskMonitorTest, ParseExporterTaskInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    // SingletonLogger::Instance()->switchLogCache(true);
    // defer(SingletonLogger::Instance()->switchLogCache(false));

    TaskMonitor taskMonitor;
    EXPECT_EQ(-1, taskMonitor.ParseExporterTaskInfo("{}", false));
    EXPECT_EQ(-1, taskMonitor.ParseExporterTaskInfo(R"XX({"interval":0})XX", false));

    const char *szJson = R"XX([{
"interval": 2,
"timeout": 0,
"metrics_path":"",
"output": [{"type":"cms","conf":{}}],
}])XX";
    taskMonitor.mExporterTaskFile = (TEST_CONF_PATH / "conf" / "tmp" / "exporter-task.json").string();
    RemoveFile(taskMonitor.mExporterTaskFile);
    ASSERT_FALSE(fs::exists(taskMonitor.mExporterTaskFile));
    defer(RemoveFile(taskMonitor.mExporterTaskFile));
    EXPECT_EQ(1, taskMonitor.ParseExporterTaskInfo(szJson, true));
    EXPECT_TRUE(fs::exists(taskMonitor.mExporterTaskFile));
    EXPECT_EQ(tm.ExporterItems()->size(), size_t(1));
    EXPECT_EQ(tm.ExporterItems()->at(0).timeout, 2_s);
    EXPECT_EQ(tm.ExporterItems()->at(0).metricsPath, "/metrics");
}

TEST_F(Core_TaskMonitorTest, LoadReceiveTaskFromFileRobust) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    TaskMonitor taskMonitor;
    auto &taskFile = taskMonitor.mReceiveTaskFile;
    taskFile = (TEST_CONF_PATH / "conf" / "tmp" / "receivedTask.json").string();
    RemoveFile((taskFile));
    defer(RemoveFile((taskFile)));

    // 文件不存在
    EXPECT_EQ(-1, taskMonitor.LoadReceiveTaskFromFile());

    {
        WriteFileContent(taskFile, "[]");
        EXPECT_EQ(-1, taskMonitor.LoadReceiveTaskFromFile());
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "format error"));
    }
    {
        WriteFileContent(taskFile, "{}");
        EXPECT_EQ(-2, taskMonitor.LoadReceiveTaskFromFile());
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "no key(receiveItem)"));
    }

    using namespace boost::json;
    // [{"type":"msg","conf":{}}]
    boost::json::array output{{{"type", "msg"},
                               {"conf", {}}}};
    const struct {
        const char *key;
        const value val;
        const int expectCode;
        const char *expect;
    } testCases[] = {
            {nullptr,  nullptr,                          0, "name error"},
            {"name",   value_from(std::string{"hello"}), 0, "type error"},
            {"type",   value_from(1),                    0, "outputItem is 0"},
            {"output", output,                           1, nullptr},
    };
    const char *szJsonPattern = R"XX({{"receiveItem":[{}]}})XX";

    std::map<std::string, value> mapItems;
    for (const auto &tc: testCases) {
        if (tc.key && !tc.val.is_null()) {
            mapItems[tc.key] = tc.val;
        }
        std::string strJson = fmt::format(szJsonPattern, serialize(value_from(mapItems)));
        fmt::println("{:*>10} [{}] {}", "", (&tc - testCases), strJson);
        WriteFileContent(taskFile, strJson);
        EXPECT_EQ(tc.expectCode, taskMonitor.LoadReceiveTaskFromFile());
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        if (tc.expect) {
            EXPECT_TRUE(StringUtils::Contain(cache, tc.expect));
        }
    }
    EXPECT_EQ(tm.ReceiveItems().Size(), size_t(1));
}

TEST_F(Core_TaskMonitorTest, ParseScriptTaskInfo) {
    TaskManager tm;
    auto *origin = SingletonTaskManager::swap(&tm);
    defer(SingletonTaskManager::swap(origin));

    TaskMonitor taskMonitor;
    auto &taskFile = taskMonitor.mScriptTaskFile;
    taskFile = (TEST_CONF_PATH / "conf" / "tmp" / "scriptTaskFile.json").string();
    RemoveFile((taskFile));
    defer(RemoveFile((taskFile)));

    EXPECT_EQ(-1, taskMonitor.ParseScriptTaskInfo("[]"));
    EXPECT_EQ(-2, taskMonitor.ParseScriptTaskInfo(R"XX({"scriptItem":{}})XX"));

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    const char *szJsonPattern = R"XX({{"scriptItem":[{}]}})XX";
    using namespace boost::json;
    // [{"type":"msg","conf":{}}]
    boost::json::array output{{{"type", "msg"},
                               {"conf", {}}}};
    const struct {
        const char *key;
        const value val;
        const int expectCode;
        const char *expect;
    } testCases[] = {
            {nullptr,    nullptr,                                  0, "name error"},
            {"name",     value_from(std::string{"hello"}),         0, "version error"},
            {"version",  value_from(std::string{"2"}),             0, "cmd error"},
            {"cron",     value_from(std::string{"xxx"}),           0, "cron error"},
            {"cron",     value_from(std::string{"[* * * * * ?]"}), 0, "cmd error"},
            {"cmd",      value_from(std::string{"ls -l"}),         0, "interval"},
            {"interval", value_from(10),                           0, "timeout"},
            {"timeout",  value_from(10),                           0, "output error"},
            {"output",   output,                                   1, nullptr},
    };
    std::map<std::string, value> mapItems;
    for (const auto &tc: testCases) {
        if (tc.key && !tc.val.is_null()) {
            mapItems[tc.key] = tc.val;
        }
        std::string strJson = fmt::format(szJsonPattern, serialize(value_from(mapItems)));
        fmt::println("{:*>10} [{}][{}] {}", "", (&tc - testCases), (tc.key ? tc.key : "<nil>"), strJson);
        EXPECT_EQ(tc.expectCode, taskMonitor.ParseScriptTaskInfo(strJson, true));
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        if (tc.expect) {
            EXPECT_TRUE(StringUtils::Contain(cache, tc.expect));
        }
    }
    EXPECT_EQ(tm.ScriptItems().Size(), size_t(1));
}
