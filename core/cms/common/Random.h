//
// Created by 韩呈杰 on 2023/2/9.
//

#ifndef ARGUSAGENT_COMMON_RANDOM_H
#define ARGUSAGENT_COMMON_RANDOM_H

#include <random>
#include <limits>
#include <type_traits>

#ifdef max
#   undef max
#endif
#ifdef min
#   undef min
#endif

template<typename T, typename = void>
struct UniformDistribution {
    // 整数为闭区间
    typedef std::uniform_int_distribution<T> type;
};

// 浮点为开区间
template<typename T>
struct UniformDistribution<T, typename std::enable_if<std::is_floating_point<T>::value, void>::type> {
    typedef std::uniform_real_distribution<T> type;
};

#include "common/test_support"
// 均匀分布模型
template<typename T, typename TDistribution = typename UniformDistribution<T>::type>
class Random {
private:
    std::random_device _rd; // 不可拷贝、不可赋值
    std::mt19937 _mt;
    TDistribution _dist;
public:
    // [begin, end] for uniform_int_distribution
    // [begin, end) for uniform_real_distribution
    Random(T begin, T end) :
            _rd{},
            _mt{_rd()},
            _dist{static_cast<T>(begin > end ? 0 : begin), static_cast<T>(begin > end ? 0 : end)} {
    }

    explicit Random(T end) : Random(0, end) {
    }

    Random() : Random(std::numeric_limits<T>::max()) {
    }

    Random(const Random &) = delete;

    Random(Random &&) = delete;

    Random &operator=(const Random &) = delete;

    Random &operator=(Random &&) = delete;

    T operator()() {
        return next();
    }

    T next() {
        return _dist(_mt);
    }
};
#include "common/test_support"

template<typename T1, typename T2>
typename std::common_type<T1, T2>::type randValue(T1 start, T2 end) {
	return Random<typename std::common_type<T1, T2>::type>(start, end).next();
}

#endif //!ARGUSAGENT_COMMON_RANDOM_H
