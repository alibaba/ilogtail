#ifndef ARGUS_THREAD_WORKER_H
#define ARGUS_THREAD_WORKER_H

#include <atomic>
#include <thread>
#include <memory>

#include <mutex>
#include <functional>
#include <chrono>
#include <utility>
#include <boost/thread/condition_variable.hpp>
#include "common/TimeFormat.h"

namespace common {
    class ThreadWorker;
}
class WorkerKeepAliveLog {
    friend class common::ThreadWorker;

    const std::string name;
    const std::chrono::microseconds step;
    std::chrono::steady_clock::time_point nextLogTime;
    size_t count = 0;
public:
    template<typename Rep, typename Period>
    WorkerKeepAliveLog(std::string s, const std::chrono::duration<Rep, Period> &interval):
            name(std::move(s)), step(std::chrono::duration_cast<std::chrono::microseconds>(interval)) {
        nextLogTime = std::chrono::steady_clock::now() + step;
    }

    explicit WorkerKeepAliveLog(const std::string &s);

    bool KeepAlive();
    size_t Count() const {
        return this->count;
    }
};

namespace common {
#include "test_support"
class ThreadWorker {
    public:
        explicit ThreadWorker(bool end = true);
        virtual ~ThreadWorker();

        virtual void doRun() {};
        bool runIt();

        bool endThread();

        bool isThreadEnd() const {
            return mEnd;
        }

        virtual void join();

        // 可被endThread打断的sleep
        // 返回false时，表示线程任务已结束
        bool sleepFor(int64_t micros);

        template<typename Rep, typename Period>
        bool sleepFor(const std::chrono::duration<Rep, Period> &duration) {
            return sleepFor(std::chrono::duration_cast<std::chrono::microseconds>(duration).count());
        }

        void keepAlive();
        std::chrono::microseconds keepAliveStep() const;

    private:
        std::shared_ptr<std::thread> mThread;

        std::atomic<bool> mEnd{true};
        std::mutex mMutex;
        // bug : https://bbs.aw-ol.com/topic/4665/%E9%9A%8F%E7%AC%94%E8%AE%B0-c-condition_variable-%E9%99%B7%E9%98%B1#C++ condition_variable 陷阱
        // bug : https://blog.csdn.net/pes2020/article/details/120722357#标准库条件变量等待时间受系统时间影响
        boost::condition_variable_any mConditionVariable;

        std::shared_ptr<WorkerKeepAliveLog> mKeepAlive;
    };
}

class ThreadWorkerEx {
public:
    explicit ThreadWorkerEx(bool running = false);
    virtual ~ThreadWorkerEx() = 0;

    // 异步运行，fnOnce返回
    bool runAsync(const std::function<std::chrono::milliseconds()> &fnOnce,
                  const std::chrono::seconds &delay = std::chrono::seconds::zero());
    void stopAsync();

    bool IsRunning() const {
        return mRunning;
    }

private:
    std::shared_ptr<std::thread> mThread;

    std::atomic<bool> mRunning{false};
    std::mutex mMutex;
    // bug : https://bbs.aw-ol.com/topic/4665/%E9%9A%8F%E7%AC%94%E8%AE%B0-c-condition_variable-%E9%99%B7%E9%98%B1#C++ condition_variable 陷阱
    // bug : https://blog.csdn.net/pes2020/article/details/120722357#标准库条件变量等待时间受系统时间影响
    boost::condition_variable_any mCv;
};
#include "test_support"

#endif
