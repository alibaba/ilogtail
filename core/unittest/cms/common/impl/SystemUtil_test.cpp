#include <gtest/gtest.h>
#include "common/SystemUtil.h"
#include "common/Logger.h"
#include "common/UnitTestEnv.h"
#ifdef ENABLE_ALIMONITOR
#include "common/StringUtils.h"
#endif
#include "common/FileUtils.h"
#include "common/Defer.h"

using namespace common;
using namespace std;

class CommonSystemUtilTest : public testing::Test {
protected:
    void SetUp() override{
    }

    void TearDown() override{
    }
};

TEST_F(CommonSystemUtilTest, ParseCluster) {
    string tianjiJson = ReadFileContent(TEST_CONF_PATH / "conf" / "tianji" / "tianji_curl.json");
    EXPECT_EQ(SystemUtil::ParseCluster(tianjiJson), "tianji-A-010b");

    EXPECT_TRUE(SystemUtil::ParseCluster("[]").empty());
    EXPECT_TRUE(SystemUtil::ParseCluster("{}").empty());
}

TEST_F(CommonSystemUtilTest, getRegionId) {
    string oldRegionId = SystemUtil::GetEnv("REGION_ID");
    SystemUtil::SetEnv("REGION_ID", "hello_region_id");

    string regionId = SystemUtil::getRegionId();
    EXPECT_EQ(regionId, "hello_region_id");

    if (oldRegionId.empty()) {
        SystemUtil::UnsetEnv("REGION_ID");
    } else {
        SystemUtil::SetEnv("REGION_ID", oldRegionId);
    }

    EXPECT_NE(SystemUtil::getRegionId(), "hello_region_id");
}

TEST_F(CommonSystemUtilTest, parseDmideCode) {
    string content;
    string result;

    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result.size(), size_t(0));

    content = "dmidecode";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "dmidecode");

    content = "\ndmidecode\n";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "dmidecode");

    content = "dmidecode\ni-test";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "dmidecode");

    content = "i-test";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "i-test");

    content = "i-test\n#hello1\n#hello2\ndmidecode";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "dmidecode");

    content = "i-test\n#hello1\ni-test2";
    result = SystemUtil::parseDmideCode(content);
    EXPECT_EQ(result, "");
}

TEST_F(CommonSystemUtilTest, GetTianjiCluster) {
    EXPECT_EQ(SystemUtil::GetTianjiCluster(), "");
}


TEST_F(CommonSystemUtilTest, getDmideCode)
{
#ifdef __linux__
    auto path = Which("dmidecode");
    if (path.empty()) {
        std::cout << "can not find dmidecode, skip this test" << std::endl;
        return;
    }
#endif
    // string content;
    // string result;
    std::string content = SystemUtil::getDmideCode();
    LogInfo("dmidecode: {}", content);
    EXPECT_FALSE(content.empty());
    //SystemUtil::execCmd("/usr/sbin/dmidecode -s system-serial-number |grep -v '#'",result);
    //EXPECT_EQ(content,result);
}

TEST_F(CommonSystemUtilTest, GetPathBase)
{
    // string result;
    EXPECT_EQ(SystemUtil::GetPathBase("/home/staragent"), "staragent");
    EXPECT_EQ(SystemUtil::GetPathBase(""), ".");
    EXPECT_EQ(SystemUtil::GetPathBase("/home/staragent/"), "staragent");
    EXPECT_EQ(SystemUtil::GetPathBase("//"), "/");
}

#ifdef WIN32
TEST_F(CommonSystemUtilTest, getWindowsDmidecode) {
    std::cout << SystemUtil::getWindowsDmidecode() << std::endl;
    EXPECT_FALSE(SystemUtil::getWindowsDmidecode().empty());

    std::cout << SystemUtil::GetWindowsDmidecode36() << std::endl;
    EXPECT_FALSE(SystemUtil::GetWindowsDmidecode36().empty());
}
#endif

#ifdef ENABLE_ALIMONITOR
TEST_F(CommonSystemUtilTest, GetLocalIP) {
    auto ips = SystemUtil::getLocalIp();
    // EXPECT_FALSE(ips.empty());
    LogDebug("LocalIPs: {}", StringUtils::join(ips, ", "));
    int count = 0;
    for (const auto &ip: ips) {
        LogDebug("[{}] {}", ++count, ip);
    }
}
#endif

TEST_F(CommonSystemUtilTest, GetDmidecode) {
#ifdef __linux__
    auto path = Which("dmidecode");
    if (path.empty()) {
        std::cout << "can not find dmidecode, skip this test" << std::endl;
        return;
    }
#endif
    auto sn = SystemUtil::getDmideCode();
    LogDebug("dmidecode: {}", sn);
    ASSERT_FALSE(sn.empty());
    EXPECT_NE(sn[0], '"');
}

TEST_F(CommonSystemUtilTest, getSn) {
#ifdef __linux__
    auto path = Which("dmidecode");
    if (path.empty()) {
        std::cout << "can not find dmidecode, skip this test" << std::endl;
        return;
    }
#endif
	auto sn = SystemUtil::getSn();
	LogDebug("sn: {}", sn);
	ASSERT_FALSE(sn.empty());
}

TEST_F(CommonSystemUtilTest, Env) {
    const char *key = __FUNCTION__;
    ASSERT_TRUE(SystemUtil::GetEnv(key).empty());
    defer(SystemUtil::UnsetEnv(key));

    SystemUtil::SetEnv(key, fmt::format("{}", __LINE__));
    constexpr int expr = __LINE__ - 1;
    EXPECT_EQ(SystemUtil::GetEnv(key), fmt::format("{}", expr));

    SystemUtil::UnsetEnv(key);
    ASSERT_TRUE(SystemUtil::GetEnv(key).empty());
}

TEST_F(CommonSystemUtilTest, getMainIp) {
    LogInfo("MainIP: {}", SystemUtil::getMainIp());
    EXPECT_FALSE(SystemUtil::getMainIp().empty());
}

TEST_F(CommonSystemUtilTest, IpCanBind) {
    EXPECT_FALSE(SystemUtil::IpCanBind(""));
    EXPECT_FALSE(SystemUtil::IpCanBind("\n"));
}

TEST_F(CommonSystemUtilTest, isInvalidIp) {
    EXPECT_TRUE(SystemUtil::isInvalidIp("", true));
    EXPECT_TRUE(SystemUtil::isInvalidIp("\n", true));
    EXPECT_TRUE(SystemUtil::isInvalidIp("100.8.2.1", true));
}