#ifndef ARGUS_COMMON_LOGGER_H
#define ARGUS_COMMON_LOGGER_H

#include <string>
#include <cstdarg>
#include <vector>
#include <mutex>
#include <chrono>
#include <fmt/format.h>  // C++20 style format
#include <fmt/printf.h>  // C style printf
#include <fmt/chrono.h>  // std::chrono::duration
#include <fmt/ostream.h> // support std::ostream

#include "common/TimeFormat.h"
#include "common/Singleton.h"
#include "common/FilePathUtils.h"
#include "common/BoostStacktraceMt.h"

class Logger;
struct ILoggerImpl;
class FileLogger;

#ifdef WIN32
#   define CRLF "\r\n"
#   define CRLF_LEN 2
#else
#   define CRLF "\n"
#   define CRLF_LEN 1
#endif

struct LoggerFileInfo {
    std::chrono::system_clock::time_point ctime;
    std::chrono::system_clock::time_point mtime;
    fs::path file;
};

enum LogLevel {
    LEVEL_ERROR = 1,  // some error message
    LEVEL_WARN = 2,   // some warning
    LEVEL_INFO = 3,   // normal information
    LEVEL_DEBUG = 4,  // debug information
    LEVEL_TRACE = 5,   // trace information
};
struct LogLevelMeta {
    const LogLevel level;
    const char *paddedName;      // 对齐的名称，后面补空格
    const std::string nameLower; // 小写的名称
    const std::string nameUpper; // 大写的名称

    static const LogLevelMeta *find(LogLevel);
    static const LogLevelMeta *find(const std::string &);
};


size_t EnumerateLoggerFiles(const fs::path &dir, const std::string &fileNamePrefix,
                            const std::function<void(const LoggerFileInfo &)> &cb);
std::vector<LoggerFileInfo> EnumerateLoggerFiles(const fs::path &dir, const std::string &filename, bool sort);

#include "common/test_support"

class Logger {
public:
    explicit Logger(bool simple = false);
    VIRTUAL ~Logger();

    VIRTUAL bool setup(const std::string &file, size_t perSize = 0, size_t count = 0);
    VIRTUAL void doLoggingStr(LogLevel level, const char *file, int line, const std::string &s);

    LogLevel swapLevel(LogLevel newLevel);
    void setLogLevel(const std::string &level);
    LogLevel getLogLevel(std::string &level) const;

    bool isLevelOK(LogLevel level) const {
        return m_level >= level;
    }

    void setCount(size_t count) const;
    void setFileSize(size_t size) const;
    size_t getFileSize() const;

    //获取该日志的所有文件、并按照修改时间降序排列
    std::vector<LoggerFileInfo> getLogFiles() const;
    void setToStdout();
    void switchLogCache(bool flag);
    std::string getLogCache(bool clear = false) const;
    void clearLogCache();
private:
    static const char *getLevelName(LogLevel level);
    std::string activeLoggerName() const;

private:
    mutable std::timed_mutex m_lock;
    // 两层日志模型
    std::shared_ptr<FileLogger> fileLogger;
    std::shared_ptr<ILoggerImpl> activeLogger;
    LogLevel m_level = LEVEL_INFO;//日志等级

    const bool m_simple = false; // true - 只显示时间和数据
};

#include "common/test_support"

typedef Singleton<Logger> SingletonLogger;


namespace log_details {
    void PrintLog(Logger *l, LogLevel level, const char *file, int line, const std::string &msg, const char *pattern);
}

#define IMPLEMENT_PRINT_LOG(F1, F2, E_R) \
    template<typename ...Args>                                                                              \
    void F1(Logger *l, LogLevel level, const char *file, int line, const char *pattern, Args &&...args) {   \
        std::string msg;                                                                                    \
        try {                                                                                               \
            msg = (sizeof...(args) == 0 ?                                                                   \
                   std::string{pattern == nullptr ? "" : pattern} :                                         \
                   (F2)(pattern, std::forward<Args>(args)...));                                             \
        } catch (const std::exception &ex) {                                                                \
            boost::stacktrace::stacktrace st;                                                               \
            msg = fmt::format("unexpected C++ exception: {}, pattern: {}\n{}",                              \
                              ex.what(), (pattern? pattern: "null"), boost::stacktrace::mt::to_string(st)); \
            log_details::PrintLog(l, LEVEL_ERROR, file, line, msg, pattern);                                \
            E_R;                                                                                            \
        }                                                                                                   \
                                                                                                            \
        log_details::PrintLog(l, level, file, line, msg, pattern);                                          \
    }

