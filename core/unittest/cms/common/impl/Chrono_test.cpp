//
// Created by 韩呈杰 on 2023/11/8.
//
#include <gtest/gtest.h>
#include "common/Chrono.h"

#include <iomanip>  // std::put_time, gcc 5.0以下没有该函数

#include <boost/chrono.hpp>
#include <boost/version.hpp>

#include "common/Logger.h"
#include "common/Defer.h"
#include "common/StringUtils.h"

template<typename T = int64_t>
T ToSeconds(const boost::chrono::system_clock::time_point &t) {
    namespace bc = boost::chrono;
	return static_cast<T>(bc::duration_cast<bc::seconds > (t.time_since_epoch()).count());

}

int64_t ToMillis(const boost::chrono::system_clock::time_point &t) {
	namespace bc = boost::chrono;
	return bc::duration_cast<bc::milliseconds>(t.time_since_epoch()).count();
}

int64_t ToMicros(const boost::chrono::system_clock::time_point &t) {
	namespace bc = boost::chrono;
	return bc::duration_cast<bc::microseconds>(t.time_since_epoch()).count();
}

bool IsZero(const boost::chrono::system_clock::time_point &t) {
    return t.time_since_epoch().count() == 0;
}

bool IsZero(const boost::chrono::steady_clock::time_point &t) {
    return t.time_since_epoch().count() == 0;
}


template<typename Rep, typename Period>
int64_t ToSeconds(const boost::chrono::duration<Rep, Period> &t) {
	namespace bc = boost::chrono;
	return bc::duration_cast<bc::seconds>(t).count();
}

template<typename Rep, typename Period>
int64_t ToMillis(const boost::chrono::duration<Rep, Period> &t) {
	namespace bc = boost::chrono;
	return bc::duration_cast<bc::milliseconds>(t).count();
}

template<typename Rep, typename Period>
int64_t ToMicros(const boost::chrono::duration<Rep, Period> &t) {
	namespace bc = boost::chrono;
	return bc::duration_cast<bc::microseconds>(t).count();
}

namespace boost {
    namespace chrono {
        typedef duration<double> fraction_seconds;
        typedef duration<double, boost::milli> fraction_millis;
        typedef duration<double, boost::micro> fraction_micros;

        system_clock::time_point FromSeconds(int64_t secs) {
            return system_clock::time_point(seconds{secs});
        }

        system_clock::time_point FromMillis(int64_t millis) {
            return system_clock::time_point{milliseconds{millis}};
        }

        system_clock::time_point FromMicros(int64_t micros) {
            return system_clock::time_point{microseconds{micros}};
        }

        ImplementChronoFunc

        template<typename Req, typename Period>
        system_clock::time_point Align(const system_clock::time_point &t,
                                       const duration<Req, Period> &interval,
                                       int multiply = 0) {
            IMPL_ALIGN(boost::chrono);
        }
    }
}

using boost::chrono::Align;

namespace date {
    /// boost <--> std 互换
    template<typename T>
    struct conv_ratio;
    template<std::intmax_t N, std::intmax_t D>
    struct conv_ratio<boost::ratio<N, D>> {
        using type = std::ratio<N, D>;
    };
    template<std::intmax_t N, std::intmax_t D>
    struct conv_ratio<std::ratio<N, D>> {
        using type = boost::ratio<N, D>;
    };
    template<typename T>
    using conv_ratio_t = typename conv_ratio<T>::type;

// Convert from boost::duration to std::duration
    template<typename T>
    struct conv_duration;

    template<typename Rep, typename Period>
    struct conv_duration<boost::chrono::duration<Rep, Period>> {
        using type = std::chrono::duration<Rep, conv_ratio_t<Period>>;
    };

    template<typename Rep, typename Period>
    struct conv_duration<std::chrono::duration<Rep, Period>> {
        using type = boost::chrono::duration<Rep, conv_ratio_t<Period>>;
    };

