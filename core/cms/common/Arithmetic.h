//
// Created by 韩呈杰 on 2024/1/9.
//

#ifndef ARGUSAGENT_ARITHMETIC_H
#define ARGUSAGENT_ARITHMETIC_H

#include <limits>
#include <type_traits>

#ifdef max
#   undef max
#endif

// T 是否无符号整数
template<typename T>
constexpr bool is_uint() {
    return std::is_integral<T>::value && std::is_unsigned<T>::value;
}

template<typename T>
constexpr bool is_numeric() {
    return std::is_arithmetic<T>::value;
}

template<typename T1, typename T2, typename ...TOthers>
constexpr bool is_numeric() {
    return is_numeric<T1>() && is_numeric<T2, TOthers...>();
}

// 无符号整数，支持溢出情况下的循环计算
template<typename T, typename std::enable_if<is_uint<T>(), int>::type = 0>
T Delta(const T &a, const T &b) {
    if (a < b) {
        // 溢出了
        return std::numeric_limits<T>::max() - b + a;
    } else {
        return a - b;
    }
}

// T1, T2不是相同数字类型，或不是无符号整数，不支持溢出情况下的循环计算
template<typename T1, typename T2, typename std::enable_if<
        is_numeric<T1, T2>() && (!std::is_same<T1, T2>::value || !is_uint<T1>()), int>::type = 0>
auto Delta(const T1 &a, const T2 &b) -> decltype(a - b) {
    return a > b ? a - b : 0;
}

// 从from 到 to增长了多少，始终返回 >= 0
template<typename T1, typename T2>
auto Increase(const T1 &from, const T2 &to) -> decltype(to - from) {
    return Delta(to, from);
}

template<typename T1, typename T2>
auto AbsMinus(const T1 &a, const T2 &b) -> decltype(a - b) {
    return a > b ? a - b : b - a;
}

template<typename T>
T Diff(const T &a, const T &b) {
    return a > b? a - b: T{0};
}

#endif //ARGUSAGENT_ARITHMETIC_H
