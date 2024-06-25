//
// Created by 韩呈杰 on 2024/2/28.
//
// #define ENABLE_SCRIPT_SCHEDULER_2
#ifdef ENABLE_SCRIPT_SCHEDULER_2

#ifndef ARGUS_CORE_SCRIPT_SCHEDULER_2_H
#define ARGUS_CORE_SCRIPT_SCHEDULER_2_H

#include "SchedulerT.h"
#include "TaskManager.h" // ScriptItem

namespace argus {
    struct ScriptScheduler2State : SchedulerState {
        ScriptItem item;

        std::string getName() const override;
        std::chrono::milliseconds interval() const override;
    };

#include "common/test_support"
class ScriptScheduler2 : public SchedulerT<ScriptItem, ScriptScheduler2State> {
public:
    explicit ScriptScheduler2(bool enableThreadPool = true);
    ~ScriptScheduler2() override;

    void GetStatus(common::CommonMetric &m) const {
        SchedulerT::GetStatus("script_status", m);
    }

private:
    std::shared_ptr<ScriptScheduler2State> createScheduleItem(const ScriptItem &s) const override;
    int scheduleOnce(ScriptScheduler2State &) override;
};
#include "common/test_support"

}
#endif //ARGUSAGENT_SCRIPTSCHEDULER2_H

#endif // !ENABLE_SCRIPT_SCHEDULER_2
