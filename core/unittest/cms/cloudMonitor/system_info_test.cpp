#include <gtest/gtest.h>
#include <iostream>
#include "cloudMonitor/system_info.h"
#include "common/Config.h"
#include "common/Logger.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;

class CmsSystemInfoTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(CmsSystemInfoTest, test) {
    SystemInfo pInfo;
    boost::json::value json = pInfo.GetSystemInfo("test11111");
    std::string actual = boost::json::serialize(json);
    cout << actual << endl;
}

TEST_F(CmsSystemInfoTest, Cmp) {
    SystemInfo pInfo;

    SicSystemInfo sysInfo;
    std::vector<std::string> ips;
    pInfo.collectSystemInfo(sysInfo, ips);
    int count = 0;
    for (const std::string &ip: ips) {
        LogDebug("[{}] {}", ++count, ip);
    }

    ips.clear();
    ips.emplace_back("11.160.139.211");

    std::string actual = boost::json::serialize(pInfo.MakeJson(sysInfo, ips, "123", 19301481));
    cout << actual << endl;

    const std::string expect = pInfo.MakeJsonString(sysInfo, ips, "123", 19301481);
    EXPECT_TRUE(HasPrefix(actual, expect));
}

namespace cloudMonitor {
    extern void highPriorityIPFirst(std::vector<std::string> &ips);
}
TEST_F(CmsSystemInfoTest, highPriorityIPFirst) {
    std::vector<std::string> ips{
            std::string("192.168.1.1"),
            std::string("30.52.96.38"),
    };
    ASSERT_EQ(size_t(2), ips.size());
    highPriorityIPFirst(ips);
    EXPECT_EQ(ips[0], "30.52.96.38");
    EXPECT_EQ(ips[1], "192.168.1.1");
}

#if !defined(__APPLE__) && !defined(__FreeBSD__)
TEST_F(CmsSystemInfoTest, GetSystemFreeSpace) {
    auto freeSpace = SystemInfo{}.GetSystemFreeSpace();
        // 如果指定目录存在，则表明当前处于OneAgent环境下，此时忽略该用例
    if (!fs::exists("/workspaces/ilogtail")) {
        EXPECT_GT(freeSpace, 0);
        // 有些系统不支持std::locale("en_US.UTF-8")
        LogDebug("FreeSpace: {} KB", convert(freeSpace, true));
    }
}
#endif // !macOS

TEST_F(CmsSystemInfoTest, collectSystemInfo) {
    SicSystemInfo sysInfo;
    std::vector<std::string> ips;
    SystemInfo{}.collectSystemInfo(sysInfo, ips);
    EXPECT_FALSE(sysInfo.name.empty());
    ASSERT_FALSE(ips.empty());
    LogDebug("ips: {}", boost::json::serialize(boost::json::value_from(ips)));
}
