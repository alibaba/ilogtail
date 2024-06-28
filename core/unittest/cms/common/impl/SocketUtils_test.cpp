//
// Created by 韩呈杰 on 2023/4/19.
//
#include <gtest/gtest.h>
#include "common/SocketUtils.h"

#ifdef WIN32
#   include <winsock2.h>
#else
#   include <sys/socket.h>
#endif

TEST(CommonSocketUtilsTest, SocketFamilyName) {
    EXPECT_EQ(SocketFamilyName(AF_INET), "AF_INET");
}

TEST(CommonSocketUtilsTest, SocketTypeName) {
    EXPECT_EQ(SocketTypeName(SOCK_STREAM), "SOCK_STREAM");
}

TEST(CommonSocketUtilsTest, SocketProtocolName) {
    EXPECT_EQ(SocketProtocolName(6), "IPPROTO_TCP");
}

TEST(CommonSocketUtilsTest, SocketUnknownFamilyName) {
    EXPECT_EQ(SocketFamilyName(-1), "UNKNOWN(-1)");
}

TEST(CommonSocketUtilsTest, SocketDesc) {
    std::string s = SocketDesc("SocketDesc", AF_INET, SOCK_STREAM, 6);
    std::cout << s << std::endl;
    EXPECT_EQ(s, "SocketDesc(AF_INET, SOCK_STREAM, IPPROTO_TCP)");
}