//
// Created by 韩呈杰 on 2023/7/2.
//
#include <gtest/gtest.h>
#include "common/HardwareInfoProvider.h"

#ifdef WIN32
#	include "common/StringUtils.h"

#	define ToGBK UTF8ToGBK
#else
#	define ToGBK(s) (s)
#endif


using namespace common;

TEST(CommonHardwareInfoProviderTest, getHostHardwareInfo01) {
    HardwareInfoProvider provider;
    EXPECT_FALSE(provider.parseJson("[]"));
}

TEST(CommonHardwareInfoProviderTest, getHostHardwareInfo02) {
    HardwareInfoProvider provider;
    // local和cmdb都为空
    EXPECT_FALSE(provider.parseJson("{}"));
    EXPECT_FALSE(provider.parseJson(R"XX({"local":{}})XX"));
    EXPECT_FALSE(provider.parseJson(R"XX({"cmdb":{}})XX"));
    EXPECT_FALSE(provider.parseJson(R"XX({"cmdb":{},"local":{}})XX"));
}

TEST(CommonHardwareInfoProviderTest, getHostHardwareInfo03) {
    const char *szJson = R"XX({
    "cmdb": {
        "appGroup": "argusagent-hcj-dev",
        "appName": "argusagent",
        "appUseType": "DAILY",
        "cabinetNum": "CK_FOR_ECS_MAPPING_NA63",
        "centerUnit": "CENTER_UNIT.center",
        "city": "张家口市",
        "deviceModel": "ecs_test_vm",
        "hostName": "dev",
        "idcName": "NA63",
        "ip": "1.2.3.4",
        "isVm": true,
        "logicRegion": "NA63",
        "regionName": "cn-zhangjiakou", // 原本是中文，但在windows下解析json失败，只能改为英文
        "roomName": "C1-1.NA63",
        "securityDomain": "ALI_TEST",
        "serviceState": "buffer",
        "sn": "i-123"
    },
    "local": {
        "deviceModel": "Alibaba Cloud ECS",
        "hostName": "argus-dev-linux",
        "ip": "1.2.3.4",
        "ipList": "|1.2.3.4|",
        "manufacturer": "Alibaba Cloud",
        "os": "Linux",
        "osBit": "64",
        "osMainVersion": "7",
        "osVersion": "7.2",
        "sn": "i-123"
    }
})XX";
    HardwareInfoProvider provider;
    ASSERT_TRUE(provider.parseJson(ToGBK(szJson)));

    EXPECT_EQ(provider.hardwareInfo.clientOs, "Linux");
    EXPECT_EQ(provider.hardwareInfo.clientOsVersion, "7.2");
    EXPECT_EQ(provider.hardwareInfo.clientOsBit, "64");
    EXPECT_EQ(provider.hardwareInfo.clientOsMainVersion, "7");
    EXPECT_EQ(provider.hardwareInfo.ipList, "|1.2.3.4|");
    EXPECT_EQ(provider.hardwareInfo.serviceTag, "i-123");
    EXPECT_EQ(provider.hardwareInfo.hostName, "argus-dev-linux");
    EXPECT_EQ(provider.hardwareInfo.mainIp, "1.2.3.4");

    EXPECT_EQ(provider.hardwareInfo.appGroup, "argusagent-hcj-dev");
    EXPECT_EQ(provider.hardwareInfo.appName, "argusagent");
    EXPECT_EQ(provider.hardwareInfo.idcName, "NA63");
    EXPECT_EQ(provider.hardwareInfo.deviceModel, "ecs_test_vm");
    EXPECT_EQ(provider.hardwareInfo.serviceState, "buffer");
    EXPECT_EQ(provider.hardwareInfo.securityDomain, "ALI_TEST");
    EXPECT_TRUE(provider.hardwareInfo.isVm);

    EXPECT_GT(provider.hardwareInfo.timestamp, 0);
}

TEST(CommonHardwareInfoProviderTest, getHostHardwareInfo04) {
    const char *szJson = R"XX({
    "cmdb": {
        "appGroup": "argusagent-hcj-dev",
        "appName": "argusagent",
        "appUseType": "DAILY",
        "cabinetNum": "CK_FOR_ECS_MAPPING_NA63",
        "centerUnit": "CENTER_UNIT.center",
        "city": "张家口市",
        "deviceModel": "ecs_test_vm",
        "hostName": "dev",
        "idcName": "NA63",
        "ip": "1.2.3.4",
        "isVm": true,
        "logicRegion": "NA63",
        "regionName": "cn-zhangjiakou",
        "roomName": "C1-1.NA63",
        "securityDomain": "ALI_TEST",
        "serviceState": "buffer",
        "sn": "i-123"
    },
})XX";
    HardwareInfoProvider provider;
    ASSERT_TRUE(provider.parseJson(ToGBK(szJson)));

    EXPECT_EQ(provider.hardwareInfo.appGroup, "argusagent-hcj-dev");
    EXPECT_EQ(provider.hardwareInfo.appName, "argusagent");
    EXPECT_EQ(provider.hardwareInfo.idcName, "NA63");
    EXPECT_EQ(provider.hardwareInfo.deviceModel, "ecs_test_vm");
    EXPECT_EQ(provider.hardwareInfo.serviceState, "buffer");
    EXPECT_TRUE(provider.hardwareInfo.isVm);
    EXPECT_EQ(provider.hardwareInfo.securityDomain, "ALI_TEST");

    EXPECT_GT(provider.hardwareInfo.timestamp, 0);
}
