#pragma once

#include <iostream>
#include <memory>
#include <json.hpp>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace apsara::sls::spl {

enum class LOG_LEVEL {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

enum class LOG_FORMAT {
    APSARA,
    JSON
};

std::ostream& operator<<(std::ostream& os, const LOG_LEVEL& p);

class ApsaraLogMaker {
   private:
    std::ostringstream ostringStream_;

   public:
    template <typename T>
    ApsaraLogMaker& operator()(const std::string& key, const T& value) {
        return this->operator()(key.c_str(), value);
    }

    template <typename T>
    ApsaraLogMaker& operator()(const char* key, const T& value) {
        ostringStream_ << "\t" << key << ":" << value;
        return *this;
    }

    ApsaraLogMaker& operator()(const std::string& key, bool value) {
        return this->operator()(key.c_str(), value ? "true" : "false");
    }
    ApsaraLogMaker& operator()(const char* key, bool value) { return this->operator()(key, value ? "true" : "false"); }

    const std::string GetContent() const { return ostringStream_.str(); }
};

class JSONLogMaker {
   private:
    nlohmann::json json_;

   public:
    template <typename T>
    JSONLogMaker& operator()(const std::string& key, const T& value) {
        return this->operator()(key.c_str(), value);
    }

    template <typename T>
    JSONLogMaker& operator()(const char* key, const T& value) {
        json_[key] = value;
        return *this;
    }

    JSONLogMaker& operator()(const std::string& key, bool value) {
        return this->operator()(key.c_str(), value ? "true" : "false");
    }
    JSONLogMaker& operator()(const char* key, bool value) {
        return this->operator()(key, value ? "true" : "false");
    }

    const std::string GetContent() const { return json_.dump(); }
};

class Logger {
   public:
    Logger() {}
    virtual ~Logger() {}
    virtual void log(const LOG_LEVEL& level, const std::string& file, const int32_t line, const std::string& content) = 0;
    virtual bool shouldLog(const LOG_LEVEL& level) = 0;
    virtual LOG_FORMAT format() = 0;
};

using LoggerPtr = std::shared_ptr<Logger>;

#define SPL_LOG_X(logger, fields, level)                                    \
    do {                                                                    \
        if (logger && logger->shouldLog(level)) {                           \
            if (logger->format() == LOG_FORMAT::APSARA) {                   \
                ApsaraLogMaker maker;                                       \
                (void)maker fields;                                         \
                logger->log(level, __FILE__, __LINE__, maker.GetContent()); \
            } else {                                                        \
                JSONLogMaker maker;                                         \
                (void)maker fields;                                         \
                logger->log(level, __FILE__, __LINE__, maker.GetContent()); \
            }                                                               \
        }                                                                   \
    } while (0)

#define SPL_LOG_DEBUG(logger, fields) SPL_LOG_X(logger, fields, LOG_LEVEL::DEBUG)
#define SPL_LOG_INFO(logger, fields) SPL_LOG_X(logger, fields, LOG_LEVEL::INFO)
#define SPL_LOG_WARNING(logger, fields) SPL_LOG_X(logger, fields, LOG_LEVEL::WARNING)
#define SPL_LOG_ERROR(logger, fields) SPL_LOG_X(logger, fields, LOG_LEVEL::ERROR)


}  // namespace apsara::sls::spl