//
// Created by 韩呈杰 on 2023/4/3.
//
#include <gtest/gtest.h>
#include "common/ExceptionsHandler.h"

#include <csignal>
#include <thread>

#include "common/Logger.h"
#include "common/JsonValueEx.h"
#include "common/ScopeGuard.h"
#include "common/BoostStacktraceMt.h"
#include "common/ThrowWithTrace.h"

using namespace common;

void foo(int i) {
    if (i >= 4) {
        ThrowWithTrace(std::out_of_range("'i' must be less than 4 in foo()"));
    } else if (i <= 0) {
        throw std::logic_error("'i' must be greater than zero in foo()");
    }
}

TEST(ExceptionStackTraceTest, ThrowWithTrace1) {
    bool caught = false;
    try {
        foo(5); // testing assert handler
    } catch (const std::exception &e) {
        caught = true;
        std::cerr << e.what() << std::endl;
        const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(e);
        ASSERT_NE(st, nullptr);
        if (st) {
            std::cerr << boost::stacktrace::mt::to_string(*st) << std::endl;
        }
    }
    EXPECT_TRUE(caught);
}

TEST(ExceptionStackTraceTest, ThrowWithTrace2) {
    bool caught = false;
    try {
        foo(-1); // testing assert handler
    } catch (const std::exception &e) {
        caught = true;
        std::cerr << e.what() << std::endl;
        const boost::stacktrace::stacktrace *st = boost::get_error_info<traced>(e);
        ASSERT_EQ(st, nullptr);
        if (st) {
            std::cerr << boost::stacktrace::mt::to_string(*st) << std::endl;
        }
    }
    EXPECT_TRUE(caught);
}

TEST(ExceptionStackTraceTest, PrintStackTrace) {
    printStacktrace(SIGINT, "ExceptionStackTraceTest_PrintStackTrace");
#ifndef WIN32
    // Win32下不支持自定义的signal
    LogInfo("+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-");
    printStacktrace(SIGUSR2, "ExceptionStackTraceTest_PrintStackTrace");
#endif
}

TEST(ExceptionStackTraceTest, SignalPrompt) {
    {
        std::string prompt = signalPrompt(SIGINT);
        EXPECT_NE(prompt.find("SIGINT"), std::string::npos);
    }
#ifndef WIN32
    {
        std::string prompt = signalPrompt(SIGUSR1);
        EXPECT_NE(prompt.find("unknown"), std::string::npos);
    }
#endif
}

#ifndef WIN32
TEST(ExceptionStackTraceTest, safeRun) {
    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    safeRun([](){
        throw std::out_of_range("ByDesign");
    });
    std::string cache = logger->getLogCache();
    std::cout << cache;
    EXPECT_NE(std::string::npos, cache.find("out_of_range"));
}
#endif

#if defined(__linux__) || defined(__unix__) || defined(WIN32)
TEST(ExceptionStackTraceTest, Dump) {
    DumpCache dumpClear(0);

    std::thread thread(safeRun, [&](){
        SetupThread("DumpTest");
        ThrowWithTrace(std::out_of_range("ByDesign"));
    });
    thread.join(); // 等待线程退出

#ifndef WITHOUT_MINI_DUMP
    // 确保确实产生了dump文件
    DumpCache dumpCache(2);
    EXPECT_TRUE(dumpCache.PrintLastCoreDown());

    using namespace std::chrono;
    system_clock::time_point now = system_clock::now();
    std::string key1 = date::format(now, "L%Y%m%d-%H%M");
    std::cout << "TARGET: " << dumpCache.DumpFiles()[0] << std::endl;
    std::cout << "EXPECT KEY1: " << key1 << std::endl;

    bool ok = (dumpCache.DumpFiles()[0].string().find(key1) != std::string::npos);
    if (!ok) {
        // 防止测试过程中出现跨分钟的情况
        now -= seconds{60};
        std::string key2 = date::format(now, "L%Y%m%d-%H%M");
        std::cout << "EXPECT KEY2: " << key2 << std::endl;
        ok = (dumpCache.DumpFiles()[0].string().find(key2) != std::string::npos);
    }
    EXPECT_TRUE(ok);

    auto lastDump = dumpCache.GetLastCoreDown();
    EXPECT_NE(lastDump.filename, "");
    EXPECT_FALSE(lastDump.callstack.empty());
    std::cout << boost::json::serialize(boost::json::value_from(lastDump.toJson())) << std::endl;
#endif // WITHOUT_MINI_DUMP
}

TEST(ExceptionStackTraceTest, tagCoreDumpFileToJson) {
    {
        tagCoreDumpFile coreDumpFile;
        coreDumpFile.filename = "/tmp/argusagent.2022-33.SIGTERM.dmp";

        EXPECT_NE(coreDumpFile.filename, "");
        EXPECT_TRUE(coreDumpFile.callstack.empty());

        boost::json::object obj = coreDumpFile.toJson();
        std::cout << boost::json::serialize(boost::json::value_from(obj)) << std::endl;

        json::Object jsonObject{&obj};
        EXPECT_EQ(jsonObject.getString("signal"), "SIGTERM");
    }
    {
        tagCoreDumpFile coreDumpFile;
        coreDumpFile.filename = "/tmp/argusagent.2022-33.dmp";

        EXPECT_NE(coreDumpFile.filename, "");
        EXPECT_TRUE(coreDumpFile.callstack.empty());

        boost::json::object obj = coreDumpFile.toJson();
        std::cout << boost::json::serialize(boost::json::value_from(obj)) << std::endl;

        json::Object jsonObject{&obj};
        EXPECT_FALSE(jsonObject.contains("signal"));
    }
}

#endif // defined(__linux__) || defined(__unix__) || defined(WIN32)
