#include <gtest/gtest.h>
#include "core/agent_status.h"

#include "common/Logger.h"
#include "common/Config.h"
#include "common/SystemUtil.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"
#include "common/UnitTestEnv.h"

using namespace common;
using namespace argus;
using namespace std;

class AgentStatusTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(AgentStatusTest,InitAgentStatus)
{
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    AgentStatus status;
    EXPECT_EQ(status.mMetricMap.Size(), size_t(3));
    EXPECT_EQ("resourceCount",status.mMetricMap[AGENT_RESOURCE_STATUS].metricName);
    EXPECT_EQ("coredumpCount",status.mMetricMap[AGENT_COREDUMP_STATUS].metricName);
    EXPECT_EQ("startCount",status.mMetricMap[AGENT_START_STATUS].metricName);
}

void AppendAgentStatus(const fs::path &agentStatus, const string &line) {
    std::fstream fs{ agentStatus.string(), std::ios_base::out | std::ios_base::app };
    if (fs.is_open()) {
        fs << line << std::endl;
        fs.close();
    }
}

void AppendAgentStatus(const fs::path &agentStatus, const string &type, const string &num) {
    std::stringstream ss;
    ss << NowSeconds() << "|" << type << "|" << num;
    AppendAgentStatus(agentStatus, ss.str());
}

TEST_F(AgentStatusTest,LoadAgentStatus1)
{
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    string result;
    auto agentStatus = cfg->getBaseDir() / "agent.status";
    boost::filesystem::remove(agentStatus);
    AppendAgentStatus(agentStatus, "resourceCount","4");
    AppendAgentStatus(agentStatus, "20|coredumpCount|5");
    AppendAgentStatus(agentStatus, "startCount","6");
    AppendAgentStatus(agentStatus, "resourceCount","7");
    AppendAgentStatus(agentStatus, "resourceCount1","8");
    AgentStatus pStatus;
    EXPECT_EQ(pStatus.mMetricMap.Size(), size_t(3));
    EXPECT_EQ(7,pStatus.mMetricMap[AGENT_RESOURCE_STATUS].count);
    EXPECT_EQ(0,pStatus.mMetricMap[AGENT_COREDUMP_STATUS].count);
    EXPECT_EQ(6,pStatus.mMetricMap[AGENT_START_STATUS].count);
    boost::filesystem::remove(agentStatus);
}
TEST_F(AgentStatusTest,LoadAgentStatus2)
{
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    AgentStatus status;
    status.UpdateAgentStatus(AGENT_RESOURCE_STATUS);
    status.UpdateAgentStatus(AGENT_COREDUMP_STATUS);
    status.UpdateAgentStatus(AGENT_START_STATUS);
    status.UpdateAgentStatus(AGENT_START_STATUS);
    status.LoadAgentStatus();
    auto now = std::chrono::Now<std::chrono::seconds>();
    EXPECT_LT(now - status.mMetricMap[AGENT_RESOURCE_STATUS].timestamp, std::chrono::seconds{1});
    EXPECT_EQ(1,status.mMetricMap[AGENT_RESOURCE_STATUS].count);
    EXPECT_LT(now - status.mMetricMap[AGENT_COREDUMP_STATUS].timestamp, std::chrono::seconds{1});
    EXPECT_EQ(1,status.mMetricMap[AGENT_COREDUMP_STATUS].count);
    EXPECT_LT(now - status.mMetricMap[AGENT_START_STATUS].timestamp, std::chrono::seconds{1});
    EXPECT_EQ(2,status.mMetricMap[AGENT_START_STATUS].count);
    boost::filesystem::remove(cfg->getBaseDir() / "agent.status");
}

TEST_F(AgentStatusTest,GetAgentStatus)
{
    Config *cfg = SingletonConfig::Instance();
    cfg->setBaseDir(TEST_CONF_PATH / "conf");
    AgentStatus status;
    string result;
    EXPECT_TRUE(status.GetAgentStatus());
    for(int i=0;i<argus::AgentStatus::maxCount(AGENT_COREDUMP_STATUS)-1;i++)
    {
        status.UpdateAgentStatus(AGENT_COREDUMP_STATUS);
        EXPECT_TRUE(status.GetAgentStatus());
    }
    status.UpdateAgentStatus(AGENT_COREDUMP_STATUS);
    EXPECT_FALSE(status.GetAgentStatus());
    status.LoadAgentStatus();
    EXPECT_FALSE(status.GetAgentStatus());
    boost::filesystem::remove(cfg->getBaseDir() / "agent.status");
    status.InitAgentStatus();
    status.LoadAgentStatus();
    EXPECT_TRUE(status.GetAgentStatus());
    for(int i=0;i<argus::AgentStatus::maxCount(AGENT_RESOURCE_STATUS)-1;i++)
    {
        status.UpdateAgentStatus(AGENT_RESOURCE_STATUS);
        EXPECT_TRUE(status.GetAgentStatus());
    }
    status.UpdateAgentStatus(AGENT_RESOURCE_STATUS);
    EXPECT_FALSE(status.GetAgentStatus());
    // SystemUtil::execCmd("rm ./conf/agent.status",result);
    boost::filesystem::remove(TEST_CONF_PATH / "conf/agent.status");
    status.InitAgentStatus();
    status.LoadAgentStatus();
    EXPECT_TRUE(status.GetAgentStatus());
    for (int i = 0; i < argus::AgentStatus::maxCount(AGENT_START_STATUS) - 1; i++) {
        status.UpdateAgentStatus(AGENT_START_STATUS);
        EXPECT_TRUE(status.GetAgentStatus());
    }
    status.UpdateAgentStatus(AGENT_START_STATUS);
    EXPECT_TRUE(status.GetAgentStatus());


    fs::path agentStatusFile = cfg->getBaseDir() / "agent.status";
    boost::filesystem::remove(agentStatusFile);
    EXPECT_FALSE(fs::exists(agentStatusFile));
    
    AppendAgentStatus(agentStatusFile, "10|resourceCount|4");
    AppendAgentStatus(agentStatusFile, "20|coredumpCount|5");
    AppendAgentStatus(agentStatusFile, "30|startCount|6");
    AppendAgentStatus(agentStatusFile, "40|resourceCount|7");
    AppendAgentStatus(agentStatusFile, "50|resourceCount1|8");
    status.InitAgentStatus();
    status.LoadAgentStatus();
    EXPECT_TRUE(status.GetAgentStatus());
    boost::filesystem::remove(agentStatusFile);
    EXPECT_FALSE(fs::exists(agentStatusFile));
    
    int64_t now = NowSeconds();
    string nowStr=StringUtils::NumberToString(now);
    //string cmd ="echo '"+nowStr+"|coredumpCount|3'>>./conf/agent.status";
    //SystemUtil::execCmd(cmd,result);
    AppendAgentStatus(agentStatusFile, nowStr + "|coredumpCount|3");
    status.InitAgentStatus();
    status.LoadAgentStatus();
    EXPECT_FALSE(status.GetAgentStatus());
    boost::filesystem::remove(agentStatusFile);
}