// fmt::sprintf、fmt::format不能放到一个函数中通过分支来调用
#ifdef ENABLE_COVERAGE
IMPLEMENT_PRINT_LOG(PrintfLog, ::fmt::sprintf, throw)
IMPLEMENT_PRINT_LOG(FmtLog,    ::fmt::format, throw)
#else
IMPLEMENT_PRINT_LOG(PrintfLog, ::fmt::sprintf, return)
IMPLEMENT_PRINT_LOG(FmtLog,    ::fmt::format, return)
#endif

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// C Style - type safe
// template<typename ...Args>
// void PrintfLog(Logger *, LogLevel level, const char *file, int line, const char *pattern, Args &&...args);

#define LogOutput(I, F, level, pattern, ...) do {                                    \
    auto *p_argus_log_2024_ = (I);                                                   \
    if (p_argus_log_2024_->isLevelOK(level)) {                                       \
        F(p_argus_log_2024_, (level), __FILE__, __LINE__, (pattern), ##__VA_ARGS__); \
    }                                                                                \
    } while(false)

#define LogPrint(level, pattern, ...)  LogOutput(SingletonLogger::Instance(), PrintfLog, (level), (pattern), ##__VA_ARGS__)
#define LogPrintError(pattern, ...) LogPrint(LEVEL_ERROR, (pattern), ##__VA_ARGS__)
#define LogPrintWarn(pattern, ...)  LogPrint(LEVEL_WARN,  (pattern), ##__VA_ARGS__)
#define LogPrintInfo(pattern, ...)  LogPrint(LEVEL_INFO,  (pattern), ##__VA_ARGS__)
#define LogPrintDebug(pattern, ...) LogPrint(LEVEL_DEBUG, (pattern), ##__VA_ARGS__)
#define LogPrintTrace(pattern, ...) LogPrint(LEVEL_TRACE, (pattern), ##__VA_ARGS__)


/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// C++20 Style
// template<typename ...Args>
// void FmtLog(Logger *, LogLevel level, const char *file, int line, const char *pattern, Args &&...args);

template<typename ...Args>
void FmtLog(Logger *l, LogLevel level, const char *file, int line, const std::string &pattern, Args &&...args) {
    FmtLog(l, level, file, line, pattern.c_str(), std::forward<Args>(args)...);
}

#define LOG(I, level, pattern, ...) LogOutput((I), FmtLog, (level), (pattern), ##__VA_ARGS__)

#define LOG_E(I, pattern, ...) LOG(I, LEVEL_ERROR, (pattern), ##__VA_ARGS__)
#define LOG_W(I, pattern, ...) LOG(I, LEVEL_WARN,  (pattern), ##__VA_ARGS__)
#define LOG_I(I, pattern, ...) LOG(I, LEVEL_INFO,  (pattern), ##__VA_ARGS__)
#define LOG_D(I, pattern, ...) LOG(I, LEVEL_DEBUG, (pattern), ##__VA_ARGS__)
#define LOG_T(I, pattern, ...) LOG(I, LEVEL_TRACE, (pattern), ##__VA_ARGS__)

#define Log(level, pattern, ...) LOG(SingletonLogger::Instance(), level, (pattern), ##__VA_ARGS__)
#define LogError(pattern, ...)   LOG_E(SingletonLogger::Instance(), (pattern), ##__VA_ARGS__)
#define LogWarn(pattern, ...)    LOG_W(SingletonLogger::Instance(), (pattern), ##__VA_ARGS__)
#define LogInfo(pattern, ...)    LOG_I(SingletonLogger::Instance(), (pattern), ##__VA_ARGS__)
#define LogDebug(pattern, ...)   LOG_D(SingletonLogger::Instance(), (pattern), ##__VA_ARGS__)
#define LogTrace(pattern, ...)   LOG_T(SingletonLogger::Instance(), (pattern), ##__VA_ARGS__)

namespace common {
    using ::LogLevel;
    using ::Logger;
}

#endif // ARGUS_COMMON_LOGGER_H
