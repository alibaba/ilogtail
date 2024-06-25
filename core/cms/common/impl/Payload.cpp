//
// Created by 韩呈杰 on 2023/5/30.
//
#include "common/Payload.h"

#include <fmt/format.h>

std::string ToPayloadString(double value) {
    std::string str;
    if (value - static_cast<int>(value) == 0) {
        str = fmt::format("{}", (int64_t)value);
    } else {
        str = fmt::format("{:0.2f}", value);
        //去掉小数点后面的0
        size_t count = str.size();
        const char *buf = str.c_str();
        for (int i = static_cast<int>(count) - 1; i >= 0; i--) {
            if (buf[i] == '0') {
                count--;
            } else {
                count -= (buf[i] == '.' ? 1 : 0);
                break;
            }
        }
        if (count != str.size()) {
            str = str.substr(0, count);
        }
    }
    return str;
}
