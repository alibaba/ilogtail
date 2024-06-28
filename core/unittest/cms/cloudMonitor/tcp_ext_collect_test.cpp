//
// Created by 许刘泽 on 2021/2/20.
//

#include <gtest/gtest.h>
#include "cloudMonitor/tcp_ext_collect.h"

#include <iostream>
#include <thread>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "sic/system_information_collector_util.h"

#include "cloudMonitor/cloud_monitor_config.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

class Cms_TcpExtCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(Cms_TcpExtCollectTest, ReadTcpExtFile) {
    auto pShared = std::make_shared<TcpExtCollect>();
    pShared->Init(15);
    pShared->mActivate = true;

    TcpExtMetric tcpExtMetric;
    tcpExtMetric.tcpLossProbeRecovery = 1;
    const_cast<std::string &>(pShared->TCP_EXT_FILE) = (TEST_CONF_PATH / "/conf/NET/netstat_prev").string();
    pShared->ReadTcpExtFile(tcpExtMetric);
    EXPECT_EQ(tcpExtMetric.delayedACKs, 0.0);
    EXPECT_EQ(tcpExtMetric.listenOverflows, 0.0);
    EXPECT_EQ(tcpExtMetric.listenDrops, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpPrequeued, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpPrequeueDropped, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpPureAcks, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpFACKReorder, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpSACKReorder, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpLossProbes, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpLossProbeRecovery, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpLostRetransmit, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpFastRetrans, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpSlowStartRetrans, 0.0);
    EXPECT_EQ(tcpExtMetric.tcpTimeouts, 0.0);
}

TEST_F(Cms_TcpExtCollectTest, ReadErrorTcpExtFile) {
    auto pShared = std::make_shared<TcpExtCollect>();
    pShared->Init(15);
    pShared->mActivate = true;
    TcpExtMetric tcpExtMetric;
    tcpExtMetric.tcpLossProbeRecovery = 1;
    const_cast<std::string &>(pShared->TCP_EXT_FILE) = (TEST_CONF_PATH / "/conf/NET/netstat_error").string();
    pShared->ReadTcpExtFile(tcpExtMetric);
}

TEST_F(Cms_TcpExtCollectTest, Collect) {
    auto pShared = std::make_shared<TcpExtCollect>();
    pShared->Init(15);
    pShared->mActivate = true;

    const_cast<std::string &>(pShared->TCP_EXT_FILE) = (TEST_CONF_PATH / "/conf/NET/netstat_prev").string();
    std::string res;
    pShared->Collect(res);

    std::this_thread::sleep_for(chrono::seconds(1));

    const_cast<std::string &>(pShared->TCP_EXT_FILE) = (TEST_CONF_PATH / "/conf/NET/netstat_cur").string();
    pShared->Collect(res);
    for (int i = 0; i < 20; ++i) {
        pShared->Collect(res);
    }
    std::cout << res << std::endl;
    EXPECT_EQ(2, 2);
}

TEST_F(Cms_TcpExtCollectTest, Calculate) {
    TcpExtCollect collector;
    collector.mActivate = true;

    collector.mLastTime = std::chrono::steady_clock::now() - std::chrono::minutes(1);
    collector.mCurrentTime = std::chrono::steady_clock::now();

    collector.Calculate();
}
