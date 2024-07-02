//
// Created by 韩呈杰 on 2024/1/12.
//

#ifndef ARGUSAGENT_DURATION_PLACEHOLDER_H
#define ARGUSAGENT_DURATION_PLACEHOLDER_H

#include <chrono>

// std::chrono::duration字面量operator
inline std::chrono::hours operator ""_h(unsigned long long v) {
    return std::chrono::hours{v};
}

inline std::chrono::minutes operator ""_min(unsigned long long v) {
    return std::chrono::minutes{v};
}

inline std::chrono::seconds operator ""_s(unsigned long long v) {
    return std::chrono::seconds{v};
}

inline std::chrono::milliseconds operator ""_ms(unsigned long long v) {
    return std::chrono::milliseconds{v};
}

inline std::chrono::microseconds operator ""_us(unsigned long long v) {
    return std::chrono::microseconds{v};
}

#endif //ARGUSAGENT_DURATION_PLACEHOLDER_H
