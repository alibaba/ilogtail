#include <gtest/gtest.h>
#include "cloudMonitor/rdma_collect.h"

#include <thread>

#include "common/Config.h"
#include "core/TaskManager.h"
#include "common/ModuleData.h"
#include "common/ModuleTool.h"
#include "common/UnitTestEnv.h"
#include "common/Chrono.h"
#include "common/TimeFormat.h"
#include "common/FileUtils.h"

#include "cloudMonitor/cloud_monitor_config.h"
#include "cloudMonitor/cloud_module_macros.h"

#ifdef max
#   undef max
#endif
#ifdef min
#   undef min
#endif

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace argus;

DECLARE_CMS_EXTERN_MODULE(rdma);

class Cms_RdmaCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();

        pShared = new RdmaCollect();
    }

    void TearDown() override {
        Delete(pShared);
        SingletonTaskManager::destroyInstance();
        SingletonConfig::destroyInstance();
    }

    RdmaCollect *pShared = nullptr;
};

TEST_F(Cms_RdmaCollectTest, ParseLines) {
    std::vector<std::string> columns = RdmaCollect::ParseLines("2018-8-9 10:02:02 CST");
    EXPECT_TRUE(columns.empty());

    columns = RdmaCollect::ParseLines("tx_bytes: 17671048460258 (7190671265652/10480377194606)");
    EXPECT_EQ(size_t(4), columns.size());
    EXPECT_EQ(columns[0], "tx_bytes");
    EXPECT_EQ(convert<int64_t>(columns[1]), int64_t(17671048460258));
    EXPECT_EQ(convert<int64_t>(columns[2]), int64_t(7190671265652));
    EXPECT_EQ(convert<int64_t>(columns[3]), int64_t(10480377194606));

    EXPECT_TRUE(RdmaCollect::ParseLines(": 17671048460258 (7190671265652/10480377194606)").empty());

    columns = RdmaCollect::ParseLines("tx_bytes  : 17671048460258");
    EXPECT_EQ(size_t(2), columns.size());
    EXPECT_EQ(columns[0], "tx_bytes");
    EXPECT_EQ(convert<int64_t>(columns[1]), int64_t(17671048460258));

    columns = RdmaCollect::ParseLines("tx_bytes  : 0 ( 0 /3 )");
    EXPECT_EQ(size_t(4), columns.size());
    EXPECT_EQ(columns[0], "tx_bytes");
    EXPECT_EQ(convert<int64_t>(columns[1]), int64_t(0));
    EXPECT_EQ(convert<int64_t>(columns[2]), int64_t(0));
    EXPECT_EQ(convert<int64_t>(columns[3]), int64_t(3));
}

TEST_F(Cms_RdmaCollectTest, TestDeviceRdmaData) {
    DeviceRdmaData rdmaData{};
    rdmaData.UpdateValue(6, 0);
    EXPECT_EQ(6, rdmaData.lastValue);
    rdmaData.UpdateValue(10, 990);
    EXPECT_EQ(4, rdmaData.value.Max());
}

struct TimedValue {
    int64_t value;
    int64_t millis;
};

class IncMetric : public SlidingWindow<int64_t> {
    std::vector<TimedValue> values;

public:
    IncMetric(const std::initializer_list<TimedValue> &l) :
            SlidingWindow<int64_t>(l.size() - 1), values(l.begin(), l.end()) {
        for (size_t i = 1; i < values.size(); ++i) {
            const TimedValue &cur = values[i];
            const TimedValue &pre = values[i - 1];
            double v = 1000 * double(cur.value - pre.value) / double(cur.millis - pre.millis);
            Update((int64_t) v);
        }
    }
};

