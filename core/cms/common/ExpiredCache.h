//
// Created by 韩呈杰 on 2023/7/7.
//

#ifndef ARGUSAGENT_EXPIRED_CACHE_H
#define ARGUSAGENT_EXPIRED_CACHE_H

#include <functional>
#include <chrono>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp> // boost::unique_lock

template<typename T, typename TMutex = boost::shared_mutex>
class ExpiredCache {
public:
    template<typename Rep, typename Period>
    ExpiredCache(const std::function<T()> &get, const std::chrono::duration<Rep, Period> &t)
            : timeout(std::chrono::duration_cast<std::chrono::microseconds>(t)), fnGet(get) {
    }

    T get() {
        {
            boost::shared_lock<decltype(mutex)> guard(mutex);
            if (IsValid()) {
                return data;
            }
        }

        boost::unique_lock<decltype(mutex)> guard(mutex);
        if (!IsValid()) {
            data = fnGet();
            expire = std::chrono::steady_clock::now() + timeout;
        }
        return data;
    }

    T operator()() {
        return this->get();
    }

private:
    bool IsValid() const {
        return std::chrono::steady_clock::now() <= expire;
    }

private:
    T data;
    const std::chrono::microseconds timeout;
    std::chrono::steady_clock::time_point expire;
    boost::shared_mutex mutex;  // 读写锁
    std::function<T()> fnGet;
};

#endif //ARGUSAGENT_EXPIRED_CACHE_H