    template<typename T>
    using conv_duration_t = typename conv_duration<T>::type;

/* Convert from A::time_point to B::time_point.  This assumes that A
 * and B are clocks with the same epoch. */
    template<typename A, typename B>
    typename B::time_point convert_time_point_same_clock(typename A::time_point const &tp) {
        return typename B::time_point(conv_duration_t<typename A::time_point::duration>(
                tp.time_since_epoch().count()));
    }
}
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
using namespace std::chrono;

TEST(CommonChronoUtilTest, FromMillis) {
    int64_t micros = NowMicros();
    if ((micros % 1000) == 0) {
        micros += 28; // 确保拿到的精度确实是微秒级时间戳
    }

    namespace bc = boost::chrono;
#if BOOST_VERSION >= 108100
    std::cout << "Now: " << bc::time_fmt(bc::timezone::local, "")
              << std::chrono::Now<std::chrono::seconds>()
              << std::endl;
#endif
    auto now1 = bc::system_clock::now();
    {
        int64_t seconds = ToSeconds(now1);
        auto now = bc::FromSeconds(seconds);
        auto stdNow = std::chrono::FromSeconds(seconds);
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));

#if BOOST_VERSION >= 108100
        std::cout << "FromSeconds: " << bc::time_fmt(bc::timezone::local, "") << now << std::endl;
#endif
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));
        EXPECT_EQ(ToMicros(now), ToSeconds(now1) * 1000 * 1000);
        EXPECT_EQ(ToSeconds(now), ToSeconds(now1));
    }
    {
        int64_t millis = ToMillis(now1); // NowMillis();
        auto now = bc::FromMillis(millis);
        auto stdNow = std::chrono::FromMillis(millis);
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));

#if BOOST_VERSION >= 108100
        std::cout << "FromMillis: " << bc::time_fmt(bc::timezone::local, "") << now << std::endl;
#endif
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));
        EXPECT_EQ(ToMicros(now), ToMillis(now1) * 1000);
        EXPECT_EQ(ToMillis(now), ToMillis(now1));
    }
    {
        int64_t millis = ToMicros(now1); // NowMillis();
        auto now = bc::FromMicros(millis);
        auto stdNow = std::chrono::FromMicros(millis);
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));

#if BOOST_VERSION >= 108100
        std::cout << "FromMicros: " << bc::time_fmt(bc::timezone::local, "") << now << std::endl;
#endif
        EXPECT_EQ(ToMicros(now), ToMicros(stdNow));
        EXPECT_EQ(ToMicros(now), ToMicros(now1));
    }
}

TEST(CommonChronoUtilTest, IsZero) {
    EXPECT_TRUE(IsZero(std::chrono::system_clock::time_point{}));
    EXPECT_TRUE(IsZero(std::chrono::steady_clock::time_point{}));
    EXPECT_FALSE(IsZero(std::chrono::system_clock::now()));
    EXPECT_FALSE(IsZero(std::chrono::steady_clock::now()));

    EXPECT_TRUE(IsZero(boost::chrono::system_clock::time_point{}));
    EXPECT_TRUE(IsZero(boost::chrono::steady_clock::time_point{}));
    EXPECT_FALSE(IsZero(boost::chrono::system_clock::now()));
    EXPECT_FALSE(IsZero(boost::chrono::steady_clock::now()));
}

TEST(CommonChronoUtilTest, Align) {
    // 12:31:45
    const int64_t micros = int64_t(12 * 60 * 60 + 31 * 60 + 45) * 1000 * 1000;
    auto tt = std::chrono::FromMicros(micros);
    auto bt = boost::chrono::FromMicros(micros);
    int64_t expect;

    expect = int64_t(12 * 60 * 60 + 31 * 60) * 1000 * 1000;
    EXPECT_EQ(Align(tt, std::chrono::seconds{60}, false), std::chrono::FromMicros(expect));
    EXPECT_EQ(Align(bt, boost::chrono::seconds{60}, false), boost::chrono::FromMicros(expect));

    expect = int64_t(12 * 60 * 60 + 32 * 60) * 1000 * 1000;
    EXPECT_EQ(Align(tt, std::chrono::seconds{60}, true), std::chrono::FromMicros(expect));
    EXPECT_EQ(Align(bt, boost::chrono::seconds{60}, true), boost::chrono::FromMicros(expect));
}

