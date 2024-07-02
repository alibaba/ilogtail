#include <gtest/gtest.h>
#include "common/CommonUtils.h"
#include <iostream>

#include "common/Logger.h"
#include "common/Chrono.h"

using namespace common;
using namespace std;

TEST(CommonUtilsTest, GetTimeStrLocal) {
    string str = CommonUtils::GetTimeStr(NowMicros());
    cout << "now=" << str << endl;
    EXPECT_EQ(str.size() > 0, true);
    EXPECT_GE(str.substr(0, 2), "20"); // >= 2000年
}

TEST(CommonUtilsTest, GetTimeStrGmt) {
    int64_t now{1680144985049905};
    {
        string str = CommonUtils::GetTimeStr(now, "%a, %d %b %Y %H:%M:%S GMT");
        cout << "microseconds(" << now << ") => " << str << endl;
        EXPECT_EQ(str, "Thu, 30 Mar 2023 02:56:25 GMT");
    }
    {
        string str = CommonUtils::GetTimeStr(now);
        cout << "micros(" << now << ") => " << str << endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25 +0800");
    }
}

TEST(CommonUtilsTest, convertLongLongToString) {
    long long num = 9223372036854775807;
    string str = "9223372036854775807";
    string res;
    res = convert(num);
    EXPECT_EQ(res, str);
    num = -9223372036854775808;
    str = "-9223372036854775808";
    res = convert(num);
    EXPECT_EQ(res, str);
}

TEST(CommonUtilsTest, ConvertStringToOneLine) {
    std::string src{"1\n2"};
    std::string dst = src;
    std::replace(dst.begin(), dst.end(), '\n', ' ');
    EXPECT_EQ(dst, "1 2");
    EXPECT_EQ(src, "1\n2");
}

TEST(CommonUtilsTest, HostName) {
    LogInfo("hostname: {}", CommonUtils::getHostName());
    EXPECT_NE(CommonUtils::getHostName(), "");
}

TEST(CommonUtilsTest, generateUuid) {
    LogInfo("uuid: {}", CommonUtils::generateUuid());
    EXPECT_NE(CommonUtils::generateUuid(), "");
}


TEST(CommonUtilsTest, GetTimeStr2)
{
#define PREFIX "%a, %d %b %Y %H:%M:%S"
    int64_t now = 1700041241139017LL;

    string str = CommonUtils::GetTimeStr(now, PREFIX" %z");
    std::cout << "now=" << str << std::endl;
    EXPECT_EQ(str, "Wed, 15 Nov 2023 17:40:41 +0800");

    string str2 = CommonUtils::GetTimeStr(now, PREFIX" GMT");
    std::cout << "now=" << str2 << std::endl;
    EXPECT_EQ(str2, "Wed, 15 Nov 2023 09:40:41 GMT");

    string str3 = CommonUtils::GetTimeStrGMT(now, PREFIX" GMT");
    std::cout << "now=" << str3 << std::endl;
    EXPECT_EQ(str3, "Wed, 15 Nov 2023 09:40:41 GMT");
#undef PREFIX
}
