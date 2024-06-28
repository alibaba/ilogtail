//
// Created by 韩呈杰 on 2023/10/24.
//
#include <gtest/gtest.h>
#include "common/NetTool.h"
#include "common/Logger.h"

using namespace IP;
using namespace common;

TEST(NetToolTest, IsIPv6) {
    EXPECT_FALSE(IsIPv6(""));
    EXPECT_FALSE(IsIPv6("1.1.1.1"));
    EXPECT_TRUE(IsIPv6("::1"));
    EXPECT_TRUE(IsIPv6("1050:0:0:0:5:600:300c:326b"));
    EXPECT_TRUE(IsIPv6("ff06::c3"));
}

TEST(NetToolTest, IsGlobalUniCast) {
    EXPECT_FALSE(IsGlobalUniCast(""));

    EXPECT_TRUE(IsGlobalUniCastV4("1.1.1.1"));

    struct {
        std::string s;
        bool ok;
        bool failed;
        bool isIPv6;
    } testCases[] = {
            {"",                   false, true,  false},  // empty
            {"0.0.0.0",            false, false, false},  // ipv4zero
            {"1.1.1.1",            true,  false, false},
            {"::",                 false, false, true},   // ipv6 unspecified
            {"ff01::1",            false, false, true},   // IPv6interfacelocalallnodes
            {"127.0.0.1",          false, false, false},  // ipv4 loopback
            {"127.255.255.254",    false, false, false},
            {"128.1.2.3",          true,  false, false},
            {"::1",                false, false, true},   // ipv6 loopback
            {"ff02::2",            false, false, true},   // IPv6linklocalallrouters
            {"224.0.0.0",          false, false, false},  // ipv4 multicast
            {"239.0.0.0",          false, false, false},
            {"240.0.0.0",          true,  false, false},
            {"ff02::01",           false, false, true},   // IPv6linklocalallrouters
            {"ff05::",             false, false, true},  // IPv6 multicast
            {"fe80::",             false, false, true},  // IPv6 link local
            {"255.1.0.0",          true,  false, false},
            {"255.2.0.0",          true,  false, false},
            {"232.0.0.0",          false, false, false},
            {"169.254.0.0",        false, false, false},
            {"255.255.255.255",    false, false, false},
            {"2001:db8::123:12:1", true,  false, true},
            {"fe80::",             false, false, true},
            {"ff05::",             false, false, true},
    };
    for (const auto &tc: testCases) {
        // IP: 127.0.0.1, Expect: {IsGlobalUnicast: false, Failed: false, IsIPv6: false}
        LogDebug("IP<{}>, Expect<IsGlobalUnicast: {}, Failed: {}, IsIPv6: {}>", tc.s, tc.ok, tc.failed, tc.isIPv6);
        EXPECT_EQ(tc.isIPv6, IsIPv6(tc.s));

        bool failed;
        EXPECT_EQ(tc.ok, IsGlobalUniCast(tc.s, &failed));
        EXPECT_EQ(tc.failed, failed);
    }
}

TEST(NetToolTest, IsPrivate) {
    struct {
        std::string s;
        bool ok;
        bool failed;
        bool isIPv6;
    } testCases[] = {
            {"",                                        false, true,  false},
            {"1.1.1.1",                                 false, false, false},
            {"9.255.255.255",                           false, false, false},
            {"10.0.0.0",                                true,  false, false},
            {"10.255.255.255",                          true,  false, false},
#if defined(ENABLE_CLOUD_MONITOR)
            {"11.0.0.0",                                false, false, false},
#else
            {"11.0.0.0",                                true, false, false},
#endif
            {"172.15.255.255",                          false, false, false},
            {"172.16.0.0",                              true,  false, false},
            {"172.16.255.255",                          true,  false, false},
            {"172.23.18.255",                           true,  false, false},
            {"172.31.255.255",                          true,  false, false},
            {"172.31.0.0",                              true,  false, false},
            {"172.32.0.0",                              false, false, false},
            {"192.167.255.255",                         false, false, false},
            {"192.168.0.0",                             true,  false, false},
            {"192.168.255.255",                         true,  false, false},
            {"192.169.0.0",                             false, false, false},
            {"fbff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", false, false, true},
            {"fc00::",                                  true,  false, true},
            {"fcff:1200:0:44::",                        true,  false, true},
            {"fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff", true,  false, true},
            {"fe00::",                                  false, false, true},
    };

    for (const auto &tc: testCases) {
        LogDebug("IP<{}>, Expect<IsPrivate: {}, Failed: {}, IsIPv6: {}>", tc.s, tc.ok, tc.failed, tc.isIPv6);
        EXPECT_EQ(tc.isIPv6, IsIPv6(tc.s));

        bool failed;
        EXPECT_EQ(tc.ok, IsPrivate(tc.s, &failed));
        EXPECT_EQ(tc.failed, failed);
    }
}
