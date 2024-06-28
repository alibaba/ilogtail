//
// Created by 韩呈杰 on 2024/2/22.
//

#ifndef ARGUS_CORE_T_SCHEDULER_H
#define ARGUS_CORE_T_SCHEDULER_H

#include <mutex>
#include <set>
#include <map>
#include <type_traits>
#include <boost/json.hpp>
// bug : https://bbs.aw-ol.com/topic/4665/%E9%9A%8F%E7%AC%94%E8%AE%B0-c-condition_variable-%E9%99%B7%E9%98%B1#C++ condition_variable 陷阱
// bug : https://blog.csdn.net/pes2020/article/details/120722357#标准库条件变量等待时间受系统时间影响
#include <boost/thread/condition_variable.hpp>
#include "common/ThreadWorker.h"
#ifdef ONE_AGENT
#   include "cms/common/ThreadPool.h"
#   include "cms/common/Common.h"
#else
#   include "common/ThreadPool.h"
#   include "common/Common.h"
#endif
#include "common/TimeProfile.h"
#include "common/Logger.h"
#include "common/ScopeGuard.h"
#include "common/Chrono.h" // ToMicros

namespace common {
    struct CommonMetric;
}

namespace argus {
    class Module;

    struct SchedulerState {
        virtual ~SchedulerState() = default;
        virtual std::string getName() const = 0;
        virtual std::chrono::milliseconds interval() const = 0;
        //
        long errorCount = 0;
        long runTimes = 0;

        std::chrono::steady_clock::time_point nextTime; // 下次执行时间

        long skipCount = 0;  // 『漏』执行的次数
    };

    void statistic(const std::string &name,
                   const std::map<std::string, std::shared_ptr<SchedulerState>> &state,
                   common::CommonMetric &metric);

#include "common/test_support"

template<typename T, typename TSchedulerState,
        typename std::enable_if<std::is_base_of<SchedulerState, TSchedulerState>::value, int>::type = 0>
class SchedulerT : public ::common::ThreadWorker {
public:
    SchedulerT() = default;

    ~SchedulerT() override {
        close();
    }

    void doRun() override {
        const auto maxTimeout = boost::chrono::microseconds{ToMicros(keepAliveStep())}; // 一般是5分钟
        std::shared_ptr<std::map<std::string, T>> prevItems;

        std::unique_lock<std::mutex> lock(this->mutex_);
        for (; !this->isThreadEnd(); keepAlive()) {
            // 配置是否有变更
            updateItems(prevItems);

            if (timerQueue.empty()) {
                this->cv_.wait_for(lock, maxTimeout);
            } else {
                const auto steadyNow = std::chrono::steady_clock::now();
                while (!timerQueue.empty()) {
                    auto te = *timerQueue.begin();
                    if (steadyNow < te.next) {
                        // 要么超时，要么被唤醒
                        auto timeout = boost::chrono::microseconds{ToMicros(te.next - steadyNow) + 1};
                        this->cv_.wait_for(lock, std::min({maxTimeout, timeout}));
                        break;
                    } else {
                        timerQueue.erase(timerQueue.begin());
                        // 执行
                        this->executeItem(te.state, te.next);
                    }
                }
            }
        }
    }

    void setItems(const std::shared_ptr<std::map<std::string, T>> &r) {
        std::unique_lock<std::mutex> lock(mutex_);
        mItems = r;
        this->cv_.notify_all();
    }

    std::shared_ptr<std::map<std::string, T>> getItems() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return mItems;
    }

    void GetStatus(const std::string &name, common::CommonMetric &m) const {
        std::map<std::string, std::shared_ptr<SchedulerState>> baseState;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            for (const auto &it: m_state) {
                baseState.emplace(it.first, it.second);
            }
        }
        statistic(name, baseState, m);
    }

    void GetStateMap(std::map<std::string, std::shared_ptr<TSchedulerState>> &stateMap) {
        std::unique_lock<std::mutex> lock(mutex_);
        stateMap = m_state;
    }

