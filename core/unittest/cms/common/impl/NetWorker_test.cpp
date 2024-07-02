//
// Created by 韩呈杰 on 2023/11/30.
//
#include <gtest/gtest.h>
#include "common/NetWorker.h"

#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#include "common/Random.h"
#include "common/Uuid.h"
#include "common/FileUtils.h"
#include "common/Defer.h"

#include "common/Config.h"

#include <apr-1/apr_errno.h> // apr_status_t

using namespace common;

TEST(CommonNetWorkerTest, Normal) {
    // 此处无限循环用于测试，NetWorker是否存在内存泄漏
    for (size_t i = 0; i < 1; i++) {
        NetWorker netWorker(__FUNCTION__);
        apr_status_t rv = netWorker.connect("www.aliyun.com", 80);
        EXPECT_EQ(APR_SUCCESS, rv);

        rv = netWorker.connect("www.baidu.com", 80);
        EXPECT_EQ(APR_SUCCESS, rv);

        if (1 == (i % 500)) {
            LogInfo("Loop {}", i);
        }
    }
}

TEST(CommonNetWorkerTest, shutdownMultiTimes) {
    NetWorker netWorker(__FUNCTION__);
    apr_status_t rv = netWorker.connect("www.aliyun.com", 80);
    EXPECT_EQ(APR_SUCCESS, rv);

    for (int i = 0; i < 10; i++) {
        apr_status_t ret = netWorker.shutdown();
        // 57: 系统直接返回的错误：Socket is not connected
        EXPECT_TRUE(ret == APR_SUCCESS || ret == 57);
    }
}

TEST(CommonNetWorkerTest, GetAddr) {
// #define MEMORY_LEACK_TEST
#ifdef MEMORY_LEAK_TEST
    auto collector = SystemInformationCollector::New();
    SicProcessMemoryInformation memInfoBegin;
    ASSERT_EQ(SIC_SUCCESS, collector->SicGetProcessMemoryInformation(GetPid(), memInfoBegin));
#endif
    {
        NetWorker netWorker(__FUNCTION__);
        // apr_sockaddr_t *sa = nullptr;
        // ASSERT_EQ(APR_SUCCESS, netWorker.createAddr(&sa, "www.aliyun.com", 80));
        // ASSERT_EQ(APR_SUCCESS, netWorker.socket());
        // ASSERT_EQ(APR_SUCCESS, netWorker.connect(sa));
        ASSERT_EQ(APR_SUCCESS, netWorker.connect("www.aliyun.com", 80));
        //  此处存在着内存泄漏，只能通过apr_pool_(destroy|clear)来释放。
        std::cout << netWorker.getLocalAddr() << ":" << netWorker.getLocalPort()
                  << " -> " << netWorker.getRemoteAddr() << ":" << netWorker.getRemotePort() << std::endl;

#ifdef MEMORY_LEAK_TEST
        int count = 0;
        TimeProfile tp;
        do {
            netWorker.getLocalAddr();
            netWorker.getRemoteAddr();
            count += 2;
        } while (tp.cost(false) < 30_s);
        auto costByMillis = convert(tp.millis(), true);

        {
            SicProcessMemoryInformation memInfoAfter;
            ASSERT_EQ(SIC_SUCCESS, collector->SicGetProcessMemoryInformation(GetPid(), memInfoAfter));
            auto leak = AbsMinus(memInfoAfter.resident, memInfoBegin.resident);
            LogInfo("after execute getSocketAddr {} times in {}ms, memory leak: {} bytes",
                    convert(count, true), costByMillis, convert(leak, true));
        }
#endif
    }

#ifdef MEMORY_LEAK_TEST
    SicProcessMemoryInformation memInfoAfter;
    ASSERT_EQ(SIC_SUCCESS, collector->SicGetProcessMemoryInformation(GetPid(), memInfoAfter));
    auto leak = AbsMinus(memInfoAfter.resident, memInfoBegin.resident);
    LogInfo("after test, memory leak: {} bytes", convert(leak, true));
#endif
}

TEST(CommonNetWorkerTest, OpenIcmp) {
    NetWorker net(__FUNCTION__);
    EXPECT_EQ(0, net.openIcmp(1024));
}

