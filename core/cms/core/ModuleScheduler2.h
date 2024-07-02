#ifndef ARGUS_CORE_MODULE_SCHEDULER_2_H
#define ARGUS_CORE_MODULE_SCHEDULER_2_H

#include <mutex>
#include <set>
#include "TaskManager.h"  // ModuleItem
#include "SchedulerT.h"
#include "common/ThreadWorker.h"
#include "common/SyncQueue.h"

class ThreadPool;

namespace common {
    struct CommonMetric;
}

namespace argus {
    class Module;

    struct ModuleScheduler2State: SchedulerState {
        std::chrono::milliseconds lastExecDuration{0};
        std::chrono::milliseconds maxExecDuration{0};
        int continueExceedTimes = 0;  // 连续运行时长超限的次数
        int exceedSkipTimes = 0;      // 运行时长超限后，需暂停的次数

        ModuleItem item;
        std::shared_ptr<Module> p_module;

        std::string getName() const override {
            return item.name;
        }
        std::chrono::milliseconds interval() const override {
            return item.scheduleInterval;
        }
    };

    typedef std::map<std::string, std::shared_ptr<ModuleScheduler2State>> Module2StateMap;

#include "common/test_support"
class ModuleScheduler2: public SchedulerT<ModuleItem, ModuleScheduler2State> {
public:
    explicit ModuleScheduler2(bool enableThreadPool = true);
    ~ModuleScheduler2() override;

    // void doRun() override;

    // void setModuleItems(const std::shared_ptr<std::map<std::string, ModuleItem>> &);
    // std::shared_ptr<std::map<std::string, ModuleItem>> getModuleItems() const;

    void GetStatus(common::CommonMetric &m) const {
        SchedulerT::GetStatus("module_status", m);
    }
    boost::json::value GetStatus(const std::string &mid) const;

private:
    std::shared_ptr<ModuleScheduler2State> createScheduleItem(const ModuleItem &s) const override;
    // std::shared_ptr<ModuleScheduler2State> createScheduleItem(const ModuleItem &s) const override;
    //
    // int executeItem(std::weak_ptr<ModuleScheduler2State> state,
    //                 const std::chrono::steady_clock::time_point &);
    // void updateModuleItems(std::shared_ptr<std::map<std::string, ModuleItem>> &prev);
    //
    // void scheduleNextTime(const std::weak_ptr<ModuleScheduler2State> &, const std::chrono::steady_clock::time_point &);
    int scheduleOnce(ModuleScheduler2State &) override;
    void afterScheduleOnce(ModuleScheduler2State &, const std::chrono::milliseconds &) const;

    VIRTUAL std::shared_ptr<Module> makeModule(const ModuleItem &) const;

private:
    // // 运行时
    // std::map<std::string, std::shared_ptr<ModuleScheduler2State>> m_state;
    //
    // std::shared_ptr<ThreadPool> threadPool;

    int mMaxModuleExecuteTimeRatio = 3;
    int mContinueExceedCount = 0;
//
// private:
//     mutable std::mutex mutex_;
//     std::condition_variable cv_;
//
//     struct TimerEvent {
//         std::chrono::steady_clock::time_point next;
//         std::weak_ptr<ModuleScheduler2State> state;
//
//         TimerEvent() = default;
//         explicit TimerEvent(const std::shared_ptr<ModuleScheduler2State> &r);
//
//         TimerEvent(const TimerEvent &) = default;
//         TimerEvent &operator=(const TimerEvent &) = default;
//
//         bool operator<(const TimerEvent &r) const {
//             return next < r.next;
//         }
//     };
//     std::multiset<TimerEvent> timerQueue;
//     std::shared_ptr<std::map<std::string, ModuleItem>> mModuleItems;
};
#include "common/test_support"

}

#endif
