#include <gtest/gtest.h>
#include "detect/http_detect.h"
#include <iostream>

#include "common/CppTimer.h"
#include "common/ModuleData.h"

#include "common/Config.h"
#include "common/Logger.h"

using namespace std;
using namespace common;

class Detect_HttpDetectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();
    }

    void TearDown() override {
    }
};

TEST_F(Detect_HttpDetectTest, test) {
    std::vector<CppTime::Timer *> tlist;
    tlist.reserve(3);
    for (int i = 0; i < 3; ++i) {
        tlist.push_back(new CppTime::Timer());
    }

    for (auto & it : tlist) {
        delete it;
    }
    tlist.clear();


    HttpDetect pDetect("", "https://www.baidu.com");
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test1) {
    HttpDetect pDetect("", "https://www.baidu1.com", "GET", "", "Content-Type: application/json,MyName:Hello,\n");
    const std::vector<std::string> expectHeaders{"Content-Type: application/json", "MyName:Hello"};
    EXPECT_EQ(pDetect.mHeaders, expectHeaders);
    pDetect.mTimeout = std::chrono::seconds{1};
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test2) {
    HttpDetect pDetect("", "http://127.0.0.1:22");
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test3) {
    HttpDetect pDetect("", "http://10.137.71.5");
    pDetect.mTimeout = std::chrono::seconds{1};
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test4) {
    HttpDetect pDetect("", "http://www.baidu.com");
    pDetect.mKeyword = "argusagent";
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test5) {
    HttpDetect pDetect("", "https://www.baidu.com");
    pDetect.mKeyword = "baidu";
    pDetect.mNegative = true;
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}

TEST_F(Detect_HttpDetectTest, test6) {
    HttpDetect pDetect("", "http://10.137.71.4:5000", "HEAD");
    pDetect.mKeyword = "baidu";
    pDetect.mNegative = true;
    HttpDetectResult res = pDetect.Detect();
    cout << "code=" << res.code << endl;
    cout << "bodyLen=" << res.bodyLen << endl;
}
