//
// Created by 韩呈杰 on 2024/2/23.
//
#include "SchedulerT.h"
#include "SchedulerStat.h"

namespace argus {
    void statistic(const std::string &name,
                   const std::map<std::string, std::shared_ptr<SchedulerState>> &state,
                   common::CommonMetric &metric) {
        statStatus(name, state, metric);
    }
}
