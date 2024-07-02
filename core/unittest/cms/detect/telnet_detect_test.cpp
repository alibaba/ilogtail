#include <gtest/gtest.h>
#include "detect/telnet_detect.h"

#include <string>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/Chrono.h"

using namespace std;
using namespace common;

class Detect_TelnetDetectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(Detect_TelnetDetectTest, test10) {
    auto *pDetect = new TelnetDetect("www.baidu.com:80", "helloworld\n", "Bad Request");
    EXPECT_EQ(TELNET_SUCCESS, pDetect->Detect());
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, test11) {
    TelnetDetect pDetect("www.baidu.com", "helloworld\n", "Bad Request");
    pDetect.mTimeout = 1_s; // windows下无效
    EXPECT_EQ(TELNET_TIMEOUT, pDetect.Detect());
}

TEST_F(Detect_TelnetDetectTest, test12) {
    auto *pDetect = new TelnetDetect("telnet://www.baidu.com:80", "helloworld\n", "Bad Request");
    EXPECT_EQ(TELNET_SUCCESS, pDetect->Detect());
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, test13) {
    auto *pDetect = new TelnetDetect("telnet://www.baidu.com:80", "helloworld\n", "111111neo");
    EXPECT_EQ(TELNET_MISMATCHING, pDetect->Detect());
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, test14) {
    auto *pDetect = new TelnetDetect("telnet://www.baidu.com:80", "helloworld", "111111neo");
    pDetect->mTimeout = 1_s;
    EXPECT_EQ(TELNET_MISMATCHING, pDetect->Detect());
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, test2) {
    auto *pDetect = new TelnetDetect("www.baidu.com:80", "helloworld", "Bad Request");
    pDetect->mTimeout = 1_s;
    EXPECT_EQ(TELNET_MISMATCHING, pDetect->Detect());
    delete pDetect;
}

// TEST_F(Detect_TelnetDetectTest, test3) {
//     // hancj.2023-12-07 这个IP地址，不知道是哪里
//     TelnetDetect pDetect("10.137.71.5:80", "helloworld", "Bad Request");
//     pDetect.mTimeout = 1_s;
//     LogDebug("{}=detect result: {}", TELNET_FAIL, pDetect.Detect());
// }

TEST_F(Detect_TelnetDetectTest, test4) {
    auto *pDetect = new TelnetDetect("127.0.0.1:80", "helloworld", "Bad Request");
    EXPECT_EQ(TELNET_FAIL, pDetect->Detect());
    delete pDetect;
}

bool contain(int n, const std::initializer_list<int> &s) {
    return std::any_of(s.begin(), s.end(), [n](int candidate) { return n == candidate; });
}

TEST_F(Detect_TelnetDetectTest, test51) {
    auto *pDetect = new TelnetDetect("telnet://10.137.71.2:15778", "", "");
    EXPECT_TRUE(contain(pDetect->Detect(), {TELNET_FAIL, TELNET_TIMEOUT}));
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, test52) {
    auto *pDetect = new TelnetDetect("telnet://10.137.71.2:15778", "helloworld", "helloworld");
    EXPECT_TRUE(contain(pDetect->Detect(), {TELNET_FAIL, TELNET_TIMEOUT}));
    delete pDetect;
}

TEST_F(Detect_TelnetDetectTest, ToUnixRequestBody) {
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("1234567890\n1234567890123456789");
        EXPECT_EQ(size_t(31), actual.size());
        EXPECT_EQ("1234567890\r\n1234567890123456789", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("1\n2");
        EXPECT_EQ(size_t(4), actual.size());
        EXPECT_EQ("1\r\n2", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("\n2");
        EXPECT_EQ(size_t(3), actual.size());
        EXPECT_EQ("\r\n2", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("1\n");
        EXPECT_EQ(size_t(3), actual.size());
        EXPECT_EQ("1\r\n", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("\n");
        EXPECT_EQ(size_t(2), actual.size());
        EXPECT_EQ("\r\n", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("1");
        EXPECT_EQ(size_t(1), actual.size());
        EXPECT_EQ("1", actual);
    }
    {
        std::string actual = TelnetDetect::ToUnixRequestBody("");
        EXPECT_EQ(size_t(0), actual.size());
        EXPECT_EQ("", actual);
    }
}

TEST_F(Detect_TelnetDetectTest, LeakDetect) {
    TelnetDetect detect("www.baidu.com:80");
    detect.mTimeout = 1_s;
    TimeProfile tp;
    size_t count;
    // 至少连续探测50次不挂
    for (count = 0; count < 50; count++) {
        EXPECT_EQ(TELNET_SUCCESS, detect.Detect());
    }
    LogInfo("telnet detect {} times, cost {}", count, tp.cost(true));
}