protected:
    virtual std::shared_ptr<TSchedulerState> createScheduleItem(const T &s) const = 0;
    virtual int scheduleOnce(TSchedulerState &) = 0;

    void setThreadPool(const std::shared_ptr<ThreadPool> &p) {
        this->threadPool = p;
    }

    void close() {
        endThread();
        {
            // 确保锁定后再notify，避免doRun executeItem后误进入wait状态
            std::unique_lock<std::mutex> lock(this->mutex_);
            this->cv_.notify_all();
        }
        join();

        threadPool.reset();
        clearStateMap();
    }

    void clearStateMap() {
        std::unique_lock<std::mutex> lock(mutex_);
        m_state.clear();
    }

protected:
    VIRTUAL int executeItem(std::weak_ptr<TSchedulerState> weak,
                            const std::chrono::steady_clock::time_point &steadyNow) {
        std::shared_ptr<TSchedulerState> state = weak.lock();
        bool ok = (bool) state;
        if (ok) {
            defer(if (!ok) scheduleNextTime(weak, steadyNow, false));
            LogDebug("run module({})", state->getName());

            auto worker = [=]() {
                auto r = weak.lock();
                if (r) {
                    defer(this->scheduleNextTime(r, steadyNow, true));
                    this->scheduleOnce(*r);
                }
            };

            ok = threadPool->commitTimeout(state->getName(), std::chrono::seconds{1}, worker).valid();
            Log((ok ? LEVEL_DEBUG : LEVEL_WARN), "threadPool->commit({}) => {}", state->getName(), ok);
        } // else {} // 任务已删除

        return ok ? 0 : -1;
    }

    void updateItems(std::shared_ptr<std::map<std::string, T>> &prev) {
        if (prev.get() != this->mItems.get()) {
            prev = mItems;

            std::map<std::string, std::shared_ptr<TSchedulerState>> newState;
            for (auto &newItem: *mItems) {
                auto old = m_state.find(newItem.first);
                if (old == m_state.end()) {
                    if (auto r = createScheduleItem(newItem.second)) {
                        newState[newItem.first] = r;
                        timerQueue.emplace(r);
                    }
                } else {
                    newState.emplace(*old);
                }
            }
            m_state = newState;
        }
    }

    void scheduleNextTime(const std::shared_ptr<TSchedulerState> &state,
                          const std::chrono::steady_clock::time_point &now,
                          bool doLock) {
        state->nextTime = now + state->interval();
        auto steadyNow = std::chrono::steady_clock::now();
        if (steadyNow > state->nextTime) {
            // 运行时长超过一个调度周期
            auto n = (steadyNow - state->nextTime) / state->interval() + 1;
			state->skipCount += static_cast<decltype(state->skipCount)>(n);
            state->nextTime += n * state->interval();
        }

        using namespace std::chrono;
        LogDebug("{}[{}] nextTime: {:L}", common::typeName(this), state->getName(),
                 system_clock::now() + duration_cast<system_clock::duration>(state->nextTime - now));

        std::unique_lock<std::mutex> lock(this->mutex_, std::defer_lock);
        if (doLock) {
            lock.lock();
        }
        bool notify = timerQueue.empty() || timerQueue.begin()->next >= state->nextTime;
        timerQueue.emplace(state);
        if (notify) {
            this->cv_.notify_all();
        }
    }

    void scheduleNextTime(const std::weak_ptr<TSchedulerState> &r,
                          const std::chrono::steady_clock::time_point &now,
                          bool doLock) {
        auto state = r.lock();
        if (state) {
            scheduleNextTime(state, now, doLock);
        }
    }


protected:
    mutable std::mutex mutex_;
    boost::condition_variable_any cv_;

    struct TimerEvent {
        std::chrono::steady_clock::time_point next;
        std::weak_ptr<TSchedulerState> state;

        TimerEvent() = default;

        explicit TimerEvent(const std::shared_ptr<TSchedulerState> &r) {
            this->next = r->nextTime;
            this->state = r;
        }

        TimerEvent(const TimerEvent &) = default;
        TimerEvent &operator=(const TimerEvent &) = default;

        bool operator<(const TimerEvent &r) const {
            return next < r.next;
        }
    };

    std::shared_ptr<std::map<std::string, T>> mItems;
    // 运行时
    std::map<std::string, std::shared_ptr<TSchedulerState>> m_state;
    std::shared_ptr<ThreadPool> threadPool;
    std::multiset<TimerEvent> timerQueue;
};

#include "common/test_support"

}

#endif //ARGUS_CORE_T_SCHEDULER_H