TEST(CommonChronoUtilTest, LocalTimeOffset) {
    std::chrono::seconds stdOffset = (getLocalTimeOffset() == 0_s ? 0_s : 28800_s);
    EXPECT_EQ(stdOffset, getLocalTimeOffset());
    EXPECT_EQ(stdOffset, getLocalTimeOffsetNow());

#ifndef WIN32
    const char *tzOld = getenv("TZ");
    defer({
              if (tzOld != nullptr) {
                  setenv("TZ", tzOld, 1);
              } else {
                  unsetenv("TZ");
              }
              tzset();
          });

    EXPECT_EQ(0, setenv("TZ", "EST+5", 1));
    tzset();  // 使环境变量TZ生效
    EXPECT_EQ(stdOffset, getLocalTimeOffset());
    EXPECT_EQ(std::chrono::seconds{-18000}, getLocalTimeOffsetNow());

    EXPECT_EQ(0, setenv("TZ", "", 1));
    tzset();
    EXPECT_EQ(stdOffset, getLocalTimeOffset());
    EXPECT_EQ(0, getLocalTimeOffsetNow().count());

    EXPECT_EQ(0, setenv("TZ", "CST-1", 1));
    tzset();
    EXPECT_EQ(stdOffset, getLocalTimeOffset());
    EXPECT_EQ(std::chrono::seconds{3600}, getLocalTimeOffsetNow());

    EXPECT_EQ(0, unsetenv("TZ"));
    tzset();
    EXPECT_EQ(stdOffset, getLocalTimeOffset());
    EXPECT_EQ(stdOffset, getLocalTimeOffsetNow());
#endif
}

