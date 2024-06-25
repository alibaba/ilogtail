//
// Created by 韩呈杰 on 2024/5/11.
//
#if defined(__GNUC__)

// OneAgent不需要线程封装
#if !defined(WITHOUT_MINI_DUMP) && (defined(__linux__) || defined(__unix__))
#   define SUPPORT_WRAP_PTHREAD_CREATE
#endif

#include "common/ThreadUtils.h"

#include <pthread.h>

#include <memory>
#include <mutex>
#include <map>
#include <utility>
#include "common/Common.h" // GetThisThreadId

class PThreadManager {
    std::mutex mutex;
    struct ThreadDescription {
        const pthread_t thread;
        std::string name;

        explicit ThreadDescription(pthread_t t):thread(t) {}

        explicit ThreadDescription(pthread_t t, std::string s) : thread(t), name(std::move(s)) {}
    };
    std::map<TID, std::shared_ptr<ThreadDescription>> threadMap;
public:
    void AddCurrentThread() {
        const int threadId = GetThisThreadId();
        std::lock_guard<std::mutex> guard(mutex);
        threadMap[threadId] = std::make_shared<ThreadDescription>(pthread_self());
    }

    void RemoveCurrentThread() {
        int threadId = GetThisThreadId();
        std::lock_guard<std::mutex> guard(mutex);
        threadMap.erase(threadId);
    }

#ifdef SUPPORT_WRAP_PTHREAD_CREATE
    size_t CopyTo(std::map<int, pthread_t> &threadIds) {
        {
            std::lock_guard<std::mutex> guard(mutex);
            for (auto const &it: threadMap) {
                threadIds[it.first] = it.second->thread;
            }
        }
        return threadIds.size();
    }
#endif

    pthread_t Get(TID tid) {
        pthread_t thread{0};
        std::lock_guard<std::mutex> guard(mutex);
        auto it = threadMap.find(tid);
        if (it != threadMap.end()) {
            thread = it->second->thread;
        }
        return thread;
    }

    void SetName(TID tid, const std::string &name) {
        std::lock_guard<std::mutex> guard(mutex);
#if !defined(SUPPORT_WRAP_PTHREAD_CREATE)
        if (name.empty()) {
            threadMap.erase(tid);
        } else {
#endif
            auto it = threadMap.find(tid);
            if (it != threadMap.end()) {
                it->second->name = name;
            }
#if !defined(SUPPORT_WRAP_PTHREAD_CREATE)
            else {
                threadMap[tid] = std::make_shared<ThreadDescription>(pthread_self(), name);
            }
        }
#endif
    }

    std::string GetName(TID tid) {
        std::string name;
        std::lock_guard<std::mutex> guard(mutex);
        auto it = threadMap.find(tid);
        if (it != threadMap.end()) {
            name = it->second->name;
        }
        return name;
    }
};

static PThreadManager * const pthreadManager = new PThreadManager();

#include "common/ScopeGuard.h"

struct wrapped_thread_entry_arg {
    void *(*startRoutine)(void *);
    void *__restrict arg;
};

extern "C" void *argus_thread_entry(void *ptr) {
    pthreadManager->AddCurrentThread();
    defer(pthreadManager->RemoveCurrentThread());

    auto arg = reinterpret_cast<wrapped_thread_entry_arg *>(ptr);
    defer(delete arg);

    void *ret = nullptr;
    try {
        ret = arg->startRoutine(arg->arg);
    } catch (...) {
    }

    return ret;
}
extern "C" __attribute__((noinline)) void argus_thread_entry_end() {}

#if defined(SUPPORT_WRAP_PTHREAD_CREATE)
extern "C" {
int __real_pthread_create(pthread_t * __restrict,
                          const pthread_attr_t * __restrict,
                          void *(*)(void *),
                          void * __restrict);
} // extern "C"

extern "C" int __wrap_pthread_create(pthread_t *__restrict thread,
                                     const pthread_attr_t *__restrict attr,
                                     void *(*start_routine)(void *),
                                     void *__restrict arg) {
    // will be release in thread
    if (start_routine != nullptr) {
        auto *wrappedArg = new wrapped_thread_entry_arg();
        wrappedArg->startRoutine = start_routine;
        wrappedArg->arg = arg;

        arg = wrappedArg;
        start_routine = argus_thread_entry;
    }
    return __real_pthread_create(thread, attr, start_routine, arg);
}

size_t EnumThreads(std::map<TID, pthread_t> &threads) {
    return pthreadManager->CopyTo(threads);
}
#endif // SUPPORT_WRAP_PTHREAD_CREATE

void SetThreadName(const std::string &name) {
    std::string actualName = (name.size() > MAX_THREAD_NAME_LEN? name.substr(0, MAX_THREAD_NAME_LEN): name);

    pthreadManager->SetName(GetThisThreadId(), name);
}

std::string GetThreadName(TID tid) {
    return pthreadManager->GetName(tid);
}

// pthread_setname_np有些奇怪，即使没设置名称的线程也会返回一个名称，且这个名称不是进程名。因此移至pthread_hook.cpp下实现
// void SetThreadName(const std::string &name) {
//     std::string actualName = (name.size() > MAX_THREAD_NAME_LEN? name.substr(0, MAX_THREAD_NAME_LEN): name);
//
// #ifdef __APPLE__
//     pthread_setname_np(actualName.c_str());
// #else
//     pthread_setname_np(pthread_self(), actualName.c_str());
// #endif
//     // std::cout << "******** SetThreadName(" << GetThisThreadId() << ", " << actualName << ")" << std::endl;
// }
//
// std::string GetThreadName(TID tid) {
//     pthread_t thread = (tid < 0 || tid == GetThisThreadId()? pthread_self(): GetThread(tid));
//
//     if (thread) {
//         char name[MAX_THREAD_NAME_LEN + 1] = {0};
//         pthread_getname_np(thread, name, sizeof(name));
//         return name;
//     }
//     return {};
// }

#endif // defined(__GNUC__)
