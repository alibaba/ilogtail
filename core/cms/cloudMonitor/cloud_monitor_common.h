#ifndef _CLOUD_MONITOR_COMMON_H_
#define _CLOUD_MONITOR_COMMON_H_

#include <cstdint>
#include "sic/system_information_collector.h"

namespace cloudMonitor {
    template<typename T>
    double GetRatio(const T &prev, const T &curr, double interval) {
        auto delta = static_cast<double>(prev > curr ? prev - curr : 0);
        return interval == 0 ? 0.0 : (delta / interval);
    }

    // (prev - curr)/interval
    template<typename T_From>
    void GetRatios(const NetInterfaceMetric<T_From> &prev, const NetInterfaceMetric<T_From> &curr, double interval,
                   NetInterfaceMetric<double> &result) {
        const size_t size = sizeof(prev) / sizeof(T_From);
        auto const pPrev = reinterpret_cast<const T_From *>(&prev);
        auto const pCurr = reinterpret_cast<const T_From *>(&curr);

        auto ratio = (double *) (&result);

        for (size_t i = 0; i < size; i++) {
            ratio[i] = GetRatio(pPrev[i], pCurr[i], interval);
        }
    }
}
#endif
