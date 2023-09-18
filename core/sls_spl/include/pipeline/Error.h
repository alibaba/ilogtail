#pragma once

#include <ostream>
#include <json.hpp>

namespace apsara::sls::spl {

enum class StatusCode {
    OK,
    TIMEOUT_ERROR,
    USER_ERROR,
    FATAL_ERROR,
    SPLIT_TOO_LARGE,
    MEM_EXCEEDED,
    IGNORE  // for short path or no spl script after *
};

std::ostream& operator<<(std::ostream& os, const StatusCode& p);

struct Error {
    StatusCode code_;
    std::string msg_;
    Error() {
        set(StatusCode::OK, "");
    }

    // check is Error is ok
    bool ok() {
        return code_ == StatusCode::OK;
    }
    void reset() {
        set(StatusCode::OK, "");
    }
    void set(const StatusCode& statusCode, const std::string& message) {
        code_ = statusCode;
        msg_ = message;
    }
};
void to_json(nlohmann::json& j, const Error& p);

}  // namespace apsara::sls::spl