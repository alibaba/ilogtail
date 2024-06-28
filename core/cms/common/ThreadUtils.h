//
// Created by 韩呈杰 on 2024/5/11.
//

#ifndef ARGUSAGENT_COMMON_THREAD_NAME_H
#define ARGUSAGENT_COMMON_THREAD_NAME_H

#include <string>
#include <list>
#include <functional>

// 最大线程名称(该尺寸不含尾部0)
#ifdef WIN32
#   define MAX_THREAD_NAME_LEN 31
#else
#   define MAX_THREAD_NAME_LEN 15
#endif

typedef int TID;

// 设置当前线程的名字Linux下线程名称不得超过15个字符，如果超过将被截断
void SetThreadName(const std::string &name);
std::string GetThreadName(TID tid);

#if !defined(WITHOUT_MINI_DUMP)

#if !defined(WIN32)

#include <pthread.h>
#include <map>

size_t EnumThreads(std::map<TID, pthread_t> &);
#endif // !WIN32


struct tagThreadStack {
    int threadId = 0;
    std::string threadName;
    std::string stackTrace;

    tagThreadStack();
    explicit tagThreadStack(const std::string &trace);
    tagThreadStack(int threadId, const std::string &trace);

    std::string str(int index = -1) const;
};

void WalkThreadStack(std::list<tagThreadStack> &);
// 计数从1起
void WalkThreadStack(const std::function<void(int, const tagThreadStack &)> &fnPrint);
#endif // !WITHOUT_MINI_DUMP

#endif // !ARGUSAGENT_COMMON_THREAD_NAME_H
