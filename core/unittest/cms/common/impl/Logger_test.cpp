//
// Created by 韩呈杰 on 2023/6/29.
//
#include <gtest/gtest.h>
#include "common/Logger.h"
#include "common/Chrono.h"
#include "common/UnitTestEnv.h"
#include "common/StringUtils.h"
#include "common/FileUtils.h"
#include <fmt/color.h>
#include <fmt/printf.h>
#include <boost/chrono.hpp>

#ifdef max
#   undef max
#   undef min
#endif

TEST(CommonLoggerTest, LogTrace) {
    LogTrace("hello");
    Log(LEVEL_WARN, "hello");
    LogInfo("bool: {}", true);
    EXPECT_EQ(fmt::format("{}", true), "true");

    LogPrintError("hello");
    LogPrintTrace("hello, %s", std::string{"world"});
    LogPrintDebug("max int64_t: %d", std::numeric_limits<int64_t>::max());
    LogPrintDebug("max int16_t: %d", std::numeric_limits<int16_t>::max());
    LogPrintDebug("max uint64_t: %u", std::numeric_limits<uint64_t>::max()); // 还是区分有符号、无符号的
    LogPrintDebug("max uint16_t: %u", std::numeric_limits<uint16_t>::max());
}

TEST(CommonLoggerTest, FmtTimePoint) {
    auto now = std::chrono::FromMicros(1699886682120836LL);
    std::cout << __LINE__ << ". " << ToMicros(now) << std::endl;

    fmt::print("{}. {}\n", __LINE__, now);
    fmt::print("{}. {:%F %T %Z}\n", __LINE__, now);
    fmt::print("{}. {:%F %T %z}\n", __LINE__, now);
    fmt::print("{}. {:%c}\n", __LINE__, now);
    fmt::print("{}. {:%F %T %Z}\n", __LINE__, fmt::localtime(std::chrono::system_clock::to_time_t(now)));
    fmt::print("{}. {:%F %T %z}\n", __LINE__, fmt::localtime(std::chrono::system_clock::to_time_t(now)));

    namespace bc = boost::chrono;
    // auto bNow = date::convert_time_point_same_clock<std::chrono::system_clock, bc::system_clock>(now);
    auto bNow = now;
    std::cout << __LINE__ << ". " << bNow << std::endl;
#if BOOST_VERSION >= 108100
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::utc, "%F %T %Z") << bNow << std::endl;
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::utc, "%F %T %z") << bNow << std::endl;
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::local, "") << bNow << std::endl;
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::local, "%F %T") << bNow << std::endl;
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::local, "%F %T %Z") << bNow << std::endl;
    std::cout << __LINE__ << ". " << bc::time_fmt(bc::timezone::local, "%F %T %z") << bNow << std::endl;
#endif // #if BOOST_VERSION >= 108100

    std::cout << __LINE__ << ". " << date::format(now, 0) << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, 3) << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, 6) << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, 9) << std::endl;

    std::cout << __LINE__ << ". " << date::format(now, "L") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T %Z") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000 %z") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000000 %Z") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000000000 %z") << std::endl;

    std::cout << __LINE__ << ". " << date::format(now, "%F %T.000") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000000") << std::endl;
    std::cout << __LINE__ << ". " << date::format(now, "L%F %T.000000000") << std::endl;

    LogInfo("{}", now);
}

TEST(CommonLoggerTest, Fmt) {
    std::string s = ::fmt::format("hello {}, number: {}", "world", 32);
    EXPECT_EQ(s, "hello world, number: 32");

    // 彩色控制在windows上是不灵光的，且有乱码嫌疑
    LogError("hello {}", std::string{"world"});
    LogWarn("hello {}, number: {}", "world", 32);
    LogInfo("hello {}, number: {}", "world", fmt::styled(32, fmt::emphasis::bold | fmt::fg(fmt::color::green)));
    LogDebug("hello {}, number: {}",
             fmt::styled("world", fmt::fg(fmt::color::light_green)), fmt::styled(32, fmt::fg(fmt::color::red)));
    int n = 3;
    LogTrace("hello {}, number: {}", "world", fmt::styled(n, fmt::emphasis::underline));
    Log(LEVEL_INFO, "hello {}, number: {}", "world", 32);

    LogInfo("no parameter");
    Log(LEVEL_WARN, "no parameter");

    // fg前景色，bg背景色。
    fmt::print(fmt::emphasis::bold | fg(fmt::color::orange), "Elapsed time: {0:.2f} seconds\n", 1.23);
    fmt::print("Elapsed time: {0:.2f} seconds\n",
               fmt::styled(1.23, fmt::fg(fmt::color::green) | fmt::bg(fmt::color::white)));
}

