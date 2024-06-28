#ifndef ARGUS_COMMON_CPP_TIMER_H
#define ARGUS_COMMON_CPP_TIMER_H

/**
 * C++11 timer component
 * =====================
 *
 * A portable, header-only C++11 timer component.
 *
 * Overview
 * --------
 *
 * This component can be used to manage a set of timeouts. It is implemented in
 * pure C++11. It is therefore very portable given a compliant compiler.
 *
 * A timeout can be added with one of the `add` functions, and removed with the
 * `remove` function. A timeout can be either one-shot or periodic. In case a
 * timeout is one-shot, the callback is invoked once and the timeout event is
 * then automatically removed. If the timer is periodic, it is never
 * automatically removed, but always renewed.
 *
 * Removing a timeout is possible even from within the callback.
 *
 * Timeout Units
 * -------------
 *
 * The preferred functions for adding timeouts are those that take a
 * `std::chrono::...` argument. However, for convenience, there is also an API
 * that takes a uint64_t. When using this API, all values are expected to be
 * given in microseconds (us).
 *
 * For periodic timeouts, a separate timeout can be specified for the initial
 * (first) timeout, and the periodicity after that.
 *
 * To avoid drifts, times are added by simply adding the period to the intially
 * calculated (or provided) time. Also, we use `wait until` type of API to wait
 * for a timeout instead of a `wait for` API.
 *
 * Data Structure
 * --------------
 *
 * Internally, a std::vector is used to store timeout events. The timer_id
 * returned from the `add` functions are used as index to this vector.
 *
 * In addition, a std::multiset is used that holds all time points when
 * timeouts expire.
 *
 * Using a vector to store timeout events has some implications. It is very
 * fast to remove an event, because the timer_id is the vector's index. On the
 * other hand, this makes it also more complicated to manage the timer_ids. The
 * current solution is to keep track of ids that are freed in order to re-use
 * them. A stack is used for this.
 *
 * Examples
 * --------
 *
 * More examples can be found in the `tests` folder.
 *
 * ~~~
 * CppTime::Timer t;
 * t.add(std::chrono::seconds(1), [](CppTime::timer_id){ std::cout << "got it!"; });
 * std::this_thread::sleep_for(std::chrono::seconds(2));
 * ~~~
 */

// Includes
#include <functional>
#include <thread>
#include <mutex>
#include <boost/thread/condition_variable.hpp>
#include <chrono>
#include <map>
#include <set>
#include <atomic>

namespace CppTime {
    // Public types
    using timer_id = unsigned int; // std::size_t;
    using handler_t = std::function<void(timer_id)>;
    using clock = std::chrono::steady_clock;
    using timestamp = std::chrono::time_point<clock>;
    using duration = std::chrono::microseconds;

    // Private definitions. Do not rely on this namespace.
    namespace detail {

        // The event structure that holds the information about a timer.
        struct Event {
            timer_id id = 0;
            timestamp start{duration::zero()};
            duration period{duration::zero()};
            handler_t handler{nullptr};

            Event() = default;

            template<typename Func>
            Event(timer_id id, timestamp start, duration period, Func &&handler)
                    : id(id), start(start), period(period), handler(std::forward<Func>(handler)) {
            }
        };

        // A time event structure that holds the next timeout and a reference to its
        // Event struct.
        struct Time_event {
            timestamp next;
            std::weak_ptr<Event> event;

            void updateNextTime(const clock::time_point &, const duration &);
        };

        inline bool operator<(const Time_event &l, const Time_event &r) {
            return l.next < r.next;
        }

    } // end namespace detail

#include "test_support"
class Timer {
private:
    using scoped_m = std::unique_lock<std::mutex>;

    // Thread and locking variables.
    std::mutex m;
    boost::condition_variable_any cond;
    std::thread worker;

    // Use to terminate the timer thread.
    std::atomic<bool> done{false};

    // The vector that holds all active events.
    std::map<timer_id, std::shared_ptr<detail::Event>> events;
    // Sorted queue that has the next timeout at its top.
    std::multiset<detail::Time_event> time_events;

    // A list of ids to be re-used. If possible, ids are used from this pool.
    // std::stack<CppTime::timer_id> free_ids;
#ifdef ENABLE_COVERAGE
    std::atomic<unsigned int> loopCount{0};
#endif

public:
    Timer();
    ~Timer();

    /**
     * Add a new timer.
     *
     * \param when The time at which the handler is invoked.
     * \param handler The callable that is invoked when the timer fires.
     * \param period The periodicity at which the timer fires. Only used for periodic timers.
     */
    timer_id add(const timestamp &when, handler_t &&handler, const duration &period = duration::zero());

    template<typename Rep, typename Period>
    timer_id add(const timestamp &when, handler_t &&handler, const std::chrono::duration<Rep, Period> &period) {
        return add(when, std::forward<handler_t>(handler), std::chrono::duration_cast<duration>(period));
    }

    /**
     * Overloaded `add` function that uses a `std::chrono::duration` instead of a
     * `time_point` for the first timeout.
     */
    template<class Rep, class Period>
    timer_id add(const std::chrono::duration<Rep, Period> &delay,
                 handler_t &&handler,
                 const duration &period = duration::zero()) {
        auto when = clock::now() + std::chrono::duration_cast<std::chrono::microseconds>(delay);
        return add(when, std::move(handler), period);
    }

    /**
     * Overloaded `add` function that uses a uint64_t instead of a `time_point` for
     * the first timeout and the period.
     */
    inline timer_id add(uint64_t microsDelay, handler_t &&handler, uint64_t microsPeriod = 0) {
        return add(duration(microsDelay), std::move(handler), duration(microsPeriod));
    }

    /**
     * Removes the timer with the given id.
     */
    void remove(timer_id id);

private:
    void run();
};
#include "test_support"

} // end namespace CppTime

// 全局定时器
#include "Singleton.h"
typedef Singleton<CppTime::Timer> SingletonTimer;

#endif //ARGUS_COMMON_CPP_TIMER_H
