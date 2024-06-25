//
// Created by 韩呈杰 on 2023/3/30.
//

#ifndef ARGUS_COMMON_TIME_FORMAT_H
#define ARGUS_COMMON_TIME_FORMAT_H

#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

// #include <boost/chrono/chrono_io.hpp>
// #include <boost/chrono/io/ios_base_state.hpp>

#include <fmt/ostream.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

template<typename TRatio, typename std::enable_if<(TRatio::den < TRatio::num || TRatio::den < 10), int>::type = 0>
constexpr int FractionSecondWidth() {
    return 0;
}

template<typename TRatio, typename std::enable_if<(TRatio::den >= TRatio::num && TRatio::den >= 10), int>::type = 0>
constexpr int FractionSecondWidth() {
    return 1 + FractionSecondWidth<std::ratio<TRatio::num, TRatio::den / 10>>();
}

template<typename TDuration>
struct DurationFractionWidth {
    static constexpr int width = FractionSecondWidth<typename TDuration::period>();
};
template<typename TDuration> constexpr int DurationFractionWidth<TDuration>::width;


namespace date {
    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    struct WidthTraits {
        static constexpr int width = ::DurationFractionWidth<T>::width;
    };
    template<typename T> constexpr int WidthTraits<T>::width;

    std::string format(const std::chrono::system_clock::time_point &tp, int fractionWidth, bool local = true);
    std::string format(const std::chrono::system_clock::time_point &tp, const char *pattern);
#ifdef WIN32
    static std::string format(const std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> &tp, const char *pattern) {
        using namespace std::chrono;
        return format(time_point_cast<system_clock::duration>(tp), pattern);
    }
#endif

    template<int FractionWidth>
    std::string format(const std::chrono::system_clock::time_point &tp, bool local = true) {
        return format(tp, FractionWidth, local);
    }
}

template<typename Rep, typename Period>
std::ostream &operator<<(std::ostream &os, const std::chrono::duration<Rep, Period> &s) {
    return os << fmt::format("{}", s);
}

std::ostream &operator<<(std::ostream &os, const std::chrono::system_clock::time_point &s);
// static inline std::ostream &operator<<(std::ostream &os, const boost::chrono::system_clock::time_point &s) {
//     return os << date::format(s, "L");
// }

namespace fmt {
    // template<typename Rep, typename Period>
    // struct formatter<boost::chrono::duration<Rep, Period>> : ostream_formatter {
    // };

    namespace argus {
        format_parse_context::iterator parse(format_parse_context &ctx, std::string &pattern);
    }

#define IMPL_FORMATTER(T)                                                                      \
    template<>                                                                                 \
    struct formatter<T>: formatter<std::string> {                                              \
        std::string pattern;                                                                   \
        auto parse(format_parse_context& ctx) -> format_parse_context::iterator {              \
            return argus::parse(ctx, pattern);                                                 \
        }                                                                                      \
                                                                                               \
        auto format(const T &a, format_context &ctx) const -> format_context::iterator {       \
            return fmt::formatter<std::string>::format(date::format(a, pattern.c_str()), ctx); \
        }                                                                                      \
    }

    IMPL_FORMATTER(std::chrono::system_clock::time_point);
    // IMPL_FORMATTER(boost::chrono::system_clock::time_point);
#ifdef WIN32
    typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> NanoTimePoint;
    IMPL_FORMATTER(NanoTimePoint);
#endif

#undef IMPL_FORMATTER

}

#endif //!ARGUS_COMMON_TIME_FORMAT_H
