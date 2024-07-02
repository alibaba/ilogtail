#include <gtest/gtest.h>
#include "common/Common.h"

#include <thread>
#include <chrono>
#include <iostream>

#include "common/TimeFormat.h"
#include "common/UnitTestEnv.h"
#include "common/Chrono.h"
#include "common/Logger.h"

using namespace common;
using namespace std;
using namespace std::chrono;

TEST(CommonTest, getCmdOutput) {
    string cmd = "echo \"aaaa\nbbbb\r\ncccc\n\r\n\r\"";
    std::cout << "command:[ " << cmd << "]" << std::endl;
    // char buf[MAX_STR_LEN+1];
    // std::string buf;
    string strbuf;
    EXPECT_EQ(getCmdOutput(cmd, strbuf), 0);
    std::cout << strbuf << std::endl;
    // EXPECT_EQ(getCmdOutput(cmd,strbuf),0);
#ifdef WIN32
    EXPECT_EQ(strbuf, "\"aaaa");
#else
    EXPECT_EQ(strbuf, "aaaa\nbbbb\r\ncccc");

    // windows命令行到回车那里就结束了，以下的测试，在windows已无意义
    string cmd2 = "echo \"";
    for (int i = 0; i < 4096; i++) {
        cmd2 += "aaaa\nbbbb\ncccc\n";
    }
    cmd2 += "\"";
    strbuf.clear();
    EXPECT_EQ(getCmdOutput(cmd2, strbuf), 0);
    EXPECT_EQ(strbuf.size(), size_t(4096 * 15 - 1));
    string cmd3 = "echo \"";
    for (int i = 0; i < 4096; i++) {
        cmd3 += "aaaabbbbcccc";
    }
    cmd3 += "\"";
    strbuf.clear();
    EXPECT_EQ(getCmdOutput(cmd3, strbuf), 0);
    EXPECT_EQ(strbuf.size(), size_t(4096 * 12));
#endif
}

TEST(CommonTest, replaceMacro) {
    // 2004-01-03 07:08:09 +0800  周六
    const auto tp = std::chrono::FromSeconds(1073084889);

    std::string patterns[] = {
            "echo %year% %month% %day% %week% %hour%",
            "echo $YEAR$ $MONTH$ $DATE$ $WEEK$ $HOUR$",
    };
    for (const std::string &oldStr: patterns) {
        string newStr = replaceMacro(oldStr, tp);
        cout << newStr << endl;
        EXPECT_EQ(newStr, "echo 2004 01 03 6 07");
    }

    std::map<std::string, std::string> mapItems{
            {"$YEAR$",       "2004"},
            {"%year%",       "2004"},
            {"$SHORTYEAR$",  "04"},
            {"$MONTH$",      "01"},
            {"%month%",      "01"},
            {"$SHORTMONTH$", "1"},
            {"$DATE$",       "03"},
            {"$DAY$",        "03"},
            {"%day%",        "03"},
            {"$SHORTDATE$",  "3"},
            {"$HOUR$",       "07"},
            {"%hour%",       "07"},
            {"$SHORTHOUR$",  "7"},
            {"$WEEK$",       "6"},
            {"%week%",       "6"},
            {"$MINUTE$",     "08"},
            {"$SECOND$",     "09"},
    };
    for (const auto &it: mapItems) {
        std::cout << fmt::format("{:13s}", it.first) << ": " << replaceMacro(it.first, tp) << std::endl;
        EXPECT_EQ(replaceMacro(it.first, tp), it.second);
    }
    const std::string key = "$BASEDIR$";
    auto baseDir = replaceMacro(key, tp);
    std::cout << fmt::format("{:13s}", key) << ": " << baseDir << std::endl;
    EXPECT_FALSE(baseDir.empty());
    EXPECT_NE(baseDir, key);

    EXPECT_EQ(replaceMacro("", tp), "");
}

TEST(CommonTest, getTimeString) {
    // using namespace boost::chrono;
    // auto now = system_clock::now();
    // int64_t micros = duration_cast<microseconds>(now.time_since_epoch()).count();
    // std::cout << micros << std::endl;
    int64_t micros = 1680626913641323LL;
    EXPECT_EQ(getTimeString(micros), "2023-04-05 00:48:33.641323");

    // std::cout << date::format<6>(boost::chrono::FromMicros(micros)) << std::endl;
    std::cout << getTimeString(micros) << std::endl;
    EXPECT_EQ(date::format<6>(std::chrono::FromMicros(micros)), getTimeString(micros));
}

TEST(CommonTest, GetMd5_01) {
    fs::path file = TEST_CONF_PATH / "conf" / "pinglist.json";
    EXPECT_GT(fs::file_size(file), boost::uintmax_t(2 * 1024));
    std::string md5;
    EXPECT_EQ(0, GetMd5(file.string(), md5));
    // 两种系统的路径分隔符不一样，导致md5也不一样
#ifdef WIN32
    EXPECT_EQ(md5, "aacff748cfb331e8fbc468779d1b5fe5");
#else
    EXPECT_EQ(md5, "7626fddbfde4f37f8a8f5c2f29b8ebb3");
#endif
}

