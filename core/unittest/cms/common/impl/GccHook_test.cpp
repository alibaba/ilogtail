//
// Created by 韩呈杰 on 2023/4/5.
//
#if defined(__GNUC__) // && defined(__linux__)

#include <gtest/gtest.h>

#include "common/GccHook.h"

#include <exception>
#include <thread>

#include "common/ExceptionsHandler.h"
#include "common/Logger.h"
#include "common/ScopeGuard.h"
#include "common/StringUtils.h"

using namespace common;

TEST(CommonGccHookTest, GccThrow) {
    safeRun([]() {
        throw std::runtime_error("test");
    });
}

TEST(CommonGccHookTest, GccThrowInThread) {
    std::string logString;
    {
        SingletonLogger::Instance()->switchLogCache(true);
        defer(SingletonLogger::Instance()->switchLogCache(false));
        std::thread thread([]() {
            safeRun([]() {
                throw std::runtime_error("test");
            });
        });
        thread.join();
        logString = SingletonLogger::Instance()->getLogCache();
    }
    std::cout << logString;
    EXPECT_NE(std::string::npos, logString.find("runtime_error: test"));
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(WITHOUT_MINI_DUMP)
    const char flag[] = "dump to: ";
    size_t pos = logString.find(flag);
    ASSERT_NE(pos, std::string::npos);
    pos += sizeof(flag) - 1;
    size_t posEnd = logString.find('\n', pos);
    std::string dumpFile = TrimSpace(logString.substr(pos, posEnd - pos));
    LogDebug("dump file: {}", dumpFile);
    EXPECT_TRUE(fs::exists(dumpFile));
    fs::remove(dumpFile);
    EXPECT_FALSE(fs::exists(dumpFile));
#endif // !macOS
}

TEST(CommonGccHookTest, SafeRunDividedByZero) {
    std::cout << "这个会触发SIGSEGV(Segment Fault)，从而导致程序abort，不能作为常规则单测" << std::endl;
    // argus::safeRun([](){
    //     volatile int b = 0;
    //     int n = 5 / b;
    // });
}

void func1();
void func2();

TEST(CommonGccHookTest, tagFuncPtr) {
    EXPECT_LT((void *) func1, (void *) func2);

    std::set<tagFuncPtr> mySet{tagFuncPtr{(void *) func1, (void *) func2}};
    EXPECT_EQ(size_t(1), mySet.size());
    EXPECT_NE(mySet.end(), mySet.find(tagFuncPtr{(void *) func1}));
    EXPECT_NE(mySet.end(), mySet.find(tagFuncPtr{(char *) func1 + 1}));
    EXPECT_EQ(mySet.end(), mySet.find(tagFuncPtr{(void *) func2}));
}

void func1() {
    printf("hello, world: func1\n");
}

void func2() {
    printf("hello, world: func2\n");
}

#endif
