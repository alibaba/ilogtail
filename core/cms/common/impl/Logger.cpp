#ifdef WIN32
#   include <windows.h>
#   include <TlHelp32.h>
#endif
#include "common/Logger.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "common/ArgusMacros.h"
#include "common/StringUtils.h"
#include "common/Chrono.h"
#include "common/ChronoLiteral.h"
#include "common/Common.h" // GetThisThreadId

#ifdef min
#	undef min
#	undef max
#endif

using namespace std;
namespace fs = boost::filesystem;

extern const size_t SRC_SRC_OFFSET;

static const LogLevelMeta logLevelMeta[] = {
        {LogLevel(0), "0    ", "0",     "0"},
        {LEVEL_ERROR, "ERROR", "error", "ERROR"},
        {LEVEL_WARN,  "WARN ", "warn",  "WARN"},
        {LEVEL_INFO,  "INFO ", "info",  "INFO"},
        {LEVEL_DEBUG, "DEBUG", "debug", "DEBUG"},
        {LEVEL_TRACE, "TRACE", "trace", "TRACE"},
};
static constexpr size_t logLevelMetaSize = sizeof(logLevelMeta) / sizeof(logLevelMeta[0]);

const LogLevelMeta *LogLevelMeta::find(LogLevel level) {
    int index = (int)level;
    return 0 <= index && index < logLevelMetaSize ? logLevelMeta + index : nullptr;
}

