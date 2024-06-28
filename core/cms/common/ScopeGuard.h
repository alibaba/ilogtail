//
// Created by 韩呈杰 on 2023/1/12.
//

#ifndef ARGUSAGENT_SCOPE_GUARD_H
#define ARGUSAGENT_SCOPE_GUARD_H

#include <functional>
#include <utility>

template<typename T>
void Delete(T *&p) {
    if (p != nullptr) {
        delete p;
        p = nullptr;
    }
}

class ScopeGuard {
    std::function<void()> fn;
public:
    explicit ScopeGuard(std::function<void()> f)
            : fn(std::move(f)) {
    }

    ~ScopeGuard() {
        fn();
    }
};

#define defer3(ln, statement) ScopeGuard __ ## ln ## _defer_([&](){statement;})
#define defer2(ln, statement) defer3(ln, statement)
#define defer(statement) defer2(__LINE__, statement)

namespace common {
    typedef ScopeGuard ResourceGuard;
}

#endif //ARGUSAGENT_SCOPE_GUARD_H
