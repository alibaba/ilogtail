//
// Created by 韩呈杰 on 2024/2/29.
//

#ifndef ARGUSAGENT_SCHEDULERSTAT_H
#define ARGUSAGENT_SCHEDULERSTAT_H

#include <vector>
#include <map>
#include "common/CommonMetric.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"

template<typename T>
void statStatus(const std::string &name,
                const std::map<std::string, std::shared_ptr<T>> &state,
                common::CommonMetric &metric) {
    std::vector<std::string> errorTasks;
    std::vector<std::string> okTasks;
    std::vector<std::string> skipTasks;
    size_t totalNumber;

    totalNumber = state.size();
    for (auto &it: state) {
        if (it.second->errorCount > 0) {
            errorTasks.push_back(it.second->getName());
        }
        if (it.second->skipCount > 0) {
            skipTasks.push_back(it.second->getName());
        }
        if (it.second->skipCount == 0 && it.second->errorCount == 0) {
            okTasks.push_back(it.second->getName());
        }
        it.second->skipCount = 0;
        it.second->errorCount = 0;
    }

    metric.value = (!errorTasks.empty() || !skipTasks.empty() ? 1 : 0);
    metric.name = name; // "module_status";
    metric.timestamp = NowSeconds(); // apr_time_now() / (1000 * 1000);
    metric.tagMap["number"] = common::StringUtils::NumberToString(totalNumber);
    metric.tagMap["ok_list"] = common::StringUtils::join(okTasks, ",");
    metric.tagMap["error_list"] = common::StringUtils::join(errorTasks, ",");
    metric.tagMap["skip_list"] = common::StringUtils::join(skipTasks, ",");
}

#endif //ARGUSAGENT_SCHEDULERSTAT_H