TEST(CommonLoggerTest, FmtCompileTimeCheck) {
    EXPECT_ANY_THROW(LogWarn("{:d}", "foo"));

    EXPECT_EQ(fmt::format("{:04}", 2), "0002"); // 这个是OK的
    // EXPECT_EQ(fmt::format("{:04}", "2"), "0002"); // 这个FAIL
    EXPECT_EQ(fmt::format("{:>4}", "2"), "   2"); // 这个是OK的
}

TEST(CommonLoggerTest, FmtWithArgsButWithBrace) {
    EXPECT_ANY_THROW(fmt::print("{:d}"));

    // 无参敢应该是安全的
    LogError("{} hello"); // 此处应能正常输出，不出错
}


TEST(CommonLoggerTest, FmtPercentChar) {
    fmt::printf("%d\n", 10);
    EXPECT_EQ(fmt::sprintf("%d", 10), "10");
}

TEST(CommonLoggerTest, StdChrono) {
    std::cout << fmt::format("{}", std::chrono::system_clock::now()) << std::endl;
    std::cout << fmt::format("{}", 238_ms) << std::endl;
    EXPECT_EQ(fmt::format("{}", 238_ms), "238ms");
}

TEST(CommonLoggerTest, Cache02) {
    Logger logger;
    logger.switchLogCache(true);

    std::string line("---------------------");
    LOG_I(&logger, "{}", line);
    std::string output = TrimRightSpace(logger.getLogCache());
    std::cout << "*" << output << std::endl;
    EXPECT_TRUE(HasSuffix(output, line));

    EXPECT_FALSE(logger.getLogCache().empty());
    logger.clearLogCache();
    EXPECT_TRUE(logger.getLogCache().empty());
}

TEST(CommonLoggerTest, Cache01) {
    Logger logger;
    logger.switchLogCache(true);

    std::string line("---------------------");
    LOG_I(&logger, "{}", line);
    std::string output = TrimRightSpace(logger.getLogCache(true));
    std::cout << "*" << output << std::endl;
    EXPECT_TRUE(HasSuffix(output, line));

    EXPECT_TRUE(logger.getLogCache().empty());
}

TEST(CommonLoggerTest, File) {
    fs::path logFile = TEST_SIC_CONF_PATH / "conf" / "logs" / "sic.log";
    if (fs::exists(logFile)) {
        fs::remove(logFile);
    }
    ASSERT_FALSE(fs::exists(logFile));

    Logger logger;
    EXPECT_TRUE(logger.setup(logFile.string()));
    ASSERT_TRUE(fs::exists(logFile));

    LOG(&logger, LEVEL_INFO, "{}[{}]", __FUNCTION__, __LINE__);
    EXPECT_GT(fs::file_size(logFile), strlen(__FUNCTION__));

    std::string content = ReadFileContent(logFile);
    std::cout << "> " << content;
    EXPECT_NE(content.find(__FUNCTION__), std::string::npos);
}

TEST(CommonLoggerTest, Rotate) {
    fs::path logFile = TEST_SIC_CONF_PATH / "conf" / "logs" / "sic-rotate.log";
    if (fs::exists(logFile.parent_path())) {
        fs::remove_all(logFile.parent_path());
    }
    ASSERT_FALSE(fs::exists(logFile));

    Logger logger;
    logger.setFileSize(256);
    EXPECT_TRUE(logger.setup(logFile.string()));
    ASSERT_TRUE(fs::exists(logFile));

    char line[257] = {0};
    memset(line, '-', sizeof(line) - 1);
    LOG(&logger, LEVEL_INFO, line);
    LOG(&logger, LEVEL_INFO, "----------------");
    EXPECT_EQ(logger.getLogFiles().size(), size_t(2));

    Logger stdoutLogger;
    int count = 0;
    for (const auto &it: logger.getLogFiles()) {
        LOG(&stdoutLogger, LEVEL_INFO, "[{}] ctime: {}, mtime: {}, {}", ++count, it.ctime, it.mtime,
            it.file.lexically_relative(TEST_SIC_CONF_PATH).string());
    }
}

