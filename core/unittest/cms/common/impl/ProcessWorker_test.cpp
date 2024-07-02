//
// Created by 韩呈杰 on 2024/1/11.
//
#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <gtest/gtest.h>
#include "common/ProcessWorker.h"
#include <apr-1/apr_thread_proc.h>  // apr_proc_t

#if !defined(DISABLE_SIC_IN_COMMON) && (defined(__linux__) || defined(__unix__))
#   define TEST_ALL
#endif
#ifdef TEST_ALL
#include "sic/system_information_collector_util.h"  // GetExecDir
#include "common/Uuid.h"
#endif
#include "common/Logger.h"
#include "common/FilePathUtils.h"

#ifdef max
#   undef max
#endif

using namespace common;

extern const size_t SRC_ROOT_OFFSET;

bool IsInDocker() {
    return fs::exists("/.dockerenv");
}
#ifdef TEST_ALL
static std::string dmidecodePath() {
    if (IsInDocker()) {
        return {}; // docker里拿不到sn，跳过
    }
    auto dmiRoot = fs::path(std::string(__FILE__, SRC_ROOT_OFFSET)) / "cmd" / "dmidecode";
    EXPECT_TRUE(fs::exists(dmiRoot));

#if defined(__i386__) || defined(__i386)
#   define SUFFIX "_x32"
#elif defined(__x86_64__) || defined(__x86_64) || defined(__amd64__) || defined(__AMD64__)
#   define SUFFIX "_x64"
#elif defined(__aarch64__) || defined(aarch64)
#   define SUFFIX "_arm64"
#endif

    std::string dmidecode = (dmiRoot / "dmidecode" SUFFIX).string();
    LogInfo("{}", dmidecode);
    return dmidecode;
}
#endif

TEST(CommonProcessWorkerTest, create) {
#ifdef WIN32
    {
        ProcessWorker pw;
        EXPECT_EQ(EPERM, pw.create("x.sh", nullptr, nullptr, nullptr, nullptr, nullptr));
    }
#endif
    ProcessWorker pw;
    EXPECT_EQ(pw.exitWhy(), pw.m_exitwhy);
    EXPECT_EQ(pw.getExitCode(), pw.m_exitcode);
    EXPECT_EQ(pw.getPid(), 0); //pw.m_proc->pid);

#ifdef TEST_ALL
    std::string dmidecode = dmidecodePath();
    if (dmidecode.empty()) {
        return ;
    }
    const char *args[] = {dmidecode.c_str(), "-s", "system-serial-number", nullptr}; // 必须以nullptr结尾
    EXPECT_EQ(APR_SUCCESS, pw.create(args[0], args, nullptr, nullptr, nullptr, GetExecDir().c_str()));
    ASSERT_TRUE(pw.wait(ProcessWorker::WAIT));
    EXPECT_TRUE((bool)pw.m_proc);

    std::vector<char> buf(8192);
    size_t size = pw.output(&buf[0], buf.size());
    EXPECT_GT(size, size_t(0));
    LogInfo("system-serial-number: {}", std::string(&buf[0], size));

    EXPECT_EQ(size_t(0), pw.errput(&buf[0], buf.size()));
#endif
}

#ifdef TEST_ALL
TEST(CommonProcessWorkerTest, create2) {
    ProcessWorker pw;

    std::string dmidecode = dmidecodePath();
    if (dmidecode.empty()) {
        return ;
    }

    const char *args[] = {dmidecode.c_str(), "-s", "system-serial-number", nullptr}; // 必须以nullptr结尾
    EXPECT_EQ(0, pw.create2(args[0], args, "admin", 0, true));
    ASSERT_TRUE(pw.wait(ProcessWorker::WAIT));
}

TEST(CommonProcessWorkerTest, create2InvalidUser) {
    ProcessWorker pw;

    std::string dmidecode = dmidecodePath();
    if (dmidecode.empty()) {
        return ;
    }

    const char *args[] = {dmidecode.c_str(), "-s", "system-serial-number", nullptr}; // 必须以nullptr结尾
    EXPECT_EQ(ENOENT, pw.create2(args[0], args, NewUuid().c_str(), 0, true));
}

TEST(CommonProcessWorkerTest, create3) {
    ProcessWorker pw;

    std::string dmidecode = dmidecodePath();
    if (dmidecode.empty()) {
        return ;
    }
    const char *args[] = {dmidecode.c_str(), "-s", "system-serial-number", nullptr}; // 必须以nullptr结尾
    EXPECT_EQ(0, pw.create3(args[0], args, nullptr, nullptr, GetExecDir().c_str(), false));
    ASSERT_TRUE(pw.wait(ProcessWorker::WAIT));
}

TEST(CommonProcessWorkerTest, create3WithUser) {
    ProcessWorker pw;

    std::string dmidecode = dmidecodePath();
    if (dmidecode.empty()) {
        return ;
    }
    const char *args[] = {dmidecode.c_str(), "-s", "system-serial-number", nullptr}; // 必须以nullptr结尾
    EXPECT_EQ(0, pw.create3(args[0], args, "admin", "admin", GetExecDir().c_str(), false));
    ASSERT_TRUE(pw.wait(ProcessWorker::WAIT));
}
#endif

TEST(CommonProcessWorkerTest, enumChildProcess) {
    if (IsInDocker()) {
        return; // docker里该用例不成立
    }
    std::set<pid_t> pidSet = ProcessWorker::enumChildProcess(1);
    LogDebug("process(1)'s children count: {}", pidSet.size());
#if defined(__linux__) || defined(__unix__) || defined(SOLARIS)
    EXPECT_GT(pidSet.size(), size_t(1));
#else
    EXPECT_TRUE(pidSet.empty());
#endif
}

#ifdef TEST_ALL
TEST(CommonProcessWorkerTest, kill) {
    ProcessWorker pw;
    pw.m_proc = std::make_shared<apr_proc_t>();
    pw.m_proc->pid = 1;
    EXPECT_FALSE(pw.kill(SIGKILL));

    pw.m_proc->pid = std::numeric_limits<decltype(pw.m_proc->pid)>::max();
    EXPECT_FALSE(pw.kill(SIGKILL));
}
#endif
