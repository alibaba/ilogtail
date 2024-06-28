//
// Created by 韩呈杰 on 2024/1/12.
//
#include <gtest/gtest.h>
#include "common/HttpServer.h"
#include "common/NetWorker.h"
#include "common/Logger.h"

using namespace common;

TEST(CommonHttpServerTest, ReceiveRequestExceedMaxHeader) {
    HttpServerMsg msg;
    EXPECT_EQ(S_Error, HttpServer::ReceiveRequest(nullptr, msg));

    {
        struct MockNetWorker : public NetWorker {
            MockNetWorker() : NetWorker("") {}

            int recv(char *buf, size_t &len, bool) override {
                for (size_t i = 0; i < len; i++) {
                    buf[i] = 'A';
                }
                return 0;
            }
        };
        MockNetWorker netWorker;
        EXPECT_EQ(S_Incomplete, HttpServer::ReceiveRequest(&netWorker, msg)); // 未达header尾部
    }
    {
        struct MockNetWorker : public NetWorker {
            MockNetWorker() : NetWorker("") {}

            int recv(char *buf, size_t &len, bool) override {
                memcpy(buf, " A B", 4);
                len = 4;
                return 0;
            }
        };
        MockNetWorker netWorker;
        EXPECT_EQ(S_Error, HttpServer::ReceiveRequest(&netWorker, msg)); // header超限
    }
}

TEST(CommonHttpServerTest, ReceiveRequest) {
    HttpServerMsg msg;
    {
        struct MockNetWorker : public NetWorker {
            int count = 0;

            MockNetWorker() : NetWorker("") {}

            int recv(char *buf, size_t &len, bool) override {
                if (count++ == 0) {
                    char szHeader[] = "HTTP/2 200 OK\r\nContent-length: 1\r\n\r\n";
                    len = sizeof(szHeader) - 1;
                    memcpy(buf, szHeader, len);
                    return 0;
                }
                return -1;
            }
        };
        MockNetWorker netWorker;
        EXPECT_EQ(S_Error, HttpServer::ReceiveRequest(&netWorker, msg));
    }
    {
        struct MockNetWorker : public NetWorker {
            MockNetWorker() : NetWorker("") {}

            int recv(char *buf, size_t &len, bool) override {
                len = 1;
                *buf = 'A';
                return 0;
            }
        };
        MockNetWorker netWorker;
        EXPECT_EQ(S_Complete, HttpServer::ReceiveRequest(&netWorker, msg));
        EXPECT_EQ(msg.body, "A");
        EXPECT_FALSE(msg.header.empty());
    }
}

TEST(CommonHttpServerTest, SendResponse01) {
    HttpServerMsg resp;
    resp.body = "A";
    EXPECT_EQ(-1, HttpServer::SendResponse(nullptr, resp));

    struct MockNetWorker : public NetWorker {
        MockNetWorker() : NetWorker("") {}

        int send(const char *buf, size_t &len) override {
            return -1;
        }
    };
    MockNetWorker netWorker;

    EXPECT_EQ(-1, HttpServer::SendResponse(&netWorker, resp));
}

TEST(CommonHttpServerTest, SendResponse02) {
    HttpServerMsg resp;
    resp.body = "A";

    struct MockNetWorker : public NetWorker {
        std::string s;

        MockNetWorker() : NetWorker("") {}

        int send(const char *buf, size_t &len) override {
            s.append(buf, buf + len);
            return 0;
        }
    };
    MockNetWorker netWorker;
    EXPECT_EQ(0, HttpServer::SendResponse(&netWorker, resp));
    EXPECT_EQ(netWorker.s, "A");
}