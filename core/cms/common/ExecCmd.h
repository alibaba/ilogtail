//
// Created by 韩呈杰 on 2024/5/8.
//

#ifndef ARGUSAGENT_COMMON_EXEC_CMD_H
#define ARGUSAGENT_COMMON_EXEC_CMD_H

#include <functional>
#include <string>
#include <vector>

struct tagExecCmdOption {
    std::function<std::string(const std::string &)> fnTrim;
    tagExecCmdOption(); // TrimSpace
};

int ExecCmd(const std::string &cmd, std::string *out = nullptr, const tagExecCmdOption *option = nullptr);
// 逐行
int ExecCmd(const std::string &cmd, std::vector<std::string> &out, bool enableEmptyLine = true);

#endif //ARGUSAGENT_COMMON_EXEC_CMD_H
