//
// Created by 韩呈杰 on 2023/11/20.
//

#ifndef ARGUSAGENT_SIC_EXCEPTIONS_H
#define ARGUSAGENT_SIC_EXCEPTIONS_H
#include <exception>
#include <string>
#include <utility> // std::move

// 预期的失败
class ExpectedFailedException : public std::exception {
    std::string message;
public:
    explicit ExpectedFailedException(std::string s) : message(std::move(s)) {}

    const char *what() const noexcept override {
        return message.c_str();
    }
};

class OverflowException : public std::exception {
};

#endif //ARGUSAGENT_SIC_EXCEPTIONS_H
