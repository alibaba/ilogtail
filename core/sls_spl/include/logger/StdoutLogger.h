#pragma once

#include "Logger.h"

namespace apsara::sls::spl {

class StdoutLogger : public Logger {
   public:
    StdoutLogger(const LOG_FORMAT& logformat = LOG_FORMAT::JSON)
        : logformat_(logformat) {}
    virtual ~StdoutLogger() {}
    virtual void log(const LOG_LEVEL& level, const std::string& file, const int32_t line, const std::string& content);
    virtual bool shouldLog(const LOG_LEVEL& level) {
        return true;
    }
    virtual LOG_FORMAT format() {
        return logformat_;
    }

   private:
    LOG_FORMAT logformat_;
};

}  // namespace apsara::sls::spl