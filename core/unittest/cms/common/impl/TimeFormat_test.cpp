//
// Created by 韩呈杰 on 2023/3/30.
//
#include <gtest/gtest.h>
#include "common/TimeFormat.h"
#include "common/Chrono.h"
#include "common/StringUtils.h"
#include "common/Logger.h"

#include <boost/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

TEST(CommonDateTest, formatRaiseException) {
    try {
        EXPECT_FALSE(fmt::format("{", std::chrono::system_clock::now()).empty());
        EXPECT_FALSE(true); // 不应走到此处
    } catch (const std::exception &ex) {
        LogInfo("{}: {}", boost::core::demangle(typeid(ex).name()), ex.what());
    }
}

TEST(CommonDateTest, formatMicros) {
    const int64_t micros = 1680144985049905;
    {
        std::string str = date::format<6>(std::chrono::FromMicros(micros));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25.049905");
    }
    // {
    //     std::string str = date::format<3>(boost::chrono::FromMicros(micros));
    //     std::cout << str << std::endl;
    //     EXPECT_EQ(str, "2023-03-30 10:56:25.050");
    // }
    {
        std::string str = date::format<3>(std::chrono::FromMicros(micros), false);
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 02:56:25.050");
    }
}

TEST(CommonDateTest, formatMillis) {
    const int64_t millis = 1680144985049;
    {
        std::string str = date::format<3>(std::chrono::FromMillis(millis));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25.049");
    }
    // {
    //     std::string str = date::format<6>(boost::chrono::FromMillis(millis));
    //     std::cout << str << std::endl;
    //     EXPECT_EQ(str, "2023-03-30 10:56:25.049000");
    // }
    {
        std::string str = date::format<6>(std::chrono::FromMillis(millis), false);
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 02:56:25.049000");
    }
    // {
    //     std::string str = date::format<0>(boost::chrono::FromMillis(millis));
    //     std::cout << str << std::endl;
    //     EXPECT_EQ(str, "2023-03-30 10:56:25");
    // }
}

TEST(CommonDateTest, formatSeconds) {
    const int64_t secs = 1680144985;
    {
        std::string str = date::format<0>(std::chrono::FromSeconds(secs));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25");
    }
    {
        std::string str = date::format<0>(std::chrono::FromSeconds(secs), false);
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 02:56:25");
    }
    {
        std::string str = date::format<3>(std::chrono::FromSeconds(secs));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25.000");
    }
}


TEST(CommonDateTest, formatStdChrobo) {
    const int64_t secs = 1680144985;
    {
        std::string str = date::format<0>(std::chrono::FromSeconds(secs));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25");
    }
    {
        std::string str = date::format<0>(std::chrono::FromSeconds(secs), false);
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 02:56:25");
    }
    {
        std::string str = date::format<3>(std::chrono::FromSeconds(secs));
        std::cout << str << std::endl;
        EXPECT_EQ(str, "2023-03-30 10:56:25.000");
    }
}

TEST(CommonDateTest, StreamOutBoostChrono) {
    const int64_t micros = 1685115231160681LL;
    auto expected = []() {
        std::string s =
#ifdef WIN32
                "16851152311606810 [1/10000000]seconds"
#elif defined(__linux__) || defined(__unix__)
                "1685115231160681000 nanoseconds"
#elif defined(__APPLE__) || defined(__FreeBSD__)
                "1685115231160681000 nanoseconds"
#endif
        ;
        return s +
               " <=> 1685115231160681 microseconds"
               " <=> 1685115231160 milliseconds"
               " <=> 1685115231 seconds"
               " <=> 28085253 minutes"
               " <=> 468087 hours";
    };
    {
        auto now = boost::chrono::system_clock::time_point{boost::chrono::microseconds{micros}};
        std::stringstream ss;
        ss << now.time_since_epoch()
           << " <=> "
           << boost::chrono::duration_cast<boost::chrono::microseconds>(now.time_since_epoch())
           << " <=> "
           << boost::chrono::duration_cast<boost::chrono::milliseconds>(now.time_since_epoch())
           << " <=> "
           << boost::chrono::duration_cast<boost::chrono::seconds>(now.time_since_epoch())
           << " <=> "
           << boost::chrono::duration_cast<boost::chrono::minutes>(now.time_since_epoch())
           << " <=> "
           << boost::chrono::duration_cast<boost::chrono::hours>(now.time_since_epoch());
        std::cout << "boost: " << ss.str() << std::endl;
        EXPECT_EQ(ss.str(), expected());
    }
}

TEST(CommonDateTest, StreamOutStdChrono) {
    const int64_t micros = 1685115231160681LL;
    auto expected = []() {
        std::string s =
#ifdef WIN32
                "16851152311606810[1/10000000]s"
#elif defined(__linux__) || defined(__unix__)
                "1685115231160681000ns"
#elif defined(__APPLE__) || defined(__FreeBSD__)
                "1685115231160681µs"
#endif
        ;
        return s +
               " <=> 1685115231160681µs"
               " <=> 1685115231160ms"
               " <=> 1685115231s"
               " <=> 28085253m"
               " <=> 468087h";
    };
    {
        auto now = std::chrono::system_clock::time_point{std::chrono::microseconds{micros}};
        std::stringstream ss;
        ss << now.time_since_epoch()
           << " <=> "
           << std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch())
           << " <=> "
           << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
           << " <=> "
           << std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
           << " <=> "
           << std::chrono::duration_cast<std::chrono::minutes>(now.time_since_epoch())
           << " <=> "
           << std::chrono::duration_cast<std::chrono::hours>(now.time_since_epoch());
        std::cout << "  std: " << ss.str() << std::endl;
        EXPECT_EQ(ss.str(), expected());
    }
}

#if BOOST_VERSION >= 108100
TEST(CommonDateTest, StreamOutTimePoint) {
    namespace bc = boost::chrono;
    std::stringstream ss;
    ss << date::format<0>(std::chrono::system_clock::now(), "%F %T %Z") << std::endl
       << bc::time_fmt(bc::timezone::local, "") << bc::system_clock::now();
    std::cout << ss.str() << std::endl;
}
#endif

TEST(CommonDateTest, fromatFractionSeconds) {
    int64_t millis = 1710408609459;
    auto now = std::chrono::FromMillis(millis);
    std::string str = fmt::format("{:%F %T.00 %%}", now);
    LogInfo("milliseconds({}) => {}", convert(millis, true), str);
    EXPECT_EQ(str, "2024-03-14 09:30:09.46 %");
}
