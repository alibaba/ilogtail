#include "common/ThreadWorker.h"

#include <thread>

#include <boost/core/demangle.hpp>
#include <boost/chrono.hpp>

#include "common/ScopeGuard.h"
#include "common/ThreadUtils.h"
#include "common/Chrono.h"

#include "common/ExceptionsHandler.h"
#include "common/Logger.h"
#include "common/TimeProfile.h"

#include "common/ResourceConsumptionRecord.h"

static const std::chrono::microseconds defaultKeepAliveInterval = 5_min;

namespace common {
    ThreadWorker::ThreadWorker(bool end) {
        mEnd = end;
    }

    ThreadWorker::~ThreadWorker() {
        endThread();
        join();
    }

    bool ThreadWorker::runIt() {
        std::unique_lock<std::mutex> lock(mMutex);
        if (!this->mEnd) {
            return true;
        }
        mEnd = false;

        mThread = std::make_shared<std::thread>(safeRun, [this]() {
            std::string name = boost::core::demangle(typeid(*this).name());
            if (name.size() > MAX_THREAD_NAME_LEN) {
                auto pos = name.find_last_of(':');
                if (pos != std::string::npos) {
                    name = name.substr(pos + 1);
                }
            }
            SetupThread(name);
            LogInfo("thread worker <{}@{}> started", name, (void *) this);
            defer(LogInfo("thread worker <{}@{}> stopped", name, (void *) this));

            this->mKeepAlive = std::make_shared<WorkerKeepAliveLog>(name);
            defer(this->mKeepAlive.reset());

            defer(mEnd = true);
            this->doRun();
        });

        return (bool) mThread;
    }

    void ThreadWorker::join() {
        if (mThread && mThread->joinable()) {
            mThread->join();
            mThread = nullptr;
        }
    }

    bool ThreadWorker::endThread() {
        mEnd = true;
        mConditionVariable.notify_all();

        return mEnd;
    }

    std::chrono::microseconds ThreadWorker::keepAliveStep() const {
        if (std::shared_ptr<WorkerKeepAliveLog> tmp = mKeepAlive) {
            return tmp->step;
        }
        return defaultKeepAliveInterval;
    }
    void ThreadWorker::keepAlive() {
        if (std::shared_ptr<WorkerKeepAliveLog> tmp = mKeepAlive) {
            tmp->KeepAlive();
        }
    }
    bool ThreadWorker::sleepFor(int64_t micros) {
        keepAlive();
        if (micros > 0) {
            boost::chrono::microseconds interval(micros);
            std::unique_lock<std::mutex> lock(mMutex);
            mConditionVariable.wait_for(lock, interval, [&]() { return mEnd.load(); });
        }
        return !mEnd;
    }
}

using namespace common;

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ThreadWorkerEx
ThreadWorkerEx::ThreadWorkerEx(bool running) {
    mRunning = running;
}

ThreadWorkerEx::~ThreadWorkerEx() {
    stopAsync();
}

void ThreadWorkerEx::stopAsync() {
    mRunning = false;
    mCv.notify_all();
    if (mThread && mThread->joinable()) {
        mThread->join();
        mThread = nullptr;
    }
}

bool ThreadWorkerEx::runAsync(const std::function<std::chrono::milliseconds()> &fnOnce,
                              const std::chrono::seconds &delay) {
    if (this->mRunning) {
        return false;
    }
    mRunning = true;

    mThread = std::make_shared<std::thread>(safeRun, [this, fnOnce, delay]() {
        std::string name = boost::core::demangle(typeid(*this).name());
        if (name.size() > MAX_THREAD_NAME_LEN) {
            auto pos = name.find_last_of(':');
            if (pos != std::string::npos) {
                name = name.substr(pos + 1);
            }
        }
        SetupThread(name);

        LogInfo("thread worker <{}> started", name);
        defer(LogInfo("thread worker <{}> stopped", name));

        defer(mRunning = false);

        if (delay.count() > 0) {
            std::unique_lock<std::mutex> lock(mMutex);
            mCv.wait_for(lock, boost::chrono::microseconds(ToMicros(delay)), [&]() { return !mRunning; });
        }

        WorkerKeepAliveLog keepAlive(name);
        while (mRunning) {
            keepAlive.KeepAlive();

            std::chrono::milliseconds interval;
            {
                CpuProfile(name);
                interval = fnOnce();
            }


            if (interval.count() > 0) {
                std::unique_lock<std::mutex> lock(mMutex);
                mCv.wait_for(lock, boost::chrono::microseconds(ToMicros(interval)), [&]() { return !mRunning; });
            }
        }
    });

    return (bool) mThread;
}


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WorkerKeepAliveLog
bool WorkerKeepAliveLog::KeepAlive() {
    auto now = std::chrono::steady_clock::now();
    bool ok = now >= nextLogTime;
    if (ok) {
        this->count++;
        nextLogTime = now + step;
        typedef std::chrono::duration<double> seconds;
        LogInfo("worker <{}> keep alive: {}/{}", name, count, std::chrono::duration_cast<seconds>(step));
    }
    return ok;
}


WorkerKeepAliveLog::WorkerKeepAliveLog(const std::string &s) :
        WorkerKeepAliveLog(s, defaultKeepAliveInterval) {
}