//
// Created by 韩呈杰 on 2023/9/14.
//

#ifndef ARGUSAGENT_COMMON_CHRONO_H
#define ARGUSAGENT_COMMON_CHRONO_H

#include <cstdint>
#include "ChronoLiteral.h"
#include "TimeFormat.h"

// 获取当前微秒级utc时间戳
int64_t NowMicros();
int64_t NowMillis();
int64_t NowSeconds();

enum EnumTimePointPrecision {
    // 微秒
    ByMicros,
    // 毫秒
    ByMillis,
    // 秒
    BySeconds
};

#define ImplementChronoFunc                                                     \
    template<typename TargetDuration>                                           \
    system_clock::time_point Now() {                                            \
        system_clock::time_point nowTime = system_clock::now();                 \
        return time_point_cast<TargetDuration>(nowTime);                        \
    }                                                                           \
    template<EnumTimePointPrecision T = EnumTimePointPrecision::ByMicros>       \
    inline system_clock::time_point Now() {                                     \
        return Now<microseconds>();                                             \
    }                                                                           \
    template<>                                                                  \
    inline system_clock::time_point Now<EnumTimePointPrecision::ByMillis>() {   \
        return Now<milliseconds>();                                             \
    }                                                                           \
    template<>                                                                  \
    inline system_clock::time_point Now<EnumTimePointPrecision::BySeconds>() {  \
        return Now<seconds>();                                                  \
    }

namespace std {
    namespace chrono {
        typedef duration<double> fraction_seconds;
        typedef duration<double, std::milli> fraction_millis;
        typedef duration<double, std::micro> fraction_micros;

        system_clock::time_point FromSeconds(int64_t seconds);
        system_clock::time_point FromMillis(int64_t millis);
        system_clock::time_point FromMicros(int64_t micros);

        ImplementChronoFunc
    }
}
using std::chrono::FromMicros;
using std::chrono::FromMillis;
using std::chrono::FromSeconds;

#define IMPL_ALIGN(NAMESPACE)                                                                      \
    auto tDura = NAMESPACE::duration_cast<NAMESPACE::duration<Req, Period>>(t.time_since_epoch()); \
    tDura -= tDura % interval;                                                                     \
    if (multiply != 0) {                                                                           \
        tDura += interval * multiply;                                                              \
    }                                                                                              \
    return NAMESPACE::system_clock::time_point{tDura}

template<typename Req, typename Period>
std::chrono::system_clock::time_point Align(const std::chrono::system_clock::time_point &t,
                                            const std::chrono::duration<Req, Period> &interval,
                                            int multiply = 0) {
    IMPL_ALIGN(std::chrono);
}

// #undef IMPL_ALIGN

template<typename T = int64_t>
T ToSeconds(const std::chrono::system_clock::time_point &t) {
    using namespace std::chrono;
    return static_cast<T>(duration_cast<seconds>(t.time_since_epoch()).count());
}
int64_t ToMillis(const std::chrono::system_clock::time_point &);
int64_t ToMicros(const std::chrono::system_clock::time_point &);
bool IsZero(const std::chrono::system_clock::time_point &);
bool IsZero(const std::chrono::steady_clock::time_point &);
template<typename Rep, typename Period>
bool IsZero(const std::chrono::duration<Rep, Period> &d) {
    return d.count() == 0;
}

template<typename Rep, typename Period>
int64_t ToSeconds(const std::chrono::duration<Rep, Period> &t) {
    using namespace std::chrono;
    return duration_cast<seconds>(t).count();
}
inline int64_t ToSeconds(const std::chrono::seconds &t) {
    return t.count();
}

template<typename Rep, typename Period>
int64_t ToMillis(const std::chrono::duration<Rep, Period> &t) {
    using namespace std::chrono;
    return duration_cast<milliseconds>(t).count();
}
inline int64_t ToMillis(const std::chrono::milliseconds &t) {
    return t.count();
}

template<typename Rep, typename Period>
int64_t ToMicros(const std::chrono::duration<Rep, Period> &t) {
    using namespace std::chrono;
    return duration_cast<microseconds>(t).count();
}
inline int64_t ToMicros(const std::chrono::microseconds &t) {
    return t.count();
}

// 计算当前时区偏移(从缓存中取)
std::chrono::seconds getLocalTimeOffset();

template<typename Rep, typename Period>
std::chrono::duration<Rep, Period> getLocalTimeOffset()  {
    return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(getLocalTimeOffset());
}

// 立即计算当前时区偏移
std::chrono::seconds getLocalTimeOffsetNow();

void StartLocalTimeTimer();
void StopLocalTimeTimer();

#endif //ARGUSAGENT_SIC_CHRONO_H
