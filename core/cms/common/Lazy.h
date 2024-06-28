//
// Created by 韩呈杰 on 2023/12/4.
//

#ifndef ARGUSAGENT_COMMON_LAZY_H
#define ARGUSAGENT_COMMON_LAZY_H

#include <mutex>       // std::once_flag
#include <functional>  // std::function
#include <memory>      // std::unique_ptr

#include "common/test_support"

template<typename T, typename Deleter = std::default_delete<T>>
class Lazy {
private:
    // 只创建一次
    std::once_flag once;
    // 延迟创建
    std::function<T *()> create;
    // 随析构销毁
    std::unique_ptr<T, Deleter> instance{nullptr};

public:
    // 注：因为是延迟初始化，这里会丧失完美转发的特性。
    template<typename ...Args>
    explicit Lazy(Args &&...args): create([=]() { return new T(args...); }) {
    }

    T *Instance() {
        std::call_once(once, [this]() {
            instance.reset(create());
        });
        return instance.get();
    }

    T *operator->() {
        return Instance();
    }

    // 显式销毁，也可以在Lazy析构时『隐式』销毁
    void Destroy() {
        instance.reset(nullptr);
    }
};

#include "common/test_support"

#endif //ARGUSAGENT_COMMON_LAZY_H
