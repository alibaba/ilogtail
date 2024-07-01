//
// Created by 许刘泽 on 2022/2/11.
//
#include <gtest/gtest.h>
#include "cloudMonitor/tcp_collect.h"
#include "cloudMonitor/cloud_monitor_config.h"

#include <iostream>
#include <thread>

#include "common/Config.h"
#include "common/UnitTestEnv.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

class Cms_TcpCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

#ifndef WIN32
TEST_F(Cms_TcpCollectTest, ReadSnmpFile) {
    TcpCollect tcpCollector;
    tcpCollector.Init(15);
    TcpMetric tcpMetric;
    string snmpPath = (TEST_CONF_PATH / "conf/NET/snmp").string();
    cloudMonitor::TcpCollect::ReadSnmpFile(tcpMetric, snmpPath);
    EXPECT_EQ(tcpMetric.activeOpens, 25995309);
    EXPECT_EQ(tcpMetric.passiveOpens, 6469304);
    EXPECT_EQ(tcpMetric.inSegs, 2000398789);
    EXPECT_EQ(tcpMetric.outSegs, 2085613524);
    EXPECT_EQ(tcpMetric.estabResets, 4537167);
    EXPECT_EQ(tcpMetric.attemptFails, 6155847);
    EXPECT_EQ(tcpMetric.retransSegs, 2369617);
    EXPECT_EQ(tcpMetric.inErrs, 3832);
    EXPECT_EQ(tcpMetric.outRsts, 9748214);
    EXPECT_EQ(tcpMetric.currEstab, 168);
    EXPECT_EQ(tcpMetric.retransRate, 0.0);
    snmpPath = (TEST_CONF_PATH / "conf/NET/snmp2").string();
    cloudMonitor::TcpCollect::ReadSnmpFile(tcpMetric, snmpPath);
    EXPECT_EQ(tcpMetric.activeOpens, 25995309 + 15);
    EXPECT_EQ(tcpMetric.passiveOpens, 6469304 + 15);
    EXPECT_EQ(tcpMetric.inSegs, 2000398789 + 15);
    EXPECT_EQ(tcpMetric.outSegs, 2085613524 + 15);
    EXPECT_EQ(tcpMetric.estabResets, 4537167 + 15);
    EXPECT_EQ(tcpMetric.attemptFails, 6155847 + 15);
    EXPECT_EQ(tcpMetric.retransSegs, 2369617 + 15);
    EXPECT_EQ(tcpMetric.inErrs, 3832 + 15);
    EXPECT_EQ(tcpMetric.outRsts, 9748214 + 15);
    EXPECT_EQ(tcpMetric.currEstab, 168);
    EXPECT_EQ(tcpMetric.retransRate, 0.0);
}
#endif

TEST_F(Cms_TcpCollectTest, Collect) {
    TcpCollect tcpCollector;
    tcpCollector.Init(15);
    tcpCollector.mActivate = true;
    const_cast<std::string &>(tcpCollector.SNMP_FILE) = (TEST_CONF_PATH / "conf/NET/snmp").string();
    std::string res;
    tcpCollector.Collect(res);
    std::this_thread::sleep_for(std::chrono::seconds{1});
    const_cast<std::string &>(tcpCollector.SNMP_FILE) = (TEST_CONF_PATH / "conf/NET/snmp2").string();
    for (int i = 0; i < 20; ++i) {
        tcpCollector.Collect(res);
    }
    std::cout << res << std::endl;
    EXPECT_EQ(1, 1);
}
