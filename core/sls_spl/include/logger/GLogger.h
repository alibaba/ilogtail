#pragma once

#include "Logger.h"

namespace apsara::sls::spl {

class GLogger : public Logger {
   public:
    GLogger() {}
    virtual ~GLogger() {}
    virtual void log(const LOG_LEVEL& level, const std::string& file, const int32_t line, const std::string& content);
    virtual bool shouldLog(const LOG_LEVEL& level) {
        return true;
    }
    virtual LOG_FORMAT format() {
        return LOG_FORMAT::JSON;
    }
};

}  // namespace apsara::sls::spl