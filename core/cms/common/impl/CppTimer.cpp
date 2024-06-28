//
// Created by 韩呈杰 on 2023/7/11.
//
#ifdef WIN32
#include <WinSock2.h>
#endif

#include <atomic>
#include <boost/core/demangle.hpp>

#include "common/CppTimer.h"
#include "common/Logger.h"
#include "common/ThrowWithTrace.h"
#include "common/Chrono.h"

namespace CppTime {
    static std::atomic<unsigned int> timerId{0};

    namespace detail {
        void Time_event::updateNextTime(const clock::time_point &now, const duration &period) {
            this->next += period;
            if (this->next < now) {
                // 由于精度源因，diff可能为0，此时直接跳到下一个周期即可
                auto diff = std::chrono::duration_cast<duration>(now - this->next);
                auto mod = diff % period;
                if (diff.count() == 0 || mod.count() > 0) {
                    diff += period - mod;
                }
                this->next += diff;
            }
        }
    }

    Timer::Timer()
    {
        scoped_m lock(m);
        done = false;
        worker = std::thread([this] { run(); });
    }

    Timer::~Timer() {
        scoped_m lock(m);
        done = true;
        lock.unlock();
        cond.notify_all();
        if (worker.joinable()) {
            worker.join();
        }

        events.clear();
        time_events.clear();
    }

    /**
     * Add a new timer.
     *
     * \param when The time at which the handler is invoked.
     * \param handler The callable that is invoked when the timer fires.
     * \param period The periodicity at which the timer fires. Only used for periodic timers.
     */
    timer_id Timer::add(const timestamp &when, handler_t &&handler, const duration &period) {
        timer_id id = ++timerId;
        scoped_m lock(m);

        // 避免碰撞
        while(id == 0 || events.end() != events.find(id)) {
            id = ++timerId;
        }
        auto item = std::make_shared<detail::Event>(id, when, period, std::move(handler));
        events.emplace(id, item);
        time_events.insert(detail::Time_event{when, item});

        cond.notify_all();
        lock.unlock();
        return id;
    }

    void Timer::remove(timer_id id) {
        scoped_m lock(m);
        events.erase(id);
        cond.notify_all();
        lock.unlock();
    }

    void Timer::run() {
        scoped_m lock(m);

        while (!done) {
#ifdef ENABLE_COVERAGE
            loopCount++;
#endif
            if (time_events.empty()) {
                // Wait for work
                cond.wait(lock);
            } else {
                detail::Time_event te = *time_events.begin();
                auto steadyNow = CppTime::clock::now();
                if (steadyNow < te.next) {
                    cond.wait_for(lock, boost::chrono::microseconds(ToMicros(te.next - steadyNow)));
                } else {
                    // Remove time event
                    time_events.erase(time_events.begin());
                    // Invoke the handler
                    lock.unlock();

                    try {
                        std::shared_ptr<detail::Event> curEvent = te.event.lock();
                        if (curEvent) {
                            curEvent->handler(curEvent->id);
                        }
                    } catch (const std::exception &ex) {
                        std::string trace;
                        if (const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(ex)) {
                            std::stringstream ss;
                            ss << ", trace: \n" << boost::stacktrace::mt::to_string(*st);
                            trace = ss.str();
                        }
                        LogError("{}: {}{}", boost::core::demangle(typeid(ex).name()), ex.what(), trace);
                    }

                    lock.lock();
                    {
                        // period为0时，表示一次性任务
                        std::shared_ptr<detail::Event> curEvent = te.event.lock();
                        if (curEvent && curEvent->period.count() > 0) {
                            te.updateNextTime(clock::now(), curEvent->period);
                            time_events.insert(te);
                        }
                    }
                }
            }
        }
    }
}