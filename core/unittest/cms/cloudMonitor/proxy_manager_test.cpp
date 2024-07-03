#include <gtest/gtest.h>
#include <iostream>
#include "cloudMonitor/proxy_manager.h"

#include "cloudMonitor/cloud_monitor_config.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/Defer.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

class Cms_ProxyManagerTest : public testing::Test {
};

TEST_F(Cms_ProxyManagerTest, proxyInfo1) {
    auto *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    auto *pConfig = SingletonConfig::Instance();
    pConfig->Set("http.proxy.host", "www.baidu.com");
    pConfig->Set("cms.agent.hosts", "http://www.baidu.com/helloworld");
    pConfig->Set("http.proxy.user", "user");
    ProxyManager pManager;
    // EXPECT_EQ(pManager.mProxyInfos.size(), pManager.DefaultConfigLen + 1);
    EXPECT_TRUE(pManager.mProxyInfos.front().user.empty());
    EXPECT_EQ(pManager.mHeartbeatUrl, "http://www.baidu.com/helloworld");
    EXPECT_EQ(pManager.mHeartbeatHost, "www.baidu.com");
}

TEST_F(Cms_ProxyManagerTest, proxyInfo2) {
    auto *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    auto *pConfig = SingletonConfig::Instance();
    pConfig->Set("http.proxy.host", "www.baidu.com");
    pConfig->Set("cms.agent.hosts", "http://www.baidu.com/helloworld");
    pConfig->Set("http.proxy.user", "user");
    pConfig->Set("http.proxy.password", "password");
    ProxyManager pManager;
    // EXPECT_EQ(pManager.mProxyInfos.size(), pManager.DefaultConfigLen + 1);
    EXPECT_EQ(pManager.mProxyInfos.front().user, "user");
    EXPECT_EQ(pManager.mProxyInfos.front().password, "password");
    EXPECT_EQ(pManager.mHeartbeatUrl, "http://www.baidu.com/helloworld");
    EXPECT_EQ(pManager.mHeartbeatHost, "www.baidu.com");
}

TEST_F(Cms_ProxyManagerTest, proxyInfo3) {
    auto *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    auto *pConfig = SingletonConfig::Instance();
    pConfig->Set("http.proxy.host", "www.baidu.com\r");
    pConfig->Set("cms.agent.hosts", "http://www.baidu.com\r");
    pConfig->Set("http.proxy.user", "user");
    ProxyManager pManager;
    // EXPECT_EQ(pManager.mProxyInfos.size(), pManager.DefaultConfigLen + 1);
    EXPECT_EQ(pManager.mProxyInfos.front().user.size(), size_t(0));
    EXPECT_EQ(pManager.mHeartbeatUrl, "http://www.baidu.com");
    EXPECT_EQ(pManager.mHeartbeatHost, "www.baidu.com");
}

TEST_F(Cms_ProxyManagerTest, proxyInfo4) {
    ProxyInfo proxyInfo;
    EXPECT_TRUE(proxyInfo.proxyUrl().empty());
    proxyInfo.url = "www.yun.com";
    EXPECT_EQ(proxyInfo.proxyUrl(), proxyInfo.url);
    proxyInfo.scheme = "https";
    EXPECT_EQ(proxyInfo.proxyUrl(), "https://www.yun.com");
}

TEST_F(Cms_ProxyManagerTest, ReadAk1) {
    auto *old = SingletonConfig::swap(newCmsConfig());
    defer(delete SingletonConfig::swap(old));

    auto *pConfig = SingletonConfig::Instance();
    pConfig->Set("cms.agent.accesskey", "accesskey");
    pConfig->Set("cms.agent.secretkey", "secretkey");
    ProxyManager pManager;
    pManager.ReadAk();
    EXPECT_EQ(pManager.mAccessKeyId, "accesskey");
    EXPECT_EQ(pManager.mAccessSecret, "secretkey");
    pConfig->Set("cms.agent.accesskey", "");
    pConfig->Set("cms.agent.secretkey", "");
}

TEST_F(Cms_ProxyManagerTest, ReadAk2) {
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH / "conf/cloudMonitor/");
    ProxyManager pManager;
    pManager.ReadAk();
    EXPECT_EQ(pManager.mAccessKeyId, "BH6zgGdmQVs");
    EXPECT_EQ(pManager.mAccessSecret, "s1KN90KfE1sRPKE6MCpGnw");
    SingletonConfig::Instance()->setBaseDir(oldBase);
}

TEST_F(Cms_ProxyManagerTest, ReadAk3) {
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    defer(SingletonConfig::Instance()->setBaseDir(oldBase));

    SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH / "conf/cloudMonitor/");

    boost::filesystem::rename(TEST_CONF_PATH / "conf/cloudMonitor/local_data/conf/accesskey.properties",
                              TEST_CONF_PATH / "conf/cloudMonitor/local_data/conf/accesskey.properties1");
    ProxyManager pManager;
    pManager.ReadAk();
    EXPECT_EQ(pManager.mAccessKeyId, "BH6zgGdmQVs11");
    EXPECT_EQ(pManager.mAccessSecret, "s1KN90KfE1sRPKE6MCpGnw22");

    boost::filesystem::rename(TEST_CONF_PATH / "conf/cloudMonitor/local_data/conf/accesskey.properties1",
                              TEST_CONF_PATH / "conf/cloudMonitor/local_data/conf/accesskey.properties");
}

