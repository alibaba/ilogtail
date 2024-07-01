//
// Created by 韩呈杰 on 2023/4/14.
//

#ifndef ARGUSAGENT_COMMON_TIME_PROFILE_H
#define ARGUSAGENT_COMMON_TIME_PROFILE_H

#include <chrono>
#include "TimeFormat.h"

class TimeProfile {
    std::chrono::steady_clock::time_point prevTime;
public:
    TimeProfile() {
        reset();
    }

    template<typename Duration = std::chrono::milliseconds>
    Duration cost(bool resetTime = true) {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<Duration>(now - prevTime);
        if (resetTime) {
            this->prevTime = now;
        }
        return duration;
    }

    int64_t millis(bool resetTime = true) {
        return cost<std::chrono::milliseconds>(resetTime).count();
    }

    void reset() {
        prevTime = std::chrono::steady_clock::now();
    }

    const std::chrono::steady_clock::time_point &lastTime() const {
        return prevTime;
    }
};

#endif // !ARGUSAGENT_COMMON_TIME_PROFILE_H
