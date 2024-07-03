//
// Created by 韩呈杰 on 2023/7/2.
//
#include "common/ArgusMacros.h"
#if HAVE_SOCKPATH
#if !defined(ENABLE_CLOUD_MONITOR) || defined(ENABLE_FILE_CHANNEL)
#include <gtest/gtest.h>
#include "core/DomainSocketCollect.h"

#include <string>
#include <thread>

#ifndef WIN32
#include <sys/socket.h>
#include <sys/un.h>

#include <apr-1/apr_network_io.h>
#endif

#include "common/Config.h"
#include "core/argus_manager.h"
#include "common/CPoll.h"
#include "common/Logger.h"
#include "common/NetWorker.h"
#include "common/UnitTestEnv.h"
#include "common/FileUtils.h"
#include "core/task_monitor.h"

using namespace std;
using namespace argus;
using namespace common;

// FIXME: can't pass compilation under windows
class DomainSocketCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
        p_shared = new DomainSocketCollect;

        SingletonArgusManager::Instance()->Start(false);
        StartGlobalPoll();
    }

    void TearDown() override {
        // 为保证测试用例间的独立性，该测试用例中使用到的单例应当在测试结束之后删除
        SingletonArgusManager::destroyInstance();

        Delete(p_shared);
    }

    DomainSocketCollect *p_shared = nullptr;
};

int sendData(const string& msg)
{
    string sockPath = SingletonConfig::Instance()->getReceiveDomainSocketPath(); // "/tmp/argus.sock";
    if (!fs::exists(sockPath)) {
        LogError("Can't access socket file [{}].", sockPath);
        return -1;
    }
    LogDebug("sockPath: {}", sockPath);

    NetWorker m_net("DomainSocketCollectUT.sendData");
    m_net.setTimeout(3_s);
    // constexpr auto _3s = std::chrono::seconds{3};
    // if (m_net.socket(AF_UNIX, SOCK_STREAM, 0) != 0 || m_net.setSockOpt(APR_SO_REUSEADDR, 1) != 0 || m_net.setSockTimeOut(_3s) != 0)
    // {
    //     LogError("Create client listener failed");
    //     m_net.clear();
    //     return -1;
    // }
    //
    // // struct sockaddr_un sockaddrun;
    // // sockaddrun.sun_family = AF_UNIX;
    // // strcpy(sockaddrun.sun_path, sockPath.c_str());
    // // int iRetCode = ::connect(m_net.getSock()->socketdes, (sockaddr *)&sockaddrun, sizeof(struct sockaddr_un));
    // int iRetCode = m_net.connect(sockPath.c_str());
    // if (0 != iRetCode)
    // {
    //     LogError("connect to StarAgent failed:{},err:{},errinfo:{}", iRetCode, errno, strerror(errno));
    //     return -1;
    // }
    //
    // iRetCode = m_net.setSockTimeOut(_3s);
    // if (0 != iRetCode)
    // {
    //     LogError("set timeout failed:{},err:{},errinfo:{}", iRetCode, errno, strerror(errno));
    //     return -1;
    // }
    int iRetCode = m_net.connectSockPath(sockPath);
    if (iRetCode != 0) {
        return -2;
    }

    auto sendLen = static_cast<int32_t>(msg.size());
    size_t len = sendLen + 8;
    char *sendBuf = new char[len];
    defer(delete [] sendBuf);
    int32_t type = 1;
    memcpy(sendBuf, &type, 4);
    memcpy(sendBuf + 4, &sendLen, 4);
    memcpy(sendBuf + 8, msg.c_str(), msg.size());
    if (m_net.send(sendBuf, len) != APR_SUCCESS) {
        //m_net.close();
        LogWarn("sendMsg to Sa Error");
        return -3;
    }
    // 等待对方接收
#if defined(__APPLE__) || defined(__FreeBSD__)
    std::this_thread::sleep_for(1_s);
#elif defined(ONE_AGENT)
    std::this_thread::sleep_for(2_s);
#else
    std::this_thread::sleep_for(50_ms);
#endif

    return 0;
}

TEST_F(DomainSocketCollectTest, collectData) {
    Config *cfg = SingletonConfig::Instance();
    string taskFile = (TEST_CONF_PATH / "conf/task/receiveTask.json").string();
    cfg->setReceiveTaskFile(taskFile);
    auto pTask = std::make_shared<TaskMonitor>();
    EXPECT_EQ(1, pTask->LoadReceiveTaskFromFile());

    fs::path writeFile = TEST_CONF_PATH / "conf/task/.localfile";
    fs::remove(writeFile);
    EXPECT_FALSE(fs::exists(writeFile));

    //模拟发送数据到domainsocket
    const string send1 = "senddata1";
    const string send2 = "senddata2";
    EXPECT_EQ(0, sendData(send1));
    EXPECT_EQ(0, sendData(send2));

    std::this_thread::sleep_for(1_s);

    string content = ReadFileContent(writeFile);
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find(send1), string::npos);
    EXPECT_NE(content.find(send2), string::npos);
}
#endif // !defined(ENABLE_CLOUD_MONITOR) || defined(ENABLE_FILE_CHANNEL)
#endif // HAVE_SOCKPATH