const LogLevelMeta *LogLevelMeta::find(const std::string &levelName) {
    auto nameLower = ToLower(levelName);
    for (const auto &curLevel: logLevelMeta) {
        if (curLevel.nameLower == nameLower) {
            return &curLevel;
        }
    }
    return nullptr;
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
// /opt/argusagent/src/sic/src/system_informaton_collector.cpp => /opt/argusagent/src
#ifdef WIN32
#   define PATH_SEP "\\"
#else
#   define PATH_SEP "/"
#endif
#ifdef ONE_AGENT
#   define SRC_ROOT PATH_SEP "core" PATH_SEP       //: /core/
#   define COMMON_ROOT SRC_ROOT "cms" PATH_SEP "common" PATH_SEP  //: /core/cms/common/
#else
#   define SRC_ROOT PATH_SEP "src" PATH_SEP        //: /src/
#   define COMMON_ROOT SRC_ROOT "common" PATH_SEP  //: /src/common/
#endif
// 源码根目录(不含尾部路径分隔符)
extern const size_t SRC_ROOT_OFFSET = []() {
    std::string thisFile{__FILE__};
    size_t pos = thisFile.rfind(COMMON_ROOT);
#ifdef ENABLE_COVERAGE
    if (pos == std::string::npos) {
    //     pos = thisFile.substr(0, pos).rfind(SRC_KEY);
    // } else {
        std::cerr << "unexpected thisFile: " << __FILE__ << std::endl;
        exit(1);
    }
#endif
    return pos + 1; // skip path separator
}();
// 源码根目录下src目录(含尾部路径分隔符)
extern const size_t SRC_SRC_OFFSET = SRC_ROOT_OFFSET - 1 + strlen(SRC_ROOT);
#undef COMMON_ROOT
#undef SRC_ROOT

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LoggerInterface
struct ILoggerImpl {
    virtual ~ILoggerImpl() = 0;
    virtual int write(const std::string &line) = 0;
    virtual std::string name() const = 0;

protected:
    static int lineBytes(const std::string &line) {
        return static_cast<int>(line.size() + CRLF_LEN);
    }
};

ILoggerImpl::~ILoggerImpl() = default;

class CacheLogger final : public ILoggerImpl {
    std::string mCache;
public:
    CacheLogger() {
        reset(false);
    }

    ~CacheLogger() override = default;

    int write(const std::string &line) override {
        mCache.append(line).append(CRLF);
        return lineBytes(line);
    }

    std::string name() const override {
        return "CacheLogger";
    }

    void reset(const bool reserved = true) {
        mCache = "";
        if (reserved) {
            mCache.reserve(0x100000);
        }
    }

    const std::string &cache() const {
        return mCache;
    }
};

class StdOutLogger final : public ILoggerImpl {
public:
    ~StdOutLogger() override = default;

    int write(const std::string &line) override {
        std::cout << line << CRLF;
        return lineBytes(line);
    }

    std::string name() const override {
        return "StdOutLogger";
    }
};

size_t EnumerateLoggerFiles(const fs::path &dir, const std::string &fileNamePrefix,
                            const std::function<void(const LoggerFileInfo &)> &cb) {
    size_t count = 0;
    for (const auto &dirEntry: fs::directory_iterator{dir, fs::directory_options::skip_permission_denied}) {
        if (fs::is_regular_file(dirEntry)) {
            fs::path path = dirEntry.path();
            std::string curFileName = path.filename().string();
            if (fileNamePrefix.empty() || HasPrefix(curFileName, fileNamePrefix)) {
                LoggerFileInfo info;
                info.file = path;
                boost::system::error_code ec;
                info.ctime = std::chrono::FromSeconds(fs::creation_time(path, ec));
                // if (ec.failed()) {
                //     // Linux下不是所有平台都支持文件创建时间的获取
                //     LogError("fs::creation_time({}), ec: {}", path.string(), ec.message());
                // }
                info.mtime = std::chrono::FromSeconds(fs::last_write_time(path, ec));
                cb(info);
            }
        }
    }
    return count;
}

std::vector<LoggerFileInfo> EnumerateLoggerFiles(const fs::path &dir, const std::string &filename, bool sort) {
    std::vector<LoggerFileInfo> vecFiles;
    EnumerateLoggerFiles(dir, filename, [&vecFiles](const LoggerFileInfo &r) { vecFiles.push_back(r); });

    if (sort && vecFiles.size() > 1) {
        std::sort(vecFiles.begin(), vecFiles.end(), [&](const LoggerFileInfo &l, const LoggerFileInfo &r) {
            const auto lname = l.file.filename().string();
            if (lname == filename) {
                return true;
            }
            const auto rname = r.file.filename().string();
            if (rname == filename) {
                return false;
            }
            return l.mtime > r.mtime || (l.mtime == r.mtime && lname > rname);
        });
    }

    RETURN_RVALUE(vecFiles);
}

class FileLogger final : public ILoggerImpl {
    std::string mFile;
    size_t mSize = 10 * 1024 * 1024;//单个日志文件的大小
    size_t mCount = 7;//该日志的文件数量
    std::ofstream ofs;
public:
    ~FileLogger() override {
        if (ofs.is_open()) {
            ofs.close();
        }
    }

    bool isOK() const {
        return !mFile.empty();
    }

    void setFileSize(size_t size) {
        if (size > 0) {
            mSize = size;
        }
    }

    size_t getFileSize() const {
        return mSize;
    }

    void setMaxCount(size_t count) {
        if (count > 0) {
            mCount = count;
        }
    }

    int write(const std::string &line) override {
        if (ofs.is_open()) {
            constexpr size_t maxLineBytes = 8192;

            size_t lineBytes = line.size();
            if (lineBytes <= maxLineBytes) {
				ofs << line << std::endl; // CRLF;
            } else {
                lineBytes = maxLineBytes;
				ofs << line.substr(0, maxLineBytes) << std::endl; // CRLF;
            }
            ofs.flush();

            if (shouldRotate()) {
                rotate();
            }
            return static_cast<int>(lineBytes + CRLF_LEN);
        }
        return 0;
    }

    std::string name() const override {
        return "FileLogger";
    }

    bool setup(const std::string &file, const size_t perSize = 0, const size_t count = 0) {
        if (!file.empty() && (!ofs.is_open() || file != mFile)) {
            const fs::path filePath{file};
            const fs::path dirPath = filePath.parent_path();
            LogInfo("log dir: {}", dirPath.string());

            {
                boost::system::error_code ec;
                if (!fs::exists(dirPath) && !fs::create_directories(dirPath, ec)) {
                    LogError("can't create dir [{}], errorMessage is [{}]", dirPath.string(), ec.message());
                    return false;
                }
            }

            if (ofs.is_open()) {
                ofs.close();
            }
            ofs.open(file, std::ios_base::out | std::ios_base::app | std::ios_base::ate);
            if (!ofs.is_open()) {
                LogError("open({}, \"out|app|ate\") failed", file);
                return false;
            }
            mFile = file;

            {
                boost::system::error_code ec;
                fs::permissions(file, fs::owner_read | fs::owner_write | fs::group_read | fs::others_read, ec);
                if (ec.failed()) {
                    LogError("chmod({}), error: {}", file, ec.message());
                }
            }
        }

        setFileSize(perSize);
        setMaxCount(count);

        return true;
    }

    std::vector<LoggerFileInfo> enumerateLogFiles() {
        fs::path filePath{this->mFile};
        return EnumerateLoggerFiles(filePath.parent_path(), filePath.filename().string(), true);
    }

private:
    bool shouldRotate() {
        return ofs.is_open() && static_cast<size_t>(ofs.tellp()) >= mSize;
    }

    void rotate() {
        // if the filename exists, change it into the "filename.yyyymmddhhnnss" as old log file.
        using namespace std::chrono;
        const std::string strNow = date::format(system_clock::now(), "L%Y%m%d%H%M%S");
        std::string oldFile = mFile + "." + strNow;

        // conflict avoid
        std::string suffix;
        for (int i = 0; fs::exists(oldFile + suffix); i++) {
            suffix = fmt::format("-{:03d}", i);
        }
        if (!suffix.empty()) {
            oldFile += suffix;
        }

        ofs.close();
        fs::rename(mFile, oldFile);

        if (!setup(mFile)) {
            return;
        }

        const vector<LoggerFileInfo> vecLogFiles = enumerateLogFiles();
        for (size_t i = mCount; i < vecLogFiles.size(); i++) {
            boost::system::error_code ec;
            fs::remove(vecLogFiles[i].file, ec);
            if (ec.failed()) {
                LogError("fs::remove({}), error: {}", vecLogFiles[i].file.string(), ec.message());
            }
        }
    }
};

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Logger
Logger::Logger(bool simple) : m_simple(simple) {
    fileLogger = std::make_shared<FileLogger>();
#ifdef ENABLE_COVERAGE
    m_level = LEVEL_TRACE;
#else
    m_level = LEVEL_DEBUG;
#endif
    setToStdout();
}

Logger::~Logger() = default;

std::string Logger::activeLoggerName() const {
    return activeLogger->name();
}

void Logger::setCount(size_t count) const {
    fileLogger->setMaxCount(count);
}

void Logger::setFileSize(size_t size) const {
    fileLogger->setFileSize(size);
}

size_t Logger::getFileSize() const {
    return fileLogger->getFileSize();
}

void Logger::setToStdout() {
    activeLogger = std::make_shared<StdOutLogger>();
}

std::vector<LoggerFileInfo> Logger::getLogFiles() const {
    return fileLogger->enumerateLogFiles();
}

bool Logger::setup(const std::string &file, size_t perSize, size_t count) {
    bool ok = fileLogger->setup(file, perSize, count);
    if (ok) {
        activeLogger = fileLogger;
    }
    return ok;
}

void Logger::doLoggingStr(LogLevel level, const char *file, int line, const std::string &s) {
	if (!this->isLevelOK(level)) {
		return;
	}

    const auto now = std::chrono::system_clock::now();

#ifdef WIN32
#   define TID_WIDTH 4
#else
#   define TID_WIDTH 5
#endif
    string strLine;
    if (m_simple) {
        strLine = fmt::format("[{}] {}", date::format(now, 3), TrimRightSpace(s));
    } else {
        strLine = fmt::format("[{}] {} [{:<{}d}] {}:{}: {}", getLevelName(level), date::format(now, 3),
                              GetThisThreadId(), TID_WIDTH, (file + SRC_SRC_OFFSET), line, TrimRightSpace(s));
    }
#undef TID_WIDTH

    // 避免无限期等待，5秒拿不到锁，就放弃该条日志
    const std::unique_lock<std::timed_mutex> lg(this->m_lock, 5_s);
    if (lg.owns_lock()) {
        activeLogger->write(strLine);
    }
}

LogLevel Logger::getLogLevel(string &level) const {
    if (auto *meta = LogLevelMeta::find(m_level)) {
        level = meta->nameLower;
    }
    return m_level;
}

LogLevel Logger::swapLevel(LogLevel newLevel) {
#ifdef ENABLE_COVERAGE
    auto *newMeta = LogLevelMeta::find(newLevel);
    auto *oldMeta = LogLevelMeta::find(m_level);
    if (newMeta && oldMeta) {
        FmtLog(this, LEVEL_INFO, __FILE__, __LINE__, "{}({}) => {}", __FUNCTION__, oldMeta->nameLower, newMeta->nameLower);
    }
#endif

    std::swap(newLevel, m_level);
    return newLevel;
}

void Logger::setLogLevel(const std::string &level) {
    if (auto *meta = LogLevelMeta::find(level)) {
        m_level = meta->level;
    }

    std::string levelName;
    getLogLevel(levelName);
    LogInfo("{}({}) => {}", __FUNCTION__, level.c_str(), levelName.c_str());
}

const char *Logger::getLevelName(LogLevel level) {
    if (auto *meta = LogLevelMeta::find(level)) {
        return meta->paddedName;
    }
    return "UNKNOWN";
}

void Logger::switchLogCache(bool flag) {
    if (flag) {
        activeLogger = std::make_shared<CacheLogger>();
    } else if (fileLogger->isOK()) {
        activeLogger = fileLogger;
    } else {
        setToStdout();
    }
}

std::string Logger::getLogCache(bool clear) const {
    std::string output;

    const std::unique_lock<std::timed_mutex> lg(this->m_lock, std::chrono::seconds{10});
    if (lg.owns_lock()) {
        auto cacheLogger = dynamic_pointer_cast<CacheLogger>(activeLogger);
        if (cacheLogger) {
            output = cacheLogger->cache();
            if (clear) {
                cacheLogger->reset();
            }
        }
    }
    return output;
}

void Logger::clearLogCache() {
    const std::unique_lock<std::timed_mutex> lg(this->m_lock, std::chrono::seconds{10});
    if (lg.owns_lock()) {
        auto cacheLogger = dynamic_pointer_cast<CacheLogger>(activeLogger);
        if (cacheLogger) {
            cacheLogger->reset();
        }
    }
}

namespace log_details {
    void PrintLog(Logger *instance, LogLevel level, const char *file,
                  int line, const std::string &msg, const char *pattern) {
        if (instance) {
            try {
                instance->doLoggingStr(level, file, line, msg);
            } catch (const std::exception &ex) {
                const char *s = "unexpected C++ exception: {}, pattern: {}";
                instance->doLoggingStr(LEVEL_ERROR, file, line, ::fmt::format(s, ex.what(), pattern));
#if defined(ENABLE_COVERAGE)
                throw;
#endif
            }
        }
    }
}
