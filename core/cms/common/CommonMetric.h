#ifndef ARGUS_COMMON_METRIC_H
#define ARGUS_COMMON_METRIC_H

#include <map>
#include <unordered_map>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>

#include "common/TimeFormat.h"

namespace common {
    template<typename TDuration>
    struct CommonMetricT;

    struct CommonMetricBase {
        std::string name;
        double value = 0;
        std::map<std::string, std::string> tagMap;

        virtual ~CommonMetricBase() = 0;
        virtual std::string toString(int tabNum) const = 0;
        std::string toString() const {
            return toString(0);
        }
    };

    struct CommonMetric : public CommonMetricBase {
        int64_t timestamp = 0;

        CommonMetric() = default;
        CommonMetric(const CommonMetric &) = default;
        CommonMetric &operator=(const CommonMetric &) = default;

        template<typename TDuration>
        explicit CommonMetric(const CommonMetricT<TDuration> &);

        using CommonMetricBase::toString;
        std::string toString(int tabNum) const override;
    };

    template<class Rep, class Period>
    struct Timestamp: public std::chrono::duration<Rep, Period> {
        typedef std::chrono::duration<Rep, Period> Type;

        template<typename Rep2, typename Period2>
        Timestamp &operator=(const std::chrono::duration<Rep2, Period2> &r) {
            *(Type *)(this) = std::chrono::duration_cast<Type>(r);
            return *this;
        }

        Timestamp &operator=(const std::chrono::system_clock::time_point &tp) {
            *(Type *)(this) = std::chrono::duration_cast<Type>(tp.time_since_epoch());
            return *this;
        }

        std::chrono::system_clock::time_point toTimePoint() const {
            using namespace std::chrono;
            return system_clock::time_point{duration_cast<system_clock::duration>(*this)};
        }
    };

    template<typename TDuration>
    struct CommonMetricT: public CommonMetricBase {
        Timestamp<typename TDuration::rep, typename TDuration::period> timestamp;

        using CommonMetricBase::toString;
        std::string toString(int tabNum) const override {
            std::stringstream ss;
            ss << this->CommonMetricBase::toString(tabNum) << ' ' << timestamp.count();
            return ss.str();
        }
    };
    typedef CommonMetricT<std::chrono::seconds> SCommonMetric;

    template<typename TDuration>
    CommonMetric::CommonMetric(const CommonMetricT<TDuration> &r) {
        *(CommonMetricBase *)(this) = (const CommonMetricBase &)(r);
        this->timestamp = r.timestamp.count();
    }

    /// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct ResultMap {
        std::chrono::system_clock::time_point ts; // 秒级时间戳
        std::unordered_map<std::string, std::string> stringMap;
    };

    struct RawData {
        std::string type;
        std::vector<ResultMap> resultMaps;
        std::map<std::string, std::string> labelMap;
    };
}
#endif
