
/**
*@author dezhi.wangdezhi@alibaba-inc.com
*@date 12/27/17
*Copyright (c) 2016 Alibaba Group Holding Limited. All rights reserved.
*
*/
#ifndef ARGUSAGENT_COMMONUTILS_H
#define ARGUSAGENT_COMMONUTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <chrono>
#include <thread>

// convert系列
#include "StringUtils.h"

namespace common {
    class CommonUtils {
    public:
        static std::string getHostName();

        static std::string generateUuid();

        // 以下两个函数
        // 按本地时区，获取日期时间的字符串
        static std::string GetTimeStr(int64_t micros,const std::string &format ="%Y-%m-%d %H:%M:%S %z");
        // 获取GMT格式的日期时间
        static std::string GetTimeStrGMT(int64_t micros, const char *format = "%a, %d %b %Y %H:%M:%S GMT");
    };
}

template<typename Pred, typename Rep1, typename Period1,
        typename Rep2 = std::chrono::milliseconds::rep, typename Period2 = std::chrono::milliseconds::period>
bool WaitFor(const Pred &pred,
             const std::chrono::duration<Rep1, Period1> &timeout,
             const std::chrono::duration<Rep2, Period2> &interval = std::chrono::milliseconds{200}) {
    using namespace std::chrono;

    const steady_clock::time_point endTime = steady_clock::now() + timeout;
    do {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(interval);
    } while (steady_clock::now() < endTime);
    return false;
}

#endif //ARGUSAGENT_COMMONUTILS_H
