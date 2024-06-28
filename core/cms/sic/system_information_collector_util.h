//
// Created by liuze.xlz on 2020/11/23.
//

#ifndef SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_UTIL_H
#define SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_UTIL_H

#if defined(WIN32) || defined(_WIN32)
static constexpr const bool isWin32 = true;
#else
static constexpr const bool isWin32 = false;
#endif

enum SicStatus {
    SIC_FAILED = -1,
    SIC_EXECUTABLE_FAILED = SIC_FAILED,

    SIC_SUCCESS = 0,
    SIC_EXECUTABLE_SUCCESS = SIC_SUCCESS,

    SIC_NOT_IMPLEMENT = 1,
};


#endif //SIC_INCLUDE_SYSTEM_INFORMATION_COLLECTOR_UTIL_H_