TEST(CommonChronoUtilTest, Format) {
    namespace sc = std::chrono;
    namespace bc = boost::chrono;

    sc::system_clock::time_point now = sc::FromMicros(1699769279071393);

#if BOOST_VERSION >= 108100
    {
        // boost bug 01: 单参time_fm实际上复用了上一次的fmt_
        std::stringstream ss;
        ss << bc::time_fmt(bc::timezone::utc, "%F %T") << now << "|" << bc::time_fmt(bc::timezone::local) << now;
        // EXPECT_EQ(ss.str(), "2023-11-12 06:07:59|2023-11-12 14:07:59.071393000 +0800");
        EXPECT_EQ(ss.str(), "2023-11-12 06:07:59|2023-11-12 14:07:59"); // 真正的答案应该是上一个
    }
    {
        // 修复 bug 01
        std::stringstream ss;
        ss << bc::time_fmt(bc::timezone::utc, "%F %T") << now << "|";
        ss << bc::time_fmt(bc::timezone::local, "") << now;
        if (DurationFractionWidth<decltype(now)::duration>::width == 6) {
            EXPECT_EQ(ss.str(), "2023-11-12 06:07:59|2023-11-12 14:07:59.071393 +0800");
        } else if (DurationFractionWidth<decltype(now)::duration>::width == 7) {
            EXPECT_EQ(ss.str(), "2023-11-12 06:07:59|2023-11-12 14:07:59.0713930 +0800");
        } else {
            EXPECT_EQ(ss.str(), "2023-11-12 06:07:59|2023-11-12 14:07:59.071393000 +0800");
        }
    }
    {
        // boost bug 02: utc模式下，%z或%Z输出的时区不正确
        std::stringstream ss;
        ss << bc::time_fmt(bc::timezone::utc, "%F %T %z") << now;
        EXPECT_EQ(ss.str(), "2023-11-12 06:07:59 +0000");
        ss << bc::time_fmt(bc::timezone::utc, "%F %T %Z") << now;
        EXPECT_NE(ss.str(), "2023-11-12 06:07:59 UTC");
    }
#endif // #if BOOST_VERSION >= 108100

    {
        //std::cout << "User-preferred locale setting is " << std::locale("").name().c_str() << '\n';
        // 不知道为啥，windows下%Z RelWithDebInfo输出乱码
        std::stringstream ss;
        char sub_pattern[] = {'%', 'Z'};
        char *pb = sub_pattern;
        char *pe = pb + sizeof(sub_pattern) / sizeof(char);
        std::tm tm = fmt::localtime(sc::system_clock::to_time_t(now));
        const auto &tpf = std::use_facet<std::time_put<char> >(ss.getloc());
        tpf.put(ss, ss, ss.fill(), &tm, pb, pe);
        std::string s = ss.str();
        std::cout << __LINE__ << ". " << s << std::endl;
    }
    // gcc < 5时，没有std::put_time
#if defined(__clang__) || __GNUC__ >= 5
    {
        std::stringstream ss;
        std::tm tm = fmt::localtime(sc::system_clock::to_time_t(now));
        ss << std::put_time(&tm, "%Z");
        std::cout << __LINE__ << ". " << ss.str() << std::endl;
    }
    {
        std::stringstream ss;
        std::tm tm = fmt::gmtime(sc::system_clock::to_time_t(now));
        ss << std::put_time(&tm, "%Z");
        std::cout << __LINE__ << ". " << ss.str() << std::endl;
    }
#endif

#ifdef WIN32
#define CST "\xD6\xD0\xB9\xFA\xB1\xEA\xD7\xBC\xCA\xB1\xBC\xE4" // GBK编码的： 中国标准时间
#else
#define CST "CST"
#endif
#if defined(__linux__)
#   define UTC "GMT"
#else
#   define UTC "UTC"
#endif

    auto bNow = date::convert_time_point_same_clock<sc::system_clock, bc::system_clock>(now);
    // 这也是一种方式，但是没有毫秒
    auto to_time_t = sc::system_clock::to_time_t;

#define F(L, R) fmt::print("{}: {:36} {} {}\n", __LINE__, R, ((L) == (R)? "==": "!="), #L); EXPECT_EQ(L, R)

	if (DurationFractionWidth<decltype(now)::duration>::width == 6) {
		F(fmt::format("{}", now), "2023-11-12 06:07:59.071393 +0000");
		F(fmt::format("{:L}", now), "2023-11-12 14:07:59.071393 +0800");
	} else if (DurationFractionWidth<decltype(now)::duration>::width == 7) {
		F(fmt::format("{}", now), "2023-11-12 06:07:59.0713930 +0000");
		F(fmt::format("{:L}", now), "2023-11-12 14:07:59.0713930 +0800");
    } else {
        F(fmt::format("{}", now), "2023-11-12 06:07:59.071393000 +0000");
        F(fmt::format("{:L}", now), "2023-11-12 14:07:59.071393000 +0800");
    }
    F(fmt::format("{:%F %T %Z}", now), "2023-11-12 06:07:59 " UTC);
    F(fmt::format("{:%F %T %z}", now), "2023-11-12 06:07:59 +0000");
    F(fmt::format("{:L%F %T %z}", now), "2023-11-12 14:07:59 +0800");
#ifdef WIN32
    F(fmt::format("{:%c}", now), "11/12/23 06:07:59");
#else
    F(fmt::format("{:%c}", now), "Sun Nov 12 06:07:59 2023");
#endif
#if !defined(_MSC_VER) || defined(_DEBUG)
    // vc下release模式编译的程序%Z输出的时区名乱码，微软知道该bug，但似乎并不着急解决
    F(fmt::format("{:L%F %T %Z}", now), "2023-11-12 14:07:59 " CST);
    F(fmt::format("{:L%F %T.000 %Z}", now), "2023-11-12 14:07:59.071 " CST);
    F(fmt::format("{:%F %T %Z}", fmt::localtime(to_time_t(now))), "2023-11-12 14:07:59 " CST);
#if BOOST_VERSION >= 108100
    F((sout{} << bc::time_fmt(bc::timezone::local, "%F %T %Z") << bNow).str(), "2023-11-12 14:07:59 " CST);
#endif // #if BOOST_VERSION >= 108100
    F(date::format(now, "L%F %T %Z"), "2023-11-12 14:07:59 " CST);
    F(date::format(now, "L%F %T %Z"), "2023-11-12 14:07:59 " CST);
    F(date::format(now, "L%F %T.000000 %Z"), "2023-11-12 14:07:59.071393 " CST);
#endif
    F(fmt::format("{:L%F %T %z}", now), "2023-11-12 14:07:59 +0800");
    F(fmt::format("{:L%F %T.000000 %z}", now), "2023-11-12 14:07:59.071393 +0800");
    F(fmt::format("{:%a, %d %b %Y %H:%M:%S} GMT", now), "Sun, 12 Nov 2023 06:07:59 GMT");
    F(fmt::format("{:%F %T %z}", fmt::localtime(to_time_t(now))), "2023-11-12 14:07:59 +0800");

    F((sout{} << bNow).str(), "2023-11-12 06:07:59.071393000 +0000");
#if BOOST_VERSION >= 108100
    F((sout{} << bc::time_fmt(bc::timezone::utc, "%F %T") << bNow).str(), "2023-11-12 06:07:59");
    F((sout{} << bc::time_fmt(bc::timezone::utc) << bNow).str(), "2023-11-12 06:07:59.071393000 +0000");
    F((sout{} << bc::time_fmt(bc::timezone::utc, "") << bNow).str(), "2023-11-12 06:07:59.071393000 +0000");
    F((sout{} << bc::time_fmt(bc::timezone::local, "") << bNow).str(), "2023-11-12 14:07:59.071393000 +0800");
    F((sout{} << bc::time_fmt(bc::timezone::local, "%F %T") << bNow).str(), "2023-11-12 14:07:59");
#endif // #if BOOST_VERSION >= 108100

    F(date::format(now, "%F %T %Z"), "2023-11-12 06:07:59 " UTC);
    F(date::format(now, "%F %T %z"), "2023-11-12 06:07:59 +0000");
    F(date::format(now, "L%F %T %z"), "2023-11-12 14:07:59 +0800");
    F(date::format(now, "%F %T %z, %a"), "2023-11-12 06:07:59 +0000, Sun");

    F(date::format(now, 0), "2023-11-12 14:07:59");
    F(date::format(now, 3), "2023-11-12 14:07:59.071");
    F(date::format(now, 6), "2023-11-12 14:07:59.071393");
    if (DurationFractionWidth<decltype(now)::duration>::width == 6) {
        F(date::format(now, 9), "2023-11-12 14:07:59.071393");
    } else if (DurationFractionWidth<decltype(now)::duration>::width == 7) {
        F(date::format(now, 9), "2023-11-12 14:07:59.0713930");
    } else {
        F(date::format(now, 9), "2023-11-12 14:07:59.071393000");
    }

    if (DurationFractionWidth<decltype(now)::duration>::width == 6) {
        F(date::format(now, "L"), "2023-11-12 14:07:59.071393 +0800");
    } else if (DurationFractionWidth<decltype(now)::duration>::width == 7) {
		F(date::format(now, "L"), "2023-11-12 14:07:59.0713930 +0800");
	} else {
        F(date::format(now, "L"), "2023-11-12 14:07:59.071393000 +0800");
    }
    F(date::format(now, "L%F %T.000 %z"), "2023-11-12 14:07:59.071 +0800");
    // if (DurationFractionWidth<decltype(bNow)::duration>::width == 6) {
    //     F(date::format(bNow, "L%F %T.000000000 %z"), "2023-11-12 14:07:59.071393 +0800");
    // } else if (DurationFractionWidth<decltype(bNow)::duration>::width == 7) {
	// 	F(date::format(bNow, "L%F %T.000000000 %z"), "2023-11-12 14:07:59.0713930 +0800"); // windows
	// } else {
	// 	F(date::format(bNow, "L%F %T.000000000 %z"), "2023-11-12 14:07:59.071393000 +0800");
	// }
    F(date::format(now, "%F %T.000"), "2023-11-12 06:07:59.071");
    F(date::format(now, "%F %T.000"), "2023-11-12 06:07:59.071");
    F(date::format(now, "%F %T.000000"), "2023-11-12 06:07:59.071393");
    if (DurationFractionWidth<decltype(now)::duration>::width == 6) {
        F(date::format(now, "%F %T.000000000"), "2023-11-12 06:07:59.071393");
	} else if (DurationFractionWidth<decltype(now)::duration>::width == 7) {
		F(date::format(now, "%F %T.000000000"), "2023-11-12 06:07:59.0713930");
	} else {
        F(date::format(now, "%F %T.000000000"), "2023-11-12 06:07:59.071393000");
    }

#undef F
}

