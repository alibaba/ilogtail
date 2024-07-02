//
// Created by 韩呈杰 on 2023/9/14.
//
#include "common/Chrono.h"

#include <iostream>
#include <vector>
#include <cctype> // std::isspace
#include <map>
#include <fmt/chrono.h>

#include <boost/date_time/posix_time/posix_time.hpp> // windows下需要这个头文件
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/chrono/chrono_io.hpp>
#include <boost/chrono/io/ios_base_state.hpp>

#include "common/CppTimer.h"

#ifdef min
#   undef min
#endif
#include <algorithm>

using namespace std::placeholders;

std::ostream &operator<<(std::ostream &os, const std::chrono::system_clock::time_point &s) {
    namespace bc = boost::chrono;
    std::string timeFmt = bc::get_time_fmt<char>(os);
    bc::timezone tz = bc::get_timezone(os);
    if (tz == bc::timezone::local || timeFmt.empty()) {
        timeFmt = "L" + timeFmt;
    }
    return os << date::format(s, timeFmt.c_str());
}

std::chrono::seconds getLocalTimeOffsetNow() {
    // // gmtime 使用全局变量做为返回值，有并发问题
    // time_t t = 0;
    // tm* tmGMT = gmtime(&t);
    // tmGMT->tm_isdst = false;
    // time_t newTimeGMT = mktime(tmGMT);
    // // local time + localTimeOffset = UTC time
    // time_t localTimeOffset = t - newTimeGMT;
    // return static_cast<int>(localTimeOffset);

    using namespace boost::posix_time;
    // boost::date_time::c_local_adjustor uses the C-API to adjust a
    // moment given in utc to the same moment in the local time zone.
    typedef boost::date_time::c_local_adjustor<ptime> local_adj;

    const ptime utc_now = second_clock::universal_time();
    const ptime now = local_adj::utc_to_local(utc_now);

    auto duration = now - utc_now;
    return std::chrono::seconds(duration.total_seconds());
}

struct tagTzName {
    std::string Z;  // %Z, CST
    std::string z;  // %z, +0800

    tagTzName() = default;

    explicit tagTzName(const std::tm &tm) {
        struct {
            char c;
            std::string &s;
        } flags[] = {
                {'z', z},
                {'Z', Z},
        };
        for (auto &flag: flags) {
            std::stringstream os;
            char tzFmt[] = {'%', flag.c};
            auto &tpf = std::use_facet<std::time_put<char> >(os.getloc());
            tpf.put(os, os, os.fill(), &tm, tzFmt, tzFmt + sizeof(tzFmt));
            flag.s = os.str();
        }
    }
};

struct tagTimeZoneInfo {
    std::chrono::seconds offset{0};
    tagTzName utc;
    tagTzName local;

    explicit tagTimeZoneInfo(bool = true);
};
// localTimeCache不能是带有析构的类型，以避免在main函数退出后，直接释放
constexpr const uintptr_t PingPongSize = 2;
static tagTimeZoneInfo *localTimeCache = new tagTimeZoneInfo[PingPongSize];
static uintptr_t localTimeCacheIndex = 0;

tagTimeZoneInfo::tagTimeZoneInfo(bool doInit) {
    if (doInit) {
        offset = getLocalTimeOffsetNow();

        using namespace std::chrono;
        std::time_t time_t = system_clock::to_time_t(system_clock::now());
        local = tagTzName(fmt::localtime(time_t));
        utc = tagTzName(fmt::gmtime(time_t));
        utc.z = "+0000";  // 标准库有bug, utc模式下无法正确输出时间偏移量，此处只能固化
#if defined(_MSC_VER)
        // Win32 Release output unreadable code
        utc.Z = "UTC";
#endif
    }
}