TEST(CommonTest, GetMd5_02) {
    fs::path file = TEST_CONF_PATH / "conf/~!@#$%^.txt";
    ASSERT_FALSE(fs::exists(file));

    std::string md5;
    EXPECT_EQ(-1, GetMd5(file.string(), md5));
}

TEST(CommonTest, GetVersionDetail) {
    EXPECT_NE(getVersionDetail(), "");
}

TEST(CommonTest, getErrorString) {
    EXPECT_EQ(getErrorString(0), "(0) not defined error");
    EXPECT_EQ(getErrorString(1), "Operation not permitted");
}

TEST(CommonTest, compileTime) {
    std::cout << "Compile At: " << compileTime() << std::endl;
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    int count = sscanf(compileTime().c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
                       &year, &month, &day, &hour, &minute, &second);
    EXPECT_EQ(count, 6);

    EXPECT_GE(year, 2023);  // 2023
    EXPECT_GT(month, 0);
    EXPECT_LE(month, 12);  // 01 - 12
    EXPECT_GT(day, 0);
    EXPECT_LE(day, 31); // 01 - 31

    EXPECT_GE(hour, 0);
    EXPECT_LT(hour, 24);  // 00 - 23
    EXPECT_GE(minute, 0);
    EXPECT_LT(minute, 60);  // 00 - 59
    EXPECT_GE(second, 0);
    EXPECT_LT(second, 60);  // 00 - 59
}


TEST(CommonTest, getHashStartTime)
{
    std::set<decltype(system_clock::now() - system_clock::now())> deltaSet{};
    for(int i=0;i<10;i++){
        system_clock::time_point  now = system_clock::now();
        system_clock::time_point t = getHashStartTime(now, 86400_ms, 43200_ms);
        auto delta = t - now;
        std::cout << "delta=" << duration_cast<milliseconds>(delta) << std::endl;
        EXPECT_EQ(deltaSet.find(delta), deltaSet.end());
        deltaSet.insert(delta);
    }

    auto now = system_clock::now();
    EXPECT_EQ(now, getHashStartTime(now, 0_s, 0_s));
}

TEST(CommonTest, randValue)
{
    for (int i = 0; i < 240; i = i + 10){
        int64_t end = 360*(i+1);
        auto value = randValue(0, (end-1));
        static_assert(std::is_same<decltype(value), int64_t>::value, "type of value is not int64_t");
        EXPECT_GE(value, 0);
        EXPECT_LE(value, end-1);
        std::cout << "randValue=" << value << ",end=" << end << std::endl;
    }
}

TEST(CommonTest, typeName) {
    std::string s;
    std::cout << typeName(&s) << std::endl;
    EXPECT_FALSE(typeName(&s).empty());
}

TEST(CommonTest, enableString) {
    EXPECT_EQ(enableString(""), "");
    EXPECT_EQ(enableString(R"(\)"), R"(\\)");
#ifndef WIN32
    EXPECT_EQ(enableString(R"(ls "C:\P F")"), R"(ls "\"C:\\P F\"")");
    EXPECT_EQ(enableString(R"(ls "P F" -"l")"), R"(ls "\"P F\"" -"l")");
#endif
}

TEST(CommonTest, UrlDecode) {
    EXPECT_EQ(UrlDecode("https://www.baidu.com"), "https://www.baidu.com");
}
TEST(CommonTest, joinOutput) {
    std::vector<std::pair<std::string, std::string>> output;
    output.emplace_back("a", "");
    output.emplace_back("b", "");
    EXPECT_EQ(joinOutput(output), R"XX([{"a":""},{"b":""}])XX");
}


TEST(CommonTest, GetPid) {
    EXPECT_GT(GetPid(), 0);
    EXPECT_GE(GetParentPid(), 0);
    std::cout << "PID: " << GetPid() << ", PPID: " << GetParentPid() << std::endl;
}


TEST(CommonTest, GetThisThreadId) {
    int tid = GetThisThreadId();
    EXPECT_NE(0, tid);
    std::cout << "           GetThisThreadId: " << tid << std::endl
              << "std::this_thread::get_id(): " << std::this_thread::get_id() << std::endl;
}


TEST(CommonTest, IPv4String) {
    uint8_t arr[4] = {192, 68, 0, 1};
    std::string strIP = IPv4String(*(uint32_t *)arr);
    EXPECT_EQ(strIP, "192.68.0.1");

    EXPECT_EQ(IPv4String(0), "0.0.0.0");
    EXPECT_EQ(IPv4String(-1), "255.255.255.255");
}

TEST(CommonTest, IPv4String_BigEndian) {
    uint32_t num = 0x0100007F;
    std::string strIP = IPv4String(num);
    LogInfo("0x{:08X} => {}", num, strIP);
    EXPECT_EQ(strIP, "127.0.0.1");
}

TEST(CommonTest, MacString) {
    {
        uint8_t arr[6] = {192, 68, 0, 1, 10, 255};
        std::string strIP = MacString(arr);
        EXPECT_EQ(strIP, "C0:44:00:01:0A:FF");
    }
    {
        uint8_t arr[6];
        memset(arr, 0, sizeof(arr));
        EXPECT_EQ(MacString(arr), "00:00:00:00:00:00");
    }
}