TEST_F(Cms_ProxyManagerTest, GetSerialNumber1) {
    ProxyManager pManager;
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    SingletonConfig::Instance()->setBaseDir((TEST_CONF_PATH / "conf/cloudMonitor/local_data/").string());

    pManager.mSerialNumber = pManager.GetSerialNumber();
    cout << "mSerialNumberFrom=" << pManager.mSerialNumberFrom << endl;
    cout << "mSerialNumber=" << pManager.mSerialNumber << endl;
    SingletonConfig::Instance()->setBaseDir(oldBase);
}

TEST_F(Cms_ProxyManagerTest, GetSerialNumber2) {
    ProxyManager pManager;
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH / "conf/cloudMonitor/local_data/");
    pManager.ReadAk();
    pManager.mSerialNumber = pManager.GetSerialNumber();
    cout << "mSerialNumberFrom=" << pManager.mSerialNumberFrom << endl;
    cout << "mSerialNumber=" << pManager.mSerialNumber << endl;
    SingletonConfig::Instance()->setBaseDir(oldBase);
}

TEST_F(Cms_ProxyManagerTest, Init) {
    ProxyManager pManager;
    auto oldBase = SingletonConfig::Instance()->getBaseDir();
    SingletonConfig::Instance()->setBaseDir(TEST_CONF_PATH / "conf/cloudMonitor/local_data/");
    pManager.Init();
    SingletonConfig::Instance()->setBaseDir(oldBase);
}

TEST_F(Cms_ProxyManagerTest, InitWithHttpProxy) {
    auto *pConfig = SingletonConfig::Instance();
    // http proxy
    {
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithHttpProxy.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(proxyManager.mIsAuto, false);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "http");
        EXPECT_TRUE(proxyManager.mProxyInfo.schemeVersion.empty());
        EXPECT_EQ(proxyManager.mProxyInfo.url, "127.0.0.1:3128");
    }
    {
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithHttp2Proxy.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(2, 2);
        EXPECT_EQ(proxyManager.mIsAuto, false);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "https");
        EXPECT_EQ(proxyManager.mProxyInfo.schemeVersion, "2");
        EXPECT_EQ(proxyManager.mProxyInfo.url, "127.0.0.1:3128");
    }
    {
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithHttpProxyNoPort.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(2, 2);
        EXPECT_TRUE(proxyManager.mIsAuto);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "http");
        EXPECT_TRUE(proxyManager.mProxyInfo.schemeVersion.empty());
        EXPECT_EQ(proxyManager.mProxyInfo.url, "127.0.0.1");
    }
}

TEST_F(Cms_ProxyManagerTest, InitWithSocks5Proxy) {
    auto *pConfig = SingletonConfig::Instance();
    {
        // socks5 proxy
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5Proxy.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(2, 2);
        EXPECT_EQ(proxyManager.mIsAuto, false);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "socks5");
        EXPECT_TRUE(proxyManager.mProxyInfo.schemeVersion.empty());
        EXPECT_EQ(proxyManager.mProxyInfo.url, "127.0.0.1:1080");
    }
    {
        // mixed proxy -> should use socks5h
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithSocks5hProxy.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(2, 2);
        EXPECT_EQ(proxyManager.mIsAuto, false);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "socks5h");
        EXPECT_TRUE(proxyManager.mProxyInfo.schemeVersion.empty());
        EXPECT_EQ(proxyManager.mProxyInfo.url, "127.0.0.1:1080");
    }
    {
        // mixed proxy -> should use socks5h
        pConfig->loadConfig((TEST_CONF_PATH / "conf/cloudMonitor/cloudMonitorWithUnixDomainSocks5hProxy.conf").string());
        ProxyManager proxyManager;
        EXPECT_EQ(2, 2);
        EXPECT_EQ(proxyManager.mIsAuto, false);
        EXPECT_EQ(proxyManager.mProxyInfo.scheme, "socks5h");
        EXPECT_TRUE(proxyManager.mProxyInfo.schemeVersion.empty());
        EXPECT_EQ(proxyManager.mProxyInfo.url, "localhost/tmp/argus.sock");
    }
}

TEST_F(Cms_ProxyManagerTest, DetectFromProxyInfos) {
    ProxyManager pm;
    // pm.Init();
    pm.mSerialNumber = pm.GetSerialNumber();
    ProxyInfo proxyInfo = pm.DetectFromProxyInfos();
    std::cout << "regionId: " << proxyInfo.regionId << std::endl;
    EXPECT_FALSE(proxyInfo.regionId.empty());
}

TEST_F(Cms_ProxyManagerTest, GetSerialNumberFromEcsAssist) {
    EXPECT_TRUE(ProxyManager::GetSerialNumberFromEcsAssist().empty());
    std::string path = (TEST_CONF_PATH / "conf/cloudMonitor/machine_id.txt").string();
    EXPECT_EQ(ProxyManager::GetSerialNumberFromEcsAssist(path), "c0d3c822-3efc-45cb-a0e8-d449a398be79");
}

TEST_F(Cms_ProxyManagerTest, GetSerialNumberFromOS) {
    const std::string sn = ProxyManager::GetSerialNumberFromOS();
    LogInfo("Serial Number: <{}>", sn);
#ifndef ONE_AGENT
    // OneAgent开发环境下可能没有dmidecode,此时就不必判断是否为空了
    EXPECT_FALSE(sn.empty());
#endif
}
