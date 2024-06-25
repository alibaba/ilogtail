//
// Created by 韩呈杰 on 2023/2/21.
//

#ifndef ARGUS_AGENT_THREADPOOL_H
#define ARGUS_AGENT_THREADPOOL_H

#include <map>
#include <set>
#include <atomic>
#include <future>
#include <functional> // std::bind
#include <stdexcept>
#include <chrono>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/core/demangle.hpp>
#include <boost/thread/condition_variable.hpp>

#include "SyncQueue.h"
#include "Logger.h"

#ifdef max
#   undef max
#endif
#ifdef min
#   undef min
#endif

#if defined(_MSC_VER) || defined(MSVC)
// VC的__FUNCTION__: ThreadPool::worker
#   define FUNC_NAME __FUNCTION__
#else
#   define FUNC_NAME (boost::core::demangle(typeid(*this).name()) + "::" + __FUNCTION__)
#endif

// https://github.com/lzpong/threadpool
// 线程池,可以提交变参函数或lambda表达式的匿名函数执行,可以获取执行返回值
// 不直接支持类成员函数, 支持类静态成员函数或全局函数,Opteron()函数等
class ThreadPool : private boost::noncopyable {
#include "test_support"
private:

    struct Task {
        std::string name;
        std::function<void()> fn;

        Task() = default;

        Task(std::string n, const std::function<void()> &f) : name(std::move(n)), fn(f) {}
    };

    const std::string _name;

    const size_t _initSize;       // 初始化线程数量
    const size_t _maxThreadCount; // 最大线程数量

    mutable std::mutex _poolMutex;                                 //线程池增长同步锁
    std::map<std::thread::id, std::shared_ptr<std::thread>> _pool;    // 线程池
    std::set<std::thread::id> _freeing; // 正在释放的线程
    boost::condition_variable_any _poolEmptyCv;

    SyncQueue<Task> _tasks; // 任务队列

    std::atomic<int> _idlThrNum{0};               //空闲线程数量
    const std::chrono::milliseconds _maxIdleTime; //最大空闲时间，超期释放

public:
    static const std::chrono::milliseconds defaultMaxIdleTime;

    static int CurrentThreadId();

    class Option {
    private:

        friend class ThreadPool;

        size_t minThreadCount = 0;
        // maxThreadCount == 0时，取值与minThreadCount相同
        size_t maxThreadCount = 0;
        std::chrono::milliseconds maxIdleTime = defaultMaxIdleTime;
        std::string poolName;
        size_t taskCapacity = 4096;
    public:
        Option() = default;
        Option &min(size_t val);
        Option &max(size_t val);
        Option &maxIdle(std::chrono::milliseconds);
        Option &name(const std::string &);
        Option &capacity(size_t);

        std::shared_ptr<ThreadPool> makePool() const;
    };

    explicit ThreadPool(const Option &);
    ~ThreadPool();

    // 提交一个任务
    // 调用.get()获取返回值会等待任务执行完,获取返回值
    // 有两种方法可以实现调用类成员，
    // 一种是使用   bind： .commit(std::bind(&Dog::sayHello, &dog));
    // 一种是用   mem_fn： .commit(std::mem_fn(&Dog::sayHello), this)
    template<typename Rep, typename Period, typename F, typename... Args>
    auto commitTimeout(const std::string &name, const std::chrono::duration<Rep, Period> &timeout, F &&f, Args &&... args) -> std::future<decltype(f(std::forward<Args>(args)...))> {
        // typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
        using RetType = decltype(f(std::forward<Args>(args)...));

        if (_tasks.IsClosed()) {
            // stoped ??
            // throw std::runtime_error("commit on ThreadPool is stopped.");
            return std::future<RetType>();
        }

        // 把函数入口及参数,打包(绑定)
        auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        // push(Task{...}) 放到队列后面
        if (_tasks.Push(Task{name, [task]() { (*task)(); }}, timeout)) {
            std::future<RetType> future = task->get_future();
            if (static_cast<int>(_tasks.Count()) > _idlThrNum) {
                addThread(1);
            }
            return future;
        }
        using namespace common;
        LogWarn("ThreadPool[{}].commit() failed, tasks(size:{}, capacity:{}, closed:{}), threads: {}",
                _name, _tasks.Count(), _tasks.Capacity(), _tasks.IsClosed(), threadCount());
        return std::future<RetType>();
    }
    template<typename F, typename... Args>
    auto commit(const std::string &name, F &&f, Args &&... args) -> std::future<decltype(f(std::forward<Args>(args)...))> {
        return commitTimeout(name, std::chrono::seconds::zero(), std::forward<F>(f), std::forward<Args>(args)...);
    }

    size_t minThreadCount() const {
        return _initSize;
    }

    size_t maxThreadCount() const {
        return _maxThreadCount;
    }

    //空闲线程数量
    int idleCount() const {
        return _idlThrNum;
    }

    //线程数量
    size_t threadCount() const;

    // 任务数量
    size_t taskCount() const;

    void stop();
    void join();

private:
    //添加指定数量的线程
    void addThread(size_t size);
    bool removeThread(std::thread::id threadId);
    void removeFreeing(std::thread::id threadId);

    void notifyOnEmpty();

    void worker();
};

#include "test_support"

#endif // !ARGUS_AGENT_THREADPOOL_H