TEST_F(Cms_RdmaCollectTest, CollectRdmaData) {
    pShared->mRdmaFile = (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data3").string();  // 初始化用，不记录在values里
    int64_t millis01 = pShared->UpdateRdmaData();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pShared->mRdmaFile = (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data4").string();
    int64_t millis02 = pShared->UpdateRdmaData();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    pShared->mRdmaFile = (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data5").string();
    int64_t millis03 = pShared->UpdateRdmaData();

    EXPECT_EQ(2, pShared->mRdmaDataMap[0].metric["num_of_qp"].value.Size());
    EXPECT_EQ(11, pShared->mRdmaDataMap[0].metric["num_of_qp"].lastValue);

    // 3 - rdma.data3
    // 4 - num_of_qp: 10
    // 5 - num_of_qp: 11
    IncMetric NumOfQp{TimedValue{6, millis01}, TimedValue{10, millis02}, TimedValue{11, millis03}};
    EXPECT_EQ(NumOfQp.Min(), pShared->mRdmaDataMap[0].metric["num_of_qp"].value.Min());
    EXPECT_EQ(NumOfQp.Avg(), pShared->mRdmaDataMap[0].metric["num_of_qp"].value.Avg());
    // 不存在eth0和eth1
    EXPECT_EQ(pShared->mRdmaDataMap[1].metric.find("num_of_qp"), pShared->mRdmaDataMap[1].metric.end());
    EXPECT_EQ(pShared->mRdmaDataMap[2].metric.find("num_of_qp"), pShared->mRdmaDataMap[2].metric.end());

    // 3 - rx_pause: 701 (2703/0)
    // 4 - rx_pause: 709 (2709/5)
    // 5 - rx_pause: 710 (2710/6)
    IncMetric Bond0RxPause{TimedValue{701, millis01}, TimedValue{709, millis02}, TimedValue{710, millis03}};
    EXPECT_EQ(Bond0RxPause.Max(), pShared->mRdmaDataMap[0].metric["rx_pause"].value.Max());
    EXPECT_EQ(Bond0RxPause.Min(), pShared->mRdmaDataMap[0].metric["rx_pause"].value.Min());
    EXPECT_EQ(Bond0RxPause.Avg(), pShared->mRdmaDataMap[0].metric["rx_pause"].value.Avg());

    IncMetric Eth0RxPause{TimedValue{2703, millis01}, TimedValue{2709, millis02}, TimedValue{2710, millis03}};
    EXPECT_EQ(Eth0RxPause.Max(), pShared->mRdmaDataMap[1].metric["rx_pause"].value.Max());
    EXPECT_EQ(Eth0RxPause.Min(), pShared->mRdmaDataMap[1].metric["rx_pause"].value.Min());
    EXPECT_EQ(Eth0RxPause.Avg(), pShared->mRdmaDataMap[1].metric["rx_pause"].value.Avg());

    IncMetric Eth1RxPause{TimedValue{0, millis01}, TimedValue{5, millis02}, TimedValue{6, millis03}};
    EXPECT_EQ(Eth1RxPause.Max(), pShared->mRdmaDataMap[2].metric["rx_pause"].value.Max());
    EXPECT_EQ(Eth1RxPause.Min(), pShared->mRdmaDataMap[2].metric["rx_pause"].value.Min());
    EXPECT_EQ(Eth1RxPause.Avg(), pShared->mRdmaDataMap[2].metric["rx_pause"].value.Avg());

    MetricData metricData;
    EXPECT_FALSE(pShared->GetRdmaMetricData("bond0", pShared->mRdmaDataMap[0], metricData));
    HpcClusterItem hpcClusterItem;
    hpcClusterItem.isValid = true;
    SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
    EXPECT_TRUE(pShared->GetRdmaMetricData("bond0", pShared->mRdmaDataMap[0], metricData));
    EXPECT_EQ(metricData.tagMap.size(), size_t(5));
    EXPECT_EQ(metricData.valueMap["num_of_qp_max"], NumOfQp.Max());
    EXPECT_EQ(metricData.valueMap["num_of_qp_min"], NumOfQp.Min());
    EXPECT_EQ(metricData.valueMap["num_of_qp_avg"], NumOfQp.Avg());
}

TEST_F(Cms_RdmaCollectTest, CollectRPingData) {
    const std::string rpingFile = (TEST_CONF_PATH / "conf" / "RDMA" / "rping.data").string();
    const int64_t now = NowMillis();
    std::cout << "now: " << std::chrono::FromMillis(now) << ", timestamp: " << now << std::endl;
    //有效数据
    std::stringstream ss;
    {
        ss << "10.11.11.1 10 " << convert(now) << std::endl;
    }
    //无效数据
    const int64_t now2 = now - 1000 * 70;
    {
        ss << "10.11.11.2 20 " << convert(now2) << std::endl;
    }
    const int64_t now3 = now - 1000 * 40;
    {
        //有效数据
        ss << "10.11.11.3    30 " << convert(now3) << std::endl;
    }
    ASSERT_TRUE(WriteFileContent(rpingFile, ss.str()));

    pShared->mRPingFile = rpingFile;
    vector<RPingData> rPingdatas;
    pShared->CollectRPingData(rPingdatas);
    ASSERT_EQ(rPingdatas.size(), size_t(2));
    EXPECT_EQ(rPingdatas[0].ip, "10.11.11.1");
    EXPECT_EQ(rPingdatas[0].ts, now);
    EXPECT_EQ(rPingdatas[0].rt, 10);
    EXPECT_EQ(rPingdatas[1].ip, "10.11.11.3");
    EXPECT_EQ(rPingdatas[1].ts, now3);
    EXPECT_EQ(rPingdatas[1].rt, 30);
    MetricData metricData;
    cloudMonitor::RdmaCollect::GetRPingMetricData(rPingdatas[0], metricData);
    EXPECT_EQ(metricData.valueMap["rt"], 10);
    EXPECT_EQ(metricData.valueMap["rping_ts"], now);
    EXPECT_EQ(metricData.valueMap.size(), size_t(3));
    EXPECT_EQ(metricData.tagMap.size(), size_t(4));
    EXPECT_EQ(metricData.tagMap["ip"], "10.11.11.1");
}

TEST_F(Cms_RdmaCollectTest, GetQosMetricData) {
    pShared->mQosCheckFile = "/usr/local/bin/rdma_qos_check";
    MetricData metricData;
    HpcClusterItem hpcClusterItem;
    if (fs::exists(pShared->mQosCheckFile)) {
        hpcClusterItem.isValid = true;
        SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
        EXPECT_FALSE(pShared->GetQosMetricData(metricData));
    }
    hpcClusterItem.isValid = false;
    SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
    pShared->mQosCheckFile = "exit 1";
    EXPECT_FALSE(pShared->GetQosMetricData(metricData));
    hpcClusterItem.isValid = true;
    SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
    pShared->mQosCheckFile = "exit 1";
    EXPECT_TRUE(pShared->GetQosMetricData(metricData));
    EXPECT_EQ(metricData.tagMap.size(), size_t(3));
    EXPECT_EQ(metricData.valueMap["exitValue"], 1);
}

TEST_F(Cms_RdmaCollectTest, Collect) {
#ifdef WIN32
    std::string data;
    EXPECT_EQ(0, pShared->Collect(data)); // windows下没有rdma
    EXPECT_TRUE(data.empty());
#else
    const std::string rpingFile = (TEST_CONF_PATH / "conf" / "RDMA/rping.data").string();
    //产生rping.data文件
    const int64_t now = NowMillis();

    std::stringstream ss;
    const int64_t now1 = now;
    {
        //有效数据
        //std::string cmd = "echo \"10.11.11.1 10 " + convert(now1) + "\" >" + rpingFile;
        //std::cout << cmd << std::endl;
        //SystemUtil::execCmd(cmd);
        ss << "10.11.11.1 10 " + convert(now1) << std::endl;
    }

    //无效数据
    const int64_t now2 = now - 1000 * 70;
    {
        //std::string cmd = "echo \"10.11.11.2 20 " + convert(now2) + "\" >>" + rpingFile;
        //std::cout << cmd << std::endl;
        //SystemUtil::execCmd(cmd);
        ss << "10.11.11.2 20 " + convert(now2) << std::endl;
    }
    //有效数据
    const int64_t now3 = now - 1000 * 40;
    {
        //std::string cmd = "echo \"10.11.11.3    30 " + convert(now3) + "\" >>" + rpingFile;
        //std::cout << cmd << std::endl;
        //SystemUtil::execCmd(cmd);
        ss << "10.11.11.3    30 " + convert(now3) << std::endl;
    }
    EXPECT_TRUE(WriteFileContent(rpingFile, ss.str()));

    std::this_thread::sleep_for(std::chrono::seconds{1});

    //设置配置有效
    HpcClusterItem hpcClusterItem;
    hpcClusterItem.isValid = true;
    SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);
    pShared->Init(5, 6,
                  (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data3").string(),
                  (TEST_CONF_PATH / "conf" / "RDMA" / "rping.data").string(),
                  "exit 1");
    for (int i = 1; i < 40; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
        string data;
        CollectData collectData;
        int ret = pShared->Collect(data);
        if (i % 5 != 0 && i % 6 != 0) {
            EXPECT_EQ(0, ret);
        } else if ((i % 5) == 0 && (i % 6) != 0) {
            //rdma和rping数据
            ModuleData::convertStringToCollectData(data, collectData);
            EXPECT_EQ(collectData.dataVector.size(), size_t(5));
        } else if ((i % 5) != 0 && (i % 6) == 0) {
            //qos数据
            ModuleData::convertStringToCollectData(data, collectData);
            EXPECT_EQ(collectData.dataVector.size(), size_t(1));
        } else {
            //全部数据
            ModuleData::convertStringToCollectData(data, collectData);
            EXPECT_EQ(collectData.dataVector.size(), size_t(6));
        }
    }
#endif
}

TEST_F(Cms_RdmaCollectTest, rmda) {
    // prepare for rdma collect
    HpcClusterItem hpcClusterItem;
    hpcClusterItem.isValid = true;
    SingletonTaskManager::Instance()->SetHpcClusterItem(hpcClusterItem);

    EXPECT_EQ(-1, cloudMonitor_rdma_collect(nullptr, nullptr));

    Config *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    auto *pConfig = SingletonConfig::Instance();
    pConfig->Set("cms.rdma.path", (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data3").string());
    pConfig->Set("cms.rping.path", (TEST_CONF_PATH / "conf" / "RDMA" / "rping.data").string());
    pConfig->Set("cms.qos.cmd", "exit 1");
    auto *handler = cloudMonitor_rdma_init("");
    EXPECT_NE(nullptr, handler);
    defer(cloudMonitor_rdma_exit(handler));
#ifndef WIN32
    for (int i = 1; i <= 60; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds{5});
        char *data = nullptr;
        int ret = cloudMonitor_rdma_collect(handler, &data);
        if (i % 15 != 0) {
            EXPECT_EQ(ret, 0);
        } else {
            CollectData collectData;
            ModuleData::convertStringToCollectData(data, collectData);
            EXPECT_EQ(collectData.moduleName, pShared->mModuleName);
            // collectData.print();
        }
    }
#endif
}

TEST_F(Cms_RdmaCollectTest, ParseData6) {
    pShared->mRdmaFile = (TEST_CONF_PATH / "conf" / "RDMA" / "rdma.data6").string();
    pShared->mRdmaCount = 15;
    pShared->UpdateRdmaData();
    EXPECT_EQ(pShared->mRdmaDataMap[0].interfaceName, "bond0");
    EXPECT_EQ(pShared->mRdmaDataMap[1].interfaceName, "eth1");
    EXPECT_EQ(pShared->mRdmaDataMap[2].interfaceName, "eth2");
}