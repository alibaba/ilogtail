//
// Created by 韩呈杰 on 2023/2/21.
//
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "common/ExceptionsHandler.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/Common.h" // typeName
#include "common/ResourceConsumptionRecord.h"

#include "common/ScopeGuard.h"
#include "common/ChronoLiteral.h"

using namespace common;

// 减少线程的震荡，线程『闲置』5分钟再销毁
const std::chrono::milliseconds ThreadPool::defaultMaxIdleTime = 5_min;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadPool::Option &ThreadPool::Option::min(size_t val) {
    minThreadCount = val;
    return *this;
}

ThreadPool::Option &ThreadPool::Option::max(size_t val) {
    maxThreadCount = val;
    return *this;
}

ThreadPool::Option &ThreadPool::Option::maxIdle(std::chrono::milliseconds idle) {
    maxIdleTime = idle;
    return *this;
}

ThreadPool::Option &ThreadPool::Option::name(const std::string &name) {
    poolName = name;
    return *this;
}

ThreadPool::Option &ThreadPool::Option::capacity(size_t cap) {
    taskCapacity = cap;
    return *this;
}

std::shared_ptr<ThreadPool> ThreadPool::Option::makePool() const {
    return std::make_shared<ThreadPool>(*this);
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ThreadPool::ThreadPool(const ThreadPool::Option &option) :
        _name(option.poolName),
        _initSize(option.minThreadCount == 0 ? 1 : option.minThreadCount),
        _maxThreadCount(option.maxThreadCount > _initSize ? option.maxThreadCount : _initSize),
        _tasks(option.taskCapacity),
        _maxIdleTime(option.maxIdleTime) {
    LogInfo("ThreadPool[{}@{}](min: {}, max: {}, taskCapacity: {}, maxIdleTime: {})",
            _name, (void *) this, _initSize, _maxThreadCount, _tasks.Capacity(), _maxIdleTime);
    addThread(_initSize);
}

ThreadPool::~ThreadPool() {
    stop();
    join();
    LogDebug("~ThreadPool[{}@{}]", _name, (void *) this);
}

int ThreadPool::CurrentThreadId() {
    return GetThisThreadId();
}

void ThreadPool::stop() {
    if (!_tasks.IsClosed()) {
        LogDebug("ThreadPool[{}@{}].close()", _name, (void *) this);
        _tasks.Close();
    }
}

size_t ThreadPool::taskCount() const {
    return _tasks.Count();
}

size_t ThreadPool::threadCount() const {
    std::unique_lock<std::mutex> lockGrow{_poolMutex};
    return _pool.size();
}

//工作线程函数
void ThreadPool::worker() {
    const auto thisThreadId = std::this_thread::get_id();
    defer(removeFreeing(thisThreadId));

    std::string threadName = (_name.empty() ? FUNC_NAME : _name);
    SetupThread(threadName);
    defer(SetThreadName(""));

    LogDebug("ThreadPool[{}@{}], enter thread: {}, {}", _name, (void *) this, GetThisThreadId(), threadName);
    defer(LogDebug("ThreadPool[{}@{}], exit thread: {}, ~{}", _name, (void *) this, GetThisThreadId(), threadName));

    _idlThrNum++;        // 空闲线程+1
    defer(_idlThrNum--); // 异常退出(this还有效)

    //防止 _run==false 时立即结束,此时任务队列可能不为空
    bool running = true;
    while (running) {
        Task task; // 获取一个待执行的 task
        if (!_tasks.Take(task, _maxIdleTime)) {
            running = !this->removeThread(thisThreadId);
        } else {
            _idlThrNum--;
            defer(_idlThrNum++);

            // 执行任务
            if (!task.name.empty()) {
                SetThreadName(task.name);
            }

            {
                CpuProfile(task.name);
                safeRun(task.fn);
            }

            if (!task.name.empty()) {
                SetThreadName(threadName);
            }
            // FIXME: 这里有可能会产生：峰值过后，空闲线程过大问题
        }
    }
}

//添加指定数量的线程
void ThreadPool::addThread(size_t size) {
    if (_tasks.IsClosed()) {
        // stoped ??
        throw std::runtime_error(fmt::format("Grow on ThreadPool[{}] is stopped.", _name));
    }
    std::unique_lock<std::mutex> lockGrow{_poolMutex}; //自动增长锁

    //增加线程数量,但不超过预定义数量 _maxThreadCount
    size_t count = 0;
    for (; _pool.size() < _maxThreadCount && count < size; ++count) {
        try {
            auto thread = std::make_shared<std::thread>(&ThreadPool::worker, this); // 有可能会创建失败，从而抛出异常
            _pool[thread->get_id()] = thread;
        } catch (const std::exception &ex) {
            // https://aone.alibaba-inc.com/v2/project/629242/bug/54910273# 《【普通】【上海夺汇网络技术有限公司企业级服务】云监控(上海夺汇网络技术有限公司企业级服务)》
            LogError("{}: {}", typeName(&ex), ex.what());
            break; // 创建线程失败，后面的线程跳过
        }
    }
    if (count > 0) {
        LogDebug("ThreadPool[{}@{}].addThread({}) complete, actual added: {}, total threads: {}",
                 _name, (void *) this, size, count, _pool.size());
    }
}

void ThreadPool::notifyOnEmpty() {
    if (_pool.empty() && _freeing.empty()) {
        Log((_tasks.IsClosed() ? LEVEL_DEBUG : LEVEL_WARN), "ThreadPool[{}@{}] empty", _name, (void *) this);
        _poolEmptyCv.notify_all();
    }
}

void ThreadPool::removeFreeing(std::thread::id threadId) {
    std::unique_lock<std::mutex> lockGrow{_poolMutex}; //自动增长锁
    _freeing.erase(threadId);
    notifyOnEmpty();
}

bool ThreadPool::removeThread(std::thread::id threadId) {
    bool removed = false;
    std::unique_lock<std::mutex> lockGrow{_poolMutex}; //自动增长锁
    if (_pool.size() > _initSize || _tasks.IsClosed()) {
        removed = true;
        auto it = _pool.find(threadId);
        if (it != _pool.end()) {
            if (it->second->joinable()) {
                if (threadId == std::this_thread::get_id()) {
                    it->second->detach();
                    _freeing.insert(threadId);
                } else {
                    it->second->join();
                }
            }
            _pool.erase(it);
        }

        notifyOnEmpty();
    }

    return removed;
}

void ThreadPool::join() {
    std::unique_lock<std::mutex> lock{_poolMutex}; //自动增长锁
    _poolEmptyCv.wait(lock, [&]() { return _pool.empty() && _freeing.empty(); }); // 等待线程池清空，也暨所有线程都退出
}
