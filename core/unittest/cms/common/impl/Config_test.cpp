#include <gtest/gtest.h>
#include "common/Config.h"
#include "common/UnitTestEnv.h"

using namespace common;

void Config::Set(const std::string &key, const std::string &v) {
    if (!mPropertiesFile) {
        mPropertiesFile = std::make_shared<common::PropertiesFile>("");
    }
    mPropertiesFile->Set(key, v);
}

/*
agent.memoryLimit=110000000
agent.test1=-4
agent.memoryExceedTimes=0.3
agent.memoryExceedTimes1=-1.3111
agent.selfMonitor.interval=15
#agent.test2=10
agent.testNotNumber=test
*/
TEST(MainConfigTest,getValue)
{
    auto pConfig=std::make_shared<Config>();
    pConfig->loadConfig((TEST_CONF_PATH / "conf/conf.properties").string());
    EXPECT_EQ(pConfig->GetValue<int64_t>("agent.memoryLimit",1),110000000);
    EXPECT_EQ(pConfig->GetValue<int64_t>("agent.test1",1),-4);
    EXPECT_EQ(pConfig->GetValue<double>("agent.memoryExceedTimes",0.0),0.3);
    EXPECT_EQ(pConfig->GetValue<double>("agent.memoryExceedTimes1",0.0),-1.3111);
    EXPECT_EQ(pConfig->GetValue<int64_t>("agent.test2",100),100);
    EXPECT_EQ(pConfig->GetValue("agent.test1",""), "-4");
    EXPECT_EQ(pConfig->GetValue<int64_t>("agent.testNotNumber",1),1);
    EXPECT_EQ(pConfig->GetValue<double>("agent.testNotNumber",0.1),0.1);

    EXPECT_EQ(pConfig->getInnerListenPort(), 8976);

    EXPECT_FALSE(pConfig->GetShennongOn());

    EXPECT_FALSE(pConfig->GetIsTianjiEnv());
    pConfig->SetIsTianjiEnv(true);
    EXPECT_TRUE(pConfig->GetIsTianjiEnv());

#ifdef ENABLE_ALIMONITOR
    EXPECT_FALSE(pConfig->getLocalIps().empty());
    EXPECT_NE(pConfig->getMainIp(), "");
#endif
    EXPECT_FALSE(pConfig->getInnerDomainSocketPath().empty());
    EXPECT_FALSE(pConfig->getEnvPath().empty());

#ifdef ENABLE_ALIMONITOR
    common::ConfigProfile m_config;
    EXPECT_EQ(pConfig->getRcPortBackup(), m_config.rcBackupPort);
#endif
}