TEST(CommonChronoUtilTest, FractionWidth) {
    EXPECT_EQ(0, DurationFractionWidth<std::chrono::minutes>::width);
    EXPECT_EQ(0, DurationFractionWidth<boost::chrono::minutes>::width);

    EXPECT_EQ(0, DurationFractionWidth<std::chrono::seconds>::width);
    EXPECT_EQ(0, DurationFractionWidth<boost::chrono::seconds>::width);

    EXPECT_EQ(3, DurationFractionWidth<std::chrono::milliseconds>::width);
    EXPECT_EQ(3, DurationFractionWidth<boost::chrono::milliseconds>::width);

    EXPECT_EQ(6, DurationFractionWidth<std::chrono::microseconds>::width);
    EXPECT_EQ(6, DurationFractionWidth<boost::chrono::microseconds>::width);

    LogInfo("std::chrono::system_clock::duration fraction width: {}",
            DurationFractionWidth<std::chrono::system_clock::duration>::width);
    // LogInfo("boost::chrono::system_clock::duration fraction width: {}",
    //         DurationFractionWidth<boost::chrono::system_clock::duration>::width);
}

TEST(CommonChronoUtilTest, TestPutTimeWithTZ) {
    auto now = std::chrono::system_clock::now();
#define F(PATTERN) std::cout << __LINE__ << ": " << #PATTERN << " => " << date::format(now, PATTERN) << std::endl
    F("");
    F("%Z");
    F("%z");
    F("L%Z");
    F("L%z");
    F("%F %T %Z");
    F("L%F %T %Z");
    F("%F %T %Z");
    F("L%F %T %Z");
    F("%F %T.000 %Z");
    F("L%F %T.000 %Z");
#undef F
}

TEST(CommonChronoUtilTest, DurationOperator) {
    EXPECT_EQ(std::chrono::seconds{3}, 3_s);
    EXPECT_EQ(std::chrono::milliseconds{3}, 3_ms);
    EXPECT_EQ(std::chrono::microseconds{3}, 3_us);
    EXPECT_EQ(std::chrono::minutes{3}, 3_min);
    EXPECT_EQ(std::chrono::hours{3}, 3_h);
}

TEST(CommonChronoUtilTest, TemplateNow) {
    std::cout << "std::chrono" << std::endl;
    std::cout << std::chrono::Now<ByMicros>() << std::endl;
    std::cout << std::chrono::Now<ByMillis>() << std::endl;
    std::cout << std::chrono::Now<BySeconds>() << std::endl;

    std::cout << "boost::chrono" << std::endl;
    std::cout << boost::chrono::Now<ByMicros>() << std::endl;
    std::cout << boost::chrono::Now<ByMillis>() << std::endl;
    std::cout << boost::chrono::Now<BySeconds>() << std::endl;
}
