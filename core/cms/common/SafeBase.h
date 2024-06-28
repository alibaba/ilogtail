#ifndef ARGUSAGENT_COMMON_SAFE_BASE_H
#define ARGUSAGENT_COMMON_SAFE_BASE_H

#include <type_traits>
#include <mutex>
#include "common/ScopeGuard.h"
#include "common/ArgusMacros.h"
#include <boost/noncopyable.hpp>

template<typename T>
class SafeT : private boost::noncopyable {
public:
    typedef T Type;

    template<typename ...Args>
    explicit SafeT(Args ...args): data(std::forward<Args>(args)...) {
    }

    Type &Lock() {
        mutex.lock();
        return data;
    }

    const Type &Lock() const {
        mutex.lock();
        return data;
    }

    void Unlock() const {
        mutex.unlock();
    }

    Type Get() const {
        Type copy;
        {
            std::unique_lock<std::recursive_mutex> lock(mutex);
            copy = data;
        }
        RETURN_RVALUE(copy);
    }

    void Set(const Type &r) {
        std::unique_lock<std::recursive_mutex> lock(mutex);
        if (&r != &data) {
            data = r;
        }
    }

    SafeT &operator=(const T &r) {
        Set(r);
        return *this;
    }

protected:
    mutable std::recursive_mutex mutex;
    Type data;
};

// 剥离m的wrapper，让后面的代码块像使用『非同步』版本的数据一样使用变量
// Sync的结束：使用双大括号来结束Sync
/**
 SafeT<int> a;
 Sync(a) {
    a = 5;
 }}}
 */
#define Sync(m) {                 \
    auto &__inner_##m = m.Lock(); \
    defer(m.Unlock());            \
    {                             \
        auto &m = __inner_##m;

/**
 SafeT<int> a;
 int value = SyncCall(a, {return a;});
 */
#define SyncCall(m, expr) \
    [&]() {               \
        Sync(m) expr }}   \
    }()

#endif // !ARGUSAGENT_COMMON_SAFE_BASE_H