static std::once_flag localTimeTimerOnce;
static CppTime::timer_id localTimeTimerId = 0;
void StartLocalTimeTimer() {
    std::call_once(localTimeTimerOnce, []() {
        localTimeTimerId = SingletonTimer::Instance()->add(10_s, [](CppTime::timer_id) {
            const uintptr_t newIndex = (localTimeCacheIndex + 1) % PingPongSize;
            localTimeCache[newIndex] = tagTimeZoneInfo(true);
            localTimeCacheIndex  = newIndex;
        }, 10_s);
    });
}
void StopLocalTimeTimer() {
    if (localTimeTimerId) {
        SingletonTimer::Instance()->remove(localTimeTimerId);
        localTimeTimerId = 0;
    }
}

std::chrono::seconds getLocalTimeOffset() {
    return localTimeCache->offset;
}

template<typename T>
int64_t NowUtcTimestamp() {
    // timeval time{};
    // gettimeofday(&time, nullptr);
    // return ((time.tv_sec * MICROSECOND) + time.tv_usec) / MILLISECOND;
    auto now = std::chrono::system_clock::now();
    return std::chrono::duration_cast<T>(now.time_since_epoch()).count();
}
int64_t NowMicros() {
    return NowUtcTimestamp<std::chrono::microseconds>();
}

// 获取当前毫秒级utc时间戳
int64_t NowMillis() {
    return NowUtcTimestamp<std::chrono::milliseconds>();
}

int64_t NowSeconds() {
    return NowUtcTimestamp<std::chrono::seconds>();
}

// namespace boost {
//     namespace chrono {
//         system_clock::time_point FromSeconds(int64_t secs) {
//             return system_clock::time_point(seconds{secs});
//         }
//
//         system_clock::time_point FromMillis(int64_t millis) {
//             return system_clock::time_point{milliseconds{millis}};
//         }
//
//         system_clock::time_point FromMicros(int64_t micros) {
//             return system_clock::time_point{microseconds{micros}};
//         }
//     }
// }
namespace std {
    namespace chrono {
        system_clock::time_point FromSeconds(int64_t secs) {
            return system_clock::time_point(seconds{secs});
        }

        system_clock::time_point FromMillis(int64_t millis) {
            return system_clock::time_point{milliseconds{millis}};
        }

        system_clock::time_point FromMicros(int64_t micros) {
            return system_clock::time_point{microseconds{micros}};
        }
    }
}

int64_t ToMillis(const std::chrono::system_clock::time_point &t) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch()).count();
}

int64_t ToMicros(const std::chrono::system_clock::time_point &t) {
    return std::chrono::duration_cast<std::chrono::microseconds>(t.time_since_epoch()).count();
}

bool IsZero(const std::chrono::system_clock::time_point &t) {
    return t.time_since_epoch().count() == 0;
}
bool IsZero(const std::chrono::steady_clock::time_point &t) {
    return t.time_since_epoch().count() == 0;
}

namespace fmt {
    namespace argus {
        format_parse_context::iterator parse(format_parse_context &ctx, std::string &pattern) {
            // [ctx.begin(), ctx.end()) is a character range that contains a part of
            // the format string starting from the format specifications to be parsed,
            // e.g. in
            //
            //   fmt::format("{:f} - point of interest", point{1, 2});
            //
            // the range will contain "f} - point of interest". The formatter should
            // parse specifiers until '}' or the end of the range. In this example
            // the formatter should parse the 'f' specifier and return an iterator
            // pointing to '}'.

            // Please also note that this character range may be empty, in case of
            // the "{}" format string, so therefore you should check ctx.begin()
            // for equality with ctx.end().

            // Parse the presentation format and store it in the formatter:
            const char *begin = ctx.begin();
            const char *end = ctx.end();
            const char *it = begin;
            for (; it < end && *it != '}'; ++it) {
            }
            // auto it = ctx.begin(), end = ctx.end();
            // if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;
            //
            //
            // // Check if reached the end of the range:
            // if (it != end && *it != '}') {
            //     detail::throw_format_error("invalid time_point format");
            // }

            pattern = std::string(begin, it);

            // Return an iterator past the end of the parsed range:
            return it;// == end? end: it + 1;
        }
    }
}
namespace date {
    // 很奇怪，utc下%z居然输出+0800
// #define USE_STD_TZ