TEST(CommonNetWorkerTest, createTcpSocket) {
    NetWorker net(__FUNCTION__);
    EXPECT_EQ(0, net.createTcpSocket());
}

TEST(CommonNetWorkerTest, bindEmpty) {
    NetWorker net(__FUNCTION__);
    EXPECT_EQ(-1, net.bind(std::string{}, 0));
    EXPECT_EQ(-1, net.bind("", 0));
    EXPECT_EQ(-1, net.bind((const char *)nullptr, 0));
}

// 通过闭包的形式，将引用以传值的方式，传递给匿名函数
TEST(CommonNetWorkerTest, PassRefByValToLambdaThroughClosure) {
    auto f = [](const std::string &s) {
        std::cout << (void *)&s << ": " << s << std::endl;
        return [s]() {
            // s此时是一个传值效果，与『闭包』外的s是完全不同的两个变量了
            const_cast<std::string &>(s) = "world";
            std::cout << (void *)&s << ": " << s << std::endl;
        };
    };

    std::string hello("hello");
    f(hello)();
    std::cout << (void *)&hello << ": " << hello << std::endl;
    EXPECT_EQ(hello, "hello");  // 此处如果失败，则makeSharedSocket需要重新修改scenario参数
}

TEST(CommonNetWorkerTest, connect4) {
    std::string localIP;
    {
        NetWorker net(__FUNCTION__);
        EXPECT_EQ(0, net.connect<TCP>("www.aliyun.com", 443));
        localIP = net.getLocalAddr();
    }

    Random<uint16_t> myRand(32767, 65535);
    int rv = -1;
    // 防止资源占用，重试10次
    for (int i = 0; rv != APR_SUCCESS && i < 10; i++) {
        uint16_t port = myRand();
        NetWorker net2(__FUNCTION__);
        rv = net2.connect<TCP>(localIP, port, "www.aliyun.com", 443);
    }
    EXPECT_EQ(0, rv);
}

TEST(CommonNetWorkerTest, sendWithNoneSock) {
    NetWorker net("test");
    size_t len = 1;
    EXPECT_EQ(-1, net.send(" ", len));
}

TEST(CommonNetWorkerTest, _4connect) {
    class NetWorkerStub : public NetWorker {
    public:
        NetWorkerStub() : NetWorker("stub") {}

        using NetWorker::connect;

        int connect(const char *hostOrIP, uint16_t remotePort, bool tcp, bool reuseSocket) override {
            return 2;
        }
    };
    NetWorkerStub netWorkerStub;
    EXPECT_EQ(2, netWorkerStub.connect(std::string{}, 0, true, false));
}

TEST(CommonNetWorkerTest, ConnectUdp) {
    NetWorker netWorker(__FUNCTION__);
    ASSERT_EQ(APR_SUCCESS, netWorker.connect<UDP>("www.aliyun.com", 80));
}

TEST(CommonNetWorkerTest, close) {
    NetWorker netWorker(__FUNCTION__);
    ASSERT_EQ(APR_SUCCESS, netWorker.connect("www.aliyun.com", 80));

    std::shared_ptr<apr_socket_t> sock = netWorker.m_sock;
    EXPECT_EQ(2, sock.use_count());

    netWorker.close();
    EXPECT_EQ(1, sock.use_count());
}

TEST(CommonNetWorkerTest, Shutdown) {
    NetWorker netWorker(__FUNCTION__);
    ASSERT_EQ(APR_SUCCESS, netWorker.connect("www.aliyun.com", 80));

    EXPECT_EQ(APR_SUCCESS, netWorker.shutdown());
    int rv = netWorker.shutdown();   // 重复释放是安全的
#if defined(__APPLE__) || defined(__FreeBSD__)
    EXPECT_EQ(rv, ENOTCONN);
#else
    EXPECT_EQ(rv, APR_SUCCESS);
#endif
}

#if HAVE_SOCKPATH
TEST(CommonNetWorkerTest, BindSockPath) {
    const std::string sockPath = "/tmp/argus-" + NewUuid() + ".sock";
    WriteFileContent(sockPath, "");
    defer(RemoveFile(sockPath));

    NetWorker net(__FUNCTION__);
    EXPECT_NE(0, net.bindSockPath(sockPath.c_str())); // (98)Address already in use
}
#endif