TEST(CommonLoggerTest, Rotate2) {
    fs::path logFile = TEST_SIC_CONF_PATH / "conf" / "logs" / "sic-rotate-02.log";
    if (fs::exists(logFile.parent_path())) {
        fs::remove_all(logFile.parent_path());
    }
    ASSERT_FALSE(fs::exists(logFile));

    Logger logger;
    logger.setFileSize(256);
    EXPECT_TRUE(logger.setup(logFile.string()));
    ASSERT_TRUE(fs::exists(logFile));

    char line[257] = {0};
    memset(line, '-', sizeof(line) - 1);
    LOG(&logger, LEVEL_INFO, line);
    LOG(&logger, LEVEL_INFO, line);
    LOG(&logger, LEVEL_INFO, "----------------");
    EXPECT_EQ(logger.getLogFiles().size(), size_t(3));

    Logger stdoutLogger;
    int count = 0;
    for (const auto &it: logger.getLogFiles()) {
        LOG(&stdoutLogger, LEVEL_INFO, "[{}] ctime: {}, mtime: {}, {}", ++count, it.ctime, it.mtime,
            it.file.lexically_relative(TEST_SIC_CONF_PATH).string());
    }
}

TEST(CommonLoggerTest, CountWith2) {
    fs::path logFile = TEST_SIC_CONF_PATH / "conf" / "logs" / "sic-rotate-02.log";
    if (fs::exists(logFile.parent_path())) {
        fs::remove_all(logFile.parent_path());
    }
    ASSERT_FALSE(fs::exists(logFile));

    Logger logger;
    logger.setFileSize(256);
    EXPECT_EQ(size_t(256), logger.getFileSize());
    logger.setCount(2);
    EXPECT_TRUE(logger.setup(logFile.string()));
    ASSERT_TRUE(fs::exists(logFile));

    char line[257] = {0};
    memset(line, '-', sizeof(line) - 1);
    LOG_I(&logger, line);
    LOG_I(&logger, line);
    LOG_I(&logger, "----------------");
    EXPECT_EQ(logger.getLogFiles().size(), size_t(2));

    Logger stdoutLogger;
    int count = 0;
    for (const auto &it: logger.getLogFiles()) {
        LOG(&stdoutLogger, LEVEL_INFO, "[{}] ctime: {}, mtime: {}, {}", ++count, it.ctime, it.mtime,
            it.file.lexically_relative(TEST_SIC_CONF_PATH).string());
    }
}

TEST(CommonLoggerTest, NoFileLine) {
    Logger logger(true);
    EXPECT_TRUE(logger.m_simple);
    logger.switchLogCache(true);
    const std::string line(R"({"key":"hello"})");
    LOG_I(&logger, "{}", line);
    std::string cache = logger.getLogCache();
    std::cout << cache; // cache自带换行
    // 显示完时间后，直接显示数据
    EXPECT_TRUE(HasSuffix(TrimRightSpace(cache), "] " + line));
}

TEST(CommonLoggerTest, SetLogLevel) {
    Logger logger;
    logger.setLogLevel("WARN");

    std::string level;
    EXPECT_EQ(logger.getLogLevel(level), LEVEL_WARN);
    EXPECT_EQ(level, "warn");
}

TEST(CommonLoggerTest, getLevelName) {
    EXPECT_EQ(std::string{"WARN "}, Logger::getLevelName(LEVEL_WARN));
    EXPECT_EQ(std::string{"UNKNOWN"}, Logger::getLevelName(LogLevel(-1)));
}