    static std::chrono::fraction_seconds get_fraction_seconds(const std::chrono::system_clock::time_point &tp) {
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch());
        return std::chrono::duration_cast<std::chrono::fraction_seconds>(tp.time_since_epoch() - seconds);
    }

    // static std::chrono::fraction_seconds get_fraction_seconds(const boost::chrono::system_clock::time_point &tp) {
    //     auto seconds = boost::chrono::duration_cast<boost::chrono::seconds>(tp.time_since_epoch());
    //     return std::chrono::duration_cast<std::chrono::fraction_seconds>(tp.time_since_epoch() - seconds);
    // }

    template<typename T>
    class TimeFormatter {
        struct Runtime {
            tagTimeZoneInfo tzInfo{false};

            const tagTimeZoneInfo &getTimeZone() {
                if (tzInfo.utc.z.empty()) {
                    tzInfo = localTimeCache[localTimeCacheIndex];
                }
                return tzInfo;
            }
        };

        typedef typename T::time_point time_point;
        typedef std::function<void(std::ostream &, const time_point &, TimeFormatter::Runtime &)> FnFormat;

        bool local = false;
        // decltype((char *) nullptr - (char *) nullptr) fractionCount = 9;
        std::vector<FnFormat> vecFormatter;

        const char *parseFractionSeconds(const char *&begin, size_t &spaceCount, const char *c) {
            appendParse(begin, spaceCount, c);

            const char *fractionPos = c;
            const char *fractionEnd = fractionPos + 1;
            for (; *fractionEnd == '0'; ++fractionEnd) {
            }
            size_t fractionCount = fractionEnd - fractionPos - 1;

            vecFormatter.emplace_back(std::bind(&TimeFormatter::putFractionSeconds, _1, _2, fractionCount));
            spaceCount = 0;
            begin = fractionEnd;

            return fractionEnd;
        }

        static void putFractionSeconds(std::ostream &os, const time_point &tp, size_t fractionCount) {
            int fractionWidth = std::min(static_cast<int>(fractionCount),
                                         DurationFractionWidth<typename T::duration>::width);
            auto v = get_fraction_seconds(tp).count();
            std::string s = fmt::format("{:.{}f}", v, fractionWidth); // 0.017
            os << s.substr(1);  // .017
        }

        static void putTZ(std::ostream &os, char flag, Runtime &rt, bool local) {
            const auto &tz = rt.getTimeZone();
            if (local) {
                os << (flag == 'z' ? tz.local.z : tz.local.Z);
            } else {
                os << (flag == 'z' ? tz.utc.z : tz.utc.Z);
            }
        }

        static void putTime(std::ostream &os, const time_point &tp, Runtime &rt,
                            const std::string &pattern, bool local) {
            std::time_t time_t = T::to_time_t(tp);
#ifdef USE_STD_TZ
            std::tm tm{};
            if (local) {
                tm = fmt::localtime(time_t);
            } else {
                tm = fmt::gmtime(time_t);
                // tm.tm_isdst = -1;
                // mktime(&tm);
            }
#else
            if (local) {
                time_t += ToSeconds(rt.getTimeZone().offset);
            }
            std::tm tm = fmt::gmtime(time_t);
#endif

            const auto &tpf = std::use_facet<std::time_put<char> >(os.getloc());
            if (!pattern.empty()) {
                tpf.put(os, os, os.fill(), &tm, pattern.c_str(), pattern.c_str() + pattern.size());
            } else {
                // dt: date time
                char dtFmt[] = {'%', 'F', ' ', '%', 'T'};
                tpf.put(os, os, os.fill(), &tm, dtFmt, dtFmt + sizeof(dtFmt));
                putFractionSeconds(os, tp, 9); // 纳秒
#ifdef USE_STD_TZ
                char tzFmt[] = {' ', '%', 'z'};
                tpf.put(os, os, os.fill(), &tm, tzFmt, tzFmt + sizeof(tzFmt));
#else
                os << ' ';
                putTZ(os, 'z', rt, local); // 时区
#endif
            }
        }

        void appendParse(const char *&begin, size_t &spaceCount, const char *end) {
            if (begin != end) {
                std::string format = {begin, end};
                if (spaceCount == format.size()) {
                    vecFormatter.emplace_back([format](std::ostream &os, const time_point &, Runtime &) {
                        os << format;
                    });
                } else {
                    // vecFormatter.emplace_back([format](std::ostream &os, const bc::system_clock::time_point &tp) {
                    //     // os << bc::time_fmt(tz_, format) << tp;
                    //     putTime(os, tp, format);
                    // });
                    vecFormatter.emplace_back(std::bind(&TimeFormatter::putTime, _1, _2, _3, format, local));
                }

                begin = end;
                spaceCount = 0;
            }
        };

#if !defined(USE_STD_TZ)

        const char *appendTZ(const char *&begin, size_t &spaceCount, const char *c) {
            appendParse(begin, spaceCount, c);

            char flag = c[1];
            bool isLocal = this->local;
            vecFormatter.emplace_back([isLocal, flag](std::ostream &os, const time_point &, Runtime &rt) {
                putTZ(os, flag, rt, isLocal);
            });
            spaceCount = 0;
            begin = c + 2;
            return begin;
        }

#endif

        void appendDefault() {
            bool isLocal = this->local;
            vecFormatter.emplace_back([isLocal](std::ostream &os, const time_point &tp, Runtime &rt) {
                putTime(os, tp, rt, "", isLocal);
                // bc::time_fmt有无参版，但该版本有bug, pattern未复位，而是复用了上次的p
                // os << bc::time_fmt(timeZone, "") << tp;
            });
        }

        void doParse(const char *begin) {
            size_t spaceCount = 0;
            const char *c = begin;
            while (c && *c) {
                if (c[0] == '%') {
                    if (c[1] == 'T' || c[1] == 'S') {
                        if (c[2] == '.' && c[3] == '0') {
                            c = parseFractionSeconds(begin, spaceCount, c + 2); // skip %T or %S
                            continue;
                        }
                    }
#if !defined(USE_STD_TZ)
                    else if (c[1] == 'z' || c[1] == 'Z') {
                        c = appendTZ(begin, spaceCount, c);
                        continue;
                    }
#endif
                    else if (c[1] == '%') {
                        // %%
                        c += 2;
                        continue;
                    }
                }

                if (std::isspace(*(unsigned char *) c)) {
                    spaceCount++;
                }
                ++c;
            }
            appendParse(begin, spaceCount, c);

            if (vecFormatter.empty()) {
                appendDefault();
            }
        }

    public:
        explicit TimeFormatter(const char *pattern) {
            this->local = (pattern != nullptr && *pattern == 'L');
            this->vecFormatter.clear();

            doParse(pattern + (this->local ? 1 : 0));
        }

        std::string format(const time_point &tp) const {
            Runtime runtime;
            std::stringstream os;
            for (const auto &f: vecFormatter) {
                f(os, tp, runtime);
            }
            return os.str();
        }
    };

    // std::string format(const boost::chrono::system_clock::time_point &tp, const char *pattern) {
    //     return TimeFormatter<boost::chrono::system_clock>{pattern}.format(tp);
    // }

    std::string format(const std::chrono::system_clock::time_point &tp, const char *pattern) {
        return TimeFormatter<std::chrono::system_clock>{pattern}.format(tp);
    }

    // boost实际调用了这个: https://en.cppreference.com/w/cpp/locale/time_put/put
    // See: https://www.boost.org/doc/libs/1_81_0/doc/html/chrono/reference.html#chrono.reference.io.ios_state_hpp.sag.set_time_fmt
    // https://www.boost.org/doc/libs/1_81_0/doc/html/chrono/users_guide.html#chrono.users_guide.tutorial.i_o.system_clock_time_point_io
    // Unfortunately there are no formatting/parsing sequences which indicate fractional seconds. Boost.Chrono does not provide such sequences.
    // In the meantime, one can format and parse fractional seconds for system_clock::time_point by defaulting the format, or by using an empty
    // string in time_fmt().

    static constexpr const char * const fullFormat = "L%F %T.000000000";
    static constexpr int fractionDotPos = 6;
    static constexpr int fractionLen = 9;
    static constexpr int fractionEnd = fractionDotPos + 1 + fractionLen;
    static_assert(fullFormat[fractionDotPos] == '.', "fractionDotPos invalid");
    static_assert(fullFormat[fractionEnd] == 0, "fractionEnd invalid");

    class tagFormatterKey{
        bool local;
        int fractionWidth;
    public:
        tagFormatterKey() = default;
        tagFormatterKey(bool l, int n): local(l), fractionWidth(n) {
        }

        tagFormatterKey(const tagFormatterKey &) = default;
        tagFormatterKey &operator=(const tagFormatterKey &) = default;

        std::string pattern() const {
            int start = (local ? 0 : 1);
            int width = (fractionWidth > 0? std::min(fractionLen, fractionWidth): fractionWidth);
            int end = (width == 0 ? fractionDotPos : fractionDotPos + 1 + width);
            return {fullFormat + start, fullFormat + end};
        }

        bool operator<(const tagFormatterKey &r) const {
            return local != r.local ? local : (fractionWidth < r.fractionWidth);
        }
    };

    template<typename T>
    static std::pair<tagFormatterKey, TimeFormatter<T>> makeTF(bool local, int fractionWidth) {
        const auto key = tagFormatterKey(local, fractionWidth);
        return {key, TimeFormatter<T>(key.pattern().c_str())};
    }

