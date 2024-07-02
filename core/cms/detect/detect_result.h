//
// Created by 许刘泽 on 2022/8/1.
//

#ifndef ARGUSAGENT_SRC_DETECT_DETECT_RESULT_H_
#define ARGUSAGENT_SRC_DETECT_DETECT_RESULT_H_

#include <string>
#include <memory>
#include <chrono>
#include "sliding_time_window.h"

enum DETECT_TYPE
{
    PING_DETECT = 1,
    TELNET_DETECT = 2,
    HTTP_DETECT = 3,
};

struct PingResult
{
    std::string targetHost;
    /**
     * rt ，在收到非超时包时更新
     */
    std::shared_ptr<common::SlidingTimeWindow<int64_t>> windowSharePtr;
    /**
     * 发送 ping 包总次数，在发送时更新
     */
    int count = 0;
    /**
     * 丢包（超时）总次数，在超时时更新
     */
    int lostCount = 0;
    /**
     * 上次调度时间，用来判断当前轮次ping起始时间
     */
    std::chrono::steady_clock::time_point lastScheduleTime;
    /**
     * 当前轮次ping是否已经收到回包（用来判断ping是否超时）
     */
     bool receiveEchoPackage = false;
};

struct TelnetResult
{
    std::string keyword;
    std::string uri;
    int status;
    std::chrono::microseconds latency;
};

struct HttpResult
{
    std::string keyword;
    std::string uri;
    int code;
    size_t bodyLen;
    std::chrono::microseconds latency;
};

struct DetectTaskResult
{
    DETECT_TYPE type;
    std::chrono::system_clock::time_point ts;
    PingResult mPingResult;
    TelnetResult mTelnetResult{};
    HttpResult mHttpResult{};
};

#endif //ARGUSAGENT_SRC_DETECT_DETECT_RESULT_H_