TEST(CommonLoggerTest, SwitchLogCache) {
    Logger logger;

    logger.switchLogCache(true);
    EXPECT_EQ(logger.activeLoggerName(), "CacheLogger");

    logger.switchLogCache(false);
    EXPECT_EQ(logger.activeLoggerName(), "StdOutLogger");

    fs::path logFile = TEST_SIC_CONF_PATH / "conf" / "logs" / "sic_logger_switchLogCache.log";
    EXPECT_TRUE(logger.setup(logFile.string(), 10 * 1024 * 1024, 2));
    logger.switchLogCache(false);
    EXPECT_EQ(logger.activeLoggerName(), "FileLogger");
}

TEST(CommonLoggerTest, swapLogLevel) {
    Logger logger;

    logger.m_level = LEVEL_TRACE;
    auto old = logger.swapLevel(LEVEL_ERROR);
    EXPECT_EQ(old, LEVEL_TRACE);
    old = logger.swapLevel(LEVEL_INFO);
    EXPECT_EQ(old, LEVEL_ERROR);
}

TEST(CommonLoggerTest, VerifyEnumValue) {
    // 保证LogLevel从0起，且数值上连续
    EXPECT_EQ(LogLevelMeta::find(LogLevel(0))->level, LogLevel(0));
    EXPECT_STREQ(LogLevelMeta::find(LogLevel(0))->paddedName, "0    ");
    EXPECT_EQ(LogLevelMeta::find(LogLevel(0))->nameLower, "0");
    EXPECT_EQ(LogLevelMeta::find(LogLevel(0))->nameUpper, "0");

    EXPECT_EQ(1, (int) LEVEL_ERROR);
    EXPECT_EQ(LogLevelMeta::find(LEVEL_ERROR)->level, LEVEL_ERROR);
    EXPECT_STREQ(LogLevelMeta::find(LEVEL_ERROR)->paddedName, "ERROR");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_ERROR)->nameLower, "error");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_ERROR)->nameUpper, "ERROR");
    EXPECT_EQ(LogLevelMeta::find("ERROR"), LogLevelMeta::find(LEVEL_ERROR));

    EXPECT_EQ(2, (int) LEVEL_WARN);
    EXPECT_EQ(LogLevelMeta::find(LEVEL_WARN)->level, LEVEL_WARN);
    EXPECT_STREQ(LogLevelMeta::find(LEVEL_WARN)->paddedName, "WARN ");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_WARN)->nameLower, "warn");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_WARN)->nameUpper, "WARN");
    EXPECT_EQ(LogLevelMeta::find("warn"), LogLevelMeta::find(LEVEL_WARN));

    EXPECT_EQ(3, (int) LEVEL_INFO);
    EXPECT_EQ(LogLevelMeta::find(LEVEL_INFO)->level, LEVEL_INFO);
    EXPECT_STREQ(LogLevelMeta::find(LEVEL_INFO)->paddedName, "INFO ");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_INFO)->nameLower, "info");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_INFO)->nameUpper, "INFO");
    EXPECT_EQ(LogLevelMeta::find("INFO"), LogLevelMeta::find(LEVEL_INFO));

    EXPECT_EQ(4, (int) LEVEL_DEBUG);
    EXPECT_EQ(LogLevelMeta::find(LEVEL_DEBUG)->level, LEVEL_DEBUG);
    EXPECT_STREQ(LogLevelMeta::find(LEVEL_DEBUG)->paddedName, "DEBUG");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_DEBUG)->nameLower, "debug");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_DEBUG)->nameUpper, "DEBUG");
    EXPECT_EQ(LogLevelMeta::find("DEBUG"), LogLevelMeta::find(LEVEL_DEBUG));

    EXPECT_EQ(5, (int) LEVEL_TRACE);
    EXPECT_EQ(LogLevelMeta::find(LEVEL_TRACE)->level, LEVEL_TRACE);
    EXPECT_STREQ(LogLevelMeta::find(LEVEL_TRACE)->paddedName, "TRACE");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_TRACE)->nameLower, "trace");
    EXPECT_EQ(LogLevelMeta::find(LEVEL_TRACE)->nameUpper, "TRACE");
    EXPECT_EQ(LogLevelMeta::find("TRACE"), LogLevelMeta::find(LEVEL_TRACE));

    EXPECT_EQ(nullptr, LogLevelMeta::find("Critical"));
}