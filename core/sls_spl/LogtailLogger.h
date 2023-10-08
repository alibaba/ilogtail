#pragma once

#include "logger/SplLogger.h"

namespace apsara::sls::spl {

class LogtailLogger : public Logger {
   public:
    LogtailLogger() {}
    virtual ~LogtailLogger() {}
    virtual void log(const LOG_LEVEL& level, const std::string& file, const int32_t line, const std::string& content);
    virtual bool shouldLog(const LOG_LEVEL& level) {
        return true;
    }
    virtual LOG_FORMAT format() {
        return LOG_FORMAT::JSON;
    }
};

}  // namespace apsara::sls::spl