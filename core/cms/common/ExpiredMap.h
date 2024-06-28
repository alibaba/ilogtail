//
// Created by hancj on 2023/7/12.
//

#ifndef ARGUSAGENT_EXPIRED_MAP_H
#define ARGUSAGENT_EXPIRED_MAP_H

#include <map>
#include <mutex>
#include <chrono>
#include <functional>

template<typename K, typename V>
class ExpiredMap {
    struct ExpiredValue {
        V v;
        std::chrono::steady_clock::time_point expired;
    };
    const std::chrono::microseconds ttl;
    mutable std::recursive_mutex mutex;
    std::map<K, ExpiredValue> data;
public:
    template<typename Rep, typename Period>
    explicit ExpiredMap(const std::chrono::duration<Rep, Period> &timeout):
            ttl(std::chrono::duration_cast<std::chrono::microseconds>(timeout)) {
    }

    struct GetterResult {
        V v;
        bool ok = false;
    };

    GetterResult Compute(const K &k, const std::function<bool(const K&, std::map<K, V> &)> &supply) {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        auto it = data.find(k);
        if (it == data.end() || std::chrono::steady_clock::now() >= it->second.expired) {
            // 不存在或过期，重新获取
            if (it != data.end()) {
                data.erase(it);
                it = data.end();
            }

            std::map<K, V> d;
            if (supply(k, d)) {
                const auto expired = std::chrono::steady_clock::now() + ttl;
                for (auto& entry : d) {
                    ExpiredValue ev;
                    ev.v = entry.second;
                    ev.expired = expired;
                    data[entry.first] = ev;
                }
                it = data.find(k);
            }
        }
        GetterResult r;
        r.ok = it != data.end();
        if (r.ok) {
            r.v = it->second.v;
        }
        return r;
    }

    size_t Size() const {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        return data.size();
    }

    size_t IsEmpty() const {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        return data.empty();
    }
};

#endif //ARGUSAGENT_EXPIRED_MAP_H