#define FORMAT_ENTRIES(NS)    \
        makeTF<NS>(true, 0),  \
        makeTF<NS>(true, 1),  \
        makeTF<NS>(true, 2),  \
        makeTF<NS>(true, 3),  \
        makeTF<NS>(true, 4),  \
        makeTF<NS>(true, 5),  \
        makeTF<NS>(true, 6),  \
        makeTF<NS>(true, 7),  \
        makeTF<NS>(true, 8),  \
        makeTF<NS>(true, 9),  \
        makeTF<NS>(false, 0), \
        makeTF<NS>(false, 1), \
        makeTF<NS>(false, 2), \
        makeTF<NS>(false, 3), \
        makeTF<NS>(false, 4), \
        makeTF<NS>(false, 5), \
        makeTF<NS>(false, 6), \
        makeTF<NS>(false, 7), \
        makeTF<NS>(false, 8), \
        makeTF<NS>(false, 9)

    static const auto stdFormatter = new std::map<tagFormatterKey, TimeFormatter<std::chrono::system_clock>>{
            FORMAT_ENTRIES(std::chrono::system_clock)
    };

    // static const auto *boostFormatter = new std::map<tagFormatterKey, TimeFormatter<boost::chrono::system_clock>>{
    //         FORMAT_ENTRIES(boost::chrono::system_clock)
    // };

#undef FORMAT_ENTRIES

    template<typename T>
    std::string formatT(const std::map<tagFormatterKey, TimeFormatter<T>> &tlsFormatter,
                        const typename T::time_point &tp, int fractionWidth, bool local) {
        const auto key = tagFormatterKey(local, fractionWidth);
        const auto it = tlsFormatter.find(key);
        if (it != tlsFormatter.end()) {
            return it->second.format(tp);
        }
        return TimeFormatter<T>(key.pattern().c_str()).format(tp);
    }

    std::string format(const std::chrono::system_clock::time_point &tp, int fractionWidth, bool local) {
        return formatT(*stdFormatter, tp, fractionWidth, local);
    }

    // std::string format(const boost::chrono::system_clock::time_point &tp, int fractionWidth, bool local) {
    //     return formatT(*boostFormatter, tp, fractionWidth, local);
    // }
}
