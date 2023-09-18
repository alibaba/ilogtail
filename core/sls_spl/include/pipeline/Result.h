#pragma once

#include <unordered_map>
#include "Error.h"
#include "task/TaskData.h"

namespace apsara::sls::spl {

using TaskResults = std::unordered_map<uint32_t, TaskData>;

struct PipelineResult {
    Error error_;
    TaskResults taskResults_;
    void set(const StatusCode& code, const std::string& msg, const TaskResults& results) {
        error_.code_ = std::move(code);
        error_.msg_ = msg;
        taskResults_ = results;
    }
    void set(const StatusCode& code, const TaskResults& results) {
        set(code, "", results);
    }
};

using PipelineResultPair = std::pair<StatusCode, TaskResults>;

}  // namespace apsara::sls::spl