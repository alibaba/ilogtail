//
// Created by 韩呈杰 on 2024/2/4.
//
#include <gtest/gtest.h>
#include "core/DomainSocketClient.h"
#include "common/TLVHandler.h"
#include "common/NetWorkerInProcess.h"
#include "common/Logger.h"
#include "common/ScopeGuard.h"
#include "common/StringUtils.h"

using namespace argus;
using namespace common;

TEST(CoreDomainSocketClientTest, ReadDSPacketHdr01) {
    DomainSocketClient client(nullptr, nullptr);
    EXPECT_EQ(nullptr, client.getSock());
    EXPECT_EQ(S_Error, client.ReadDSPacketHdr());
    EXPECT_EQ(S_Error, client.ReadDSPacketData());

    client.m_net = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    client.m_package.mRecvSize = MSG_HDR_LEN + 1;
    EXPECT_EQ(S_Complete, client.ReadDSPacketHdr());
}

TEST(CoreDomainSocketClientTest, ReadDSPacketHdr02) {
    auto server = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    char buf[MSG_HDR_LEN - 1];
    size_t size = sizeof(buf);
    server->send(buf, size);
    EXPECT_EQ(size, sizeof(buf));

    DomainSocketClient client(server->accept(), nullptr);
    EXPECT_EQ(S_Incomplete, client.ReadDSPacketHdr());
}

TEST(CoreDomainSocketClientTest, ReadDSPacketHdr03) {
    auto server = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    char buf[MSG_HDR_LEN] = {0};
    size_t size = sizeof(buf);
    server->send(buf, size);
    EXPECT_EQ(size, sizeof(buf));

    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    DomainSocketClient client(server->accept(), nullptr);
    EXPECT_EQ(S_Error, client.ReadDSPacketHdr());
    std::string cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, "too large"));
}

TEST(CoreDomainSocketClientTest, ReadDSPacketData01) {
    DomainSocketClient client(nullptr, nullptr);
    EXPECT_EQ(nullptr, client.getSock());
    EXPECT_EQ(S_Error, client.ReadDSPacketData());
}

// read返回0
TEST(CoreDomainSocketClientTest, ReadDSPacketData02) {
    auto server = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    size_t len = 0;
    server->send("", len);

    DomainSocketClient client(server->accept(), nullptr);
    client.m_package.mRecvSize = MSG_HDR_LEN;
    client.m_package.mPacket.resize(100);
    EXPECT_EQ(S_Error, client.ReadDSPacketData());
}

// m_net未就绪
TEST(CoreDomainSocketClientTest, readEventError) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    auto net = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    net->valid = false;
    auto client = std::make_shared<DomainSocketClient>(net, nullptr);
    EXPECT_FALSE(client->m_net->isValid());
    client->m_package.mRecvSize = MSG_HDR_LEN;
    client->m_package.mPacket.resize(100);
    EXPECT_EQ(-1, client->readEvent(client));
    std::string cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, "client failed"));
}

TEST(CoreDomainSocketClientTest, readEventInComplete) {
    auto server = std::make_shared<InProcessNetWorker>(__FUNCTION__);
    size_t len = 1;
    server->send("a", len);

    auto client = std::make_shared<DomainSocketClient>(server->accept(), nullptr);
    EXPECT_TRUE(client->m_net->isValid());
    client->m_package.mRecvSize = MSG_HDR_LEN;
    client->m_package.mPacket.resize(100);
    EXPECT_EQ(1, client->readEvent(client));
}
