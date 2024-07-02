//
// Created by 韩呈杰 on 2024/1/12.
//
#include <gtest/gtest.h>
#include "common/TLVHandler.h"
#include "common/NetWorker.h"

#include <vector>

using namespace common;

TEST(CommonTLVPackageTest, GetSetType) {
    TLVPackage package;
    EXPECT_EQ(T_U8Json, package.getType());

    package.setType(T_Binary);
    EXPECT_EQ(T_Binary, package.getType());
}

TEST(CommonTLVPackageTest, GetSetValue) {
    TLVPackage package;
    EXPECT_TRUE(package.getValue().empty());

    package.setValue("hello");
    EXPECT_EQ("hello", package.getValue());

    package.setValue("world", 5);
    EXPECT_EQ("world", package.getValue());
}

TEST(CommonTLVPackageTest, resetSendLen) {
    TLVPackage package;
    package.m_sendLen = 1;
    package.resetSendLen();
    EXPECT_EQ(decltype(package.m_sendLen)(0), package.m_sendLen);
}

TEST(CommonTLVPackageTest, serialize) {
    TLVPackage package;
    package.setType(T_PB_Ext);
    package.setValue("hello");

    const char szExpect[] = "\0\3\0\0\0\5hello";
    EXPECT_EQ(sizeof(szExpect), size_t(12));
    std::string expect = std::string(szExpect, sizeof(szExpect) - 1);
    EXPECT_EQ(expect.size(), sizeof(szExpect) - 1);
    EXPECT_EQ(expect, package.serialize());

    TLVPackage package2;
    EXPECT_TRUE(package2.deserialize(expect));
    EXPECT_EQ(package2.getValue(), "hello");
    EXPECT_EQ(package2.getType(), T_PB_Ext);
}

TEST(CommonTLVPackageTest, deserialize) {
    TLVPackage package;
    EXPECT_FALSE(package.deserialize(""));
    const char txt[] = "\0\0\0\0\0\5";
    EXPECT_FALSE(package.deserialize(std::string(txt, sizeof(txt) - 1)));
}

struct MockNetWorker : public NetWorker {
    std::vector<char> data;
    size_t rd = 0;

    MockNetWorker() : NetWorker("MockNetWorker") {}

    int send(const char *buf, size_t &len) override {
        this->data.insert(data.end(), buf, buf + len);
        return 0;
    }

    int recv(char *buf, size_t &len, bool) override {
        size_t size = data.size() - rd;
        if (size < len) {
            len = size;
        }
        memcpy(buf, &data[rd], len);
        rd += len;
        return 0;
    }

    std::string str() const {
        return std::string(&data[0], &data[0] + data.size());
    }
};


TEST(CommonTLVHandlerTest, recvPackage01) {
    TLVHandler handler;
    TLVPackage package;
    EXPECT_EQ(S_Error, handler.recvPackage(nullptr, package));
}

TEST(CommonTLVHandlerTest, recvPackage02) {
    TLVHandler handler;
    TLVPackage package;

    struct MockNetWorker1 : public MockNetWorker {
        int recv(char *buf, size_t &len, bool) override {
            len = 0;
            return -1;
        }
    } netWorker;
    EXPECT_EQ(S_Error, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage03) {
    TLVHandler handler;
    TLVPackage package;

    struct MockNetWorker1 : public MockNetWorker {
        int recv(char *buf, size_t &len, bool) override {
            len = 1;
            return 0;
        }
    } netWorker;
    EXPECT_EQ(S_Incomplete, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage04) {
    TLVHandler handler;
    TLVPackage package;
    MockNetWorker netWorker;
    const char szPackage[] = "\0\0\0\0\0\0";
    netWorker.data.assign(szPackage, szPackage + sizeof(szPackage) - 1);
    EXPECT_EQ(S_Complete, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage05) {
    TLVHandler handler;
    TLVPackage package;
    package.m_recvdLen = package.m_totalLen = TLVHandler::MAX_RECV_LEN + 1;
    MockNetWorker netWorker;
    EXPECT_EQ(S_Error, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage06) {
    TLVHandler handler;
    TLVPackage package;
    package.m_recvdLen = TL_LEN;
    package.m_totalLen = TL_LEN + 6;

    struct MockNetWorker1 : public MockNetWorker {
        int recv(char *buf, size_t &len, bool) override {
            len = 0;
            return -1;
        }
    } netWorker;
    EXPECT_EQ(S_Error, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage07) {
    TLVHandler handler;
    TLVPackage package;
    package.m_recvdLen = TL_LEN;
    package.m_totalLen = TL_LEN + 6;

    struct MockNetWorker1 : public MockNetWorker {
        int recv(char *buf, size_t &len, bool) override {
            len = 1;
            return 0;
        }
    } netWorker;
    EXPECT_EQ(S_Incomplete, handler.recvPackage(&netWorker, package));
}

TEST(CommonTLVHandlerTest, recvPackage08) {
    char szPackage[] = "\0\1\0\0\0\5hello";
    for (int i = 0; i < 5; i++) {
        szPackage[1] = (char)i;
        MockNetWorker netWorker;
        netWorker.data.assign(szPackage, szPackage + sizeof(szPackage) - 1);

        TLVHandler handler;
        TLVPackage package;
        EXPECT_EQ(S_Complete, handler.recvPackage(&netWorker, package));
        EXPECT_EQ(package.m_totalLen, sizeof(szPackage) - 1);
        EXPECT_EQ(package.getValue(), "hello");
    }
}

TEST(CommonTLVHandlerTest, sendPackage01) {
    TLVHandler handler;
    TLVPackage package;
    EXPECT_EQ(-1, handler.sendPackage(nullptr, package));
}

TEST(CommonTLVHandlerTest, sendPackage02) {
    struct MockNetWorker1 : public MockNetWorker {
        int send(const char *buf, size_t &len) override {
            len = 0;
            return -2;
        }
    } netWorker;

    TLVHandler handler;
    TLVPackage package;
    EXPECT_EQ(-2, handler.sendPackage(&netWorker, package));
}

template<typename TNetWorker>
static void testSendOK() {
    char szPackage[] = "\0\1\0\0\0\5hello";
    for (int i = 0; i < 5; i++) {
        TLVPackage package;
        package.setValue("hello");
        package.setType((ProtoType)i);
        szPackage[1] = (char)i;

        TNetWorker netWorker;
        TLVHandler handler;
        EXPECT_EQ(0, handler.sendPackage(&netWorker, package));
        EXPECT_EQ(package.m_sendLen, decltype(package.m_sendLen)(0));
        EXPECT_EQ(std::string(szPackage, szPackage + sizeof(szPackage) - 1), netWorker.str());
    }
}

TEST(CommonTLVHandlerTest, sendPackage03) {
    testSendOK<MockNetWorker>();
}

TEST(CommonTLVHandlerTest, sendPackage04) {
    struct MockNetWorker1: public MockNetWorker{
        // 一次只发一个字节
        int send(const char *buf, size_t &len) override {
            if (len > 0) {
                len = 1;
                this->data.push_back(buf[0]);
            }
            return 0;
        }
    };
    testSendOK<MockNetWorker1>();
}