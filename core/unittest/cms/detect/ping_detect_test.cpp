#include <gtest/gtest.h>
#include "detect/ping_detect.h"
#include "detect/protocol_adt.h"

#include "common/Config.h"
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/NetWorker.h"
#include "common/Uuid.h"
#include "common/Common.h" // IPv4String
#include "common/Defer.h"

using namespace std;
using namespace common;

class Detect_PingDetectTest : public testing::Test {
};

TEST_F(Detect_PingDetectTest, test1) {
    PingDetect ping("www.baidu.com", "testID");
    EXPECT_TRUE(ping.Init(false));

    for (int i = 0; i < 3; i++) {
        ASSERT_TRUE(ping.PingSend());
        int rv = -3;
        // 重试两次
        for (int tryCount = 0; tryCount < 3 && rv == -3; tryCount++) {
            rv = ping.readEvent();
        }
        EXPECT_EQ(0, rv);
    }
}

TEST_F(Detect_PingDetectTest, GetIpString) {
    uint32_t num = 0x0100007F;
    std::string strIP = IPv4String(num);
    LogInfo("0x{:08X} => {}", num, strIP);
    EXPECT_EQ(strIP, "127.0.0.1");
}

TEST_F(Detect_PingDetectTest, PingSendFail01) {
    PingDetect pingDetect("", "");
    EXPECT_EQ(nullptr, pingDetect.mDestSockaddr);
    EXPECT_FALSE(pingDetect.PingSend());
}

TEST_F(Detect_PingDetectTest, PingSendFail02) {
    PingDetect pingDetect("www.aliyun.com", "");
    EXPECT_TRUE(pingDetect.Init(false));
    EXPECT_NE(nullptr, pingDetect.mDestSockaddr);

    struct NetWorkerStub : public NetWorker {
        NetWorkerStub() : NetWorker(__FUNCTION__) {}

        int sendTo(apr_sockaddr_t *where, int32_t flags, const char *buf, size_t &len) override {
            return -1;
        }
    } netWorkerStub;
    pingDetect.mSock = std::make_shared<NetWorkerStub>();

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    EXPECT_FALSE(pingDetect.PingSend());

    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, "send icmp msg error"));
}

TEST_F(Detect_PingDetectTest, PingReceiveFail01) {
    PingDetect pingDetect("www.aliyun.com", "");
    EXPECT_TRUE(pingDetect.Init(false));
    EXPECT_NE(nullptr, pingDetect.mDestSockaddr);

    struct NetWorkerStub : public NetWorker {
        NetWorkerStub() : NetWorker(__FUNCTION__) {}

        size_t mLen = 0;
        int mRecvRet = 0;

        int recv(char *buf, size_t &len, bool) override {
            len = mLen;
            return mRecvRet;
        }
    } netWorkerStub;
    pingDetect.mSock = std::make_shared<NetWorkerStub>();

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));
    {
        EXPECT_FALSE(pingDetect.PingReceive());
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "ret: 0(Success),  recvLen: 0"));
    }
}

TEST_F(Detect_PingDetectTest, readEventFail) {
    struct PingDetectStub : PingDetect {
        PingDetectStub(const std::string &host, const std::string &taskId) : PingDetect(host, taskId) {
        }

        bool mPingReceiveReturn = true;

        bool PingReceive() override {
            return mPingReceiveReturn;
        }
    } pingDetect("www.aliyun.com", "ping-" + NewUuid().substr(0, 8));

    pingDetect.mPingResult.receiveEchoPackage = false;
    pingDetect.mPingResult.lastScheduleTime = std::chrono::steady_clock::now() - 24_h;
    EXPECT_EQ(-1, pingDetect.readEvent()); // 超时

    pingDetect.mPingResult.receiveEchoPackage = true;
    EXPECT_EQ(-2, pingDetect.readEvent());
}

TEST_F(Detect_PingDetectTest, IcmpUnPack) {
    PingDetect pingDetect("www.aliyun.com", "ping-" + NewUuid().substr(0, 8));
    pingDetect.mSeq = 1;
    pingDetect.mUuid = 2;

    char buf[sizeof(struct ip) + sizeof(struct icmp)] = {0};
    ip *ipHdr = (struct ip *) buf;
    ipHdr->ip_hl = sizeof(struct ip) / 4;
    icmp *icmpHdr = (struct icmp *) (buf + sizeof(struct ip));
    icmpHdr->icmp_type = ICMP_ECHOREPLY;
    int pid = 1234;
    icmpHdr->icmp_id = pid;
    icmpHdr->icmp_seq = 1;
    icmpHdr->data[0] = pingDetect.mUuid + 1;

    struct {
        size_t size;
        const char *expect;
    } testCases[] = {
            {sizeof(struct ip), "invalid icmp packet"},
            {sizeof(buf),       "uuid is not right"},
    };

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    for (const auto &entry: testCases) {
        EXPECT_FALSE(pingDetect.IcmpUnPack(pid, buf, entry.size));
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, entry.expect));
    }
}