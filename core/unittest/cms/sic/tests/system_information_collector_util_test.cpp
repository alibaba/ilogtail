#include <gtest/gtest.h>
#include "sic/system_information_collector_util.h"
#include "sic/system_information_collector.h"

#include "common/BoostStacktraceMt.h"
#include "common/Logger.h"
#include "common/Uuid.h"
#include "common/StringUtils.h"
#include "common/FilePathUtils.h"

#include <list>
#include <array>

#include <boost/chrono/chrono_io.hpp>

#ifdef min
#   undef min
#endif
#ifdef max
#   undef max
#endif

#if !defined(ONE_AGENT)
extern const fs::path TEST_SIC_CONF_PATH = fs::path(__FILE__).parent_path();
#else
#include "common/UnitTestEnv.h"
#endif

using namespace common;

TEST(SicSystemInformationCollectorTest, ConfPath) {
    std::cout << "TEST_SIC_CONF_PATH: " << TEST_SIC_CONF_PATH << std::endl;
    const std::string suffix = "/sic/tests";
    std::string sub = TEST_SIC_CONF_PATH.string().substr(TEST_SIC_CONF_PATH.string().size() - suffix.size());
    EXPECT_NE(sub, "");
}

TEST(SicSystemInformationCollectorTest, SicNetState) {
    SicNetState obj;
    for (auto n: obj.tcpStates) {
        EXPECT_EQ(n, 0);
    }
    std::string s = obj.toString();
    EXPECT_NE(s, "");
}

TEST(SicSystemInformationCollectorTest, EnumTcpStateName) {
    EXPECT_EQ(std::string("ESTABLISHED"), GetTcpStateName(SIC_TCP_ESTABLISHED));
    EXPECT_EQ(std::string("NO_USED"), GetTcpStateName(EnumTcpState(0)));
    for (int i = 1; i <= SIC_TCP_STATE_END; i++) {
        std::string name = GetTcpStateName((EnumTcpState) i);
        EXPECT_FALSE(name.empty());
    }
}

TEST(SicSystemInformationCollectorTest, RValueScope) {
    struct A {
        const int a;
        std::stringstream &ss;

        explicit A(std::stringstream &s, int v) : a(v), ss(s) {
            ss << "A::A(" << a << ")" << std::endl;
        }

        ~A() {
            ss << "A::~A(" << a << ")" << std::endl;
        }
    };

    auto getA = [](std::stringstream &s, int v) {
        return std::make_shared<A>(s, v);
    };

    std::stringstream ss;
    getA(ss, 1);  // 实际测试，下一条语句前，返回值就被销毁了
    getA(ss, 2);
    LogDebug("\n{}", ss.str());

    std::vector<std::string> lines = StringUtils::split(ss.str(), '\n', false);
    std::array<std::string, 4> expectLines{{"A::A(1)", "A::~A(1)", "A::A(2)", "A::~A(2)",}};
    ASSERT_EQ(expectLines.size(), lines.size());
    for (size_t i = 0; i < expectLines.size(); i++) {
        EXPECT_EQ(expectLines[i], TrimSpace(lines[i]));
    }
}

TEST(SicSystemInformationCollectorTest, BoostTimePoint) {
    using namespace boost::chrono;
    auto now = system_clock::now();
    std::cout << "Now: " << now << std::endl;
    std::ostringstream oss;
    oss << now;
    EXPECT_TRUE(HasSuffix(oss.str(), " +0000"));
    std::cout << "Now: " << time_fmt(boost::chrono::timezone::local, "") << now << std::endl;
    std::cout << time_fmt(boost::chrono::timezone::utc, "%a, %d %b %Y %H:%M:%S GMT") << now << std::endl;

    system_clock::time_point tp{microseconds{1680144985049905}};

    std::string actual = (sout{} << time_fmt(boost::chrono::timezone::utc, "%a, %d %b %Y %H:%M:%S GMT") << tp).str();
    EXPECT_EQ(actual, "Thu, 30 Mar 2023 02:56:25 GMT");

    // ${BOOST_SOURCE}/doc/html/chrono/reference.html#chrono.reference.io.ios_state_hpp.sag.set_time_fmt
    {
        std::string actual2 = (sout{} << time_fmt(boost::chrono::timezone::local, "%Y-%m-%d %T") << tp
                                      << '.' << std::setfill('0') << std::setw(6)
                                      << duration_cast<microseconds>(tp.time_since_epoch() % seconds(1)).count()
        ).str();
        std::cout << actual2 << std::endl;
        EXPECT_EQ(actual2, "2023-03-30 10:56:25.049905");
    }
}

#include <boost/stacktrace.hpp>
#include <boost/stacktrace/safe_dump_to.hpp>

TEST(SicSystemInformationCollectorTest, StackTraceDump) {
#ifndef WIN32
    const std::string dumpFile = (GetExecDir() / "backtrace.dump").string();
    std::cout << "dumpFile: " << dumpFile << std::endl;
    boost::filesystem::remove(dumpFile);
    EXPECT_FALSE(boost::filesystem::exists(dumpFile));
    EXPECT_GT(boost::stacktrace::safe_dump_to(0, 1024, dumpFile.c_str()), (size_t)0);
    EXPECT_TRUE(boost::filesystem::exists(dumpFile));

    // there is a backtrace
    std::ifstream ifs(dumpFile);

    boost::stacktrace::stacktrace st = boost::stacktrace::stacktrace::from_dump(ifs);
    std::cout << "Previous run crashed:\n" << boost::stacktrace::mt::to_string(st);

    // cleaning up
    ifs.close();
    boost::filesystem::remove(dumpFile);
#endif
}

TEST(SicSystemInformationCollectorTest, SizeOf) {
    // 官方关于数值类型文档：
    // https://en.cppreference.com/w/cpp/language/types # Fundamental types
    // https://en.cppreference.com/w/cpp/types/integer # Fixed width integer types (since C++11)
#define SIZEOF(T) LogInfo("sizeof(" #T ") => {}", sizeof(T))
    SIZEOF(char);
    SIZEOF(short);
    SIZEOF(short int);
    SIZEOF(int);
    SIZEOF(signed);
    SIZEOF(signed int);
    SIZEOF(long);
    SIZEOF(long int);
    SIZEOF(long long);
    SIZEOF(long long int);

    std::cout << std::endl;
    SIZEOF(float);
    SIZEOF(double);

    std::cout << std::endl;
    SIZEOF(int8_t);
    SIZEOF(int16_t);
    SIZEOF(int32_t);
    SIZEOF(int64_t);

    std::cout << std::endl;
    SIZEOF(int_fast8_t);
    SIZEOF(int_fast16_t);
    SIZEOF(int_fast32_t);
    SIZEOF(int_fast64_t);

    std::cout << std::endl;
    SIZEOF(int_least8_t);
    SIZEOF(int_least16_t);
    SIZEOF(int_least32_t);
    SIZEOF(int_least64_t);

    std::cout << std::endl;
    SIZEOF(intmax_t);
    SIZEOF(intptr_t);

    std::cout << std::endl;
    SIZEOF(size_t);
#undef PR
}
