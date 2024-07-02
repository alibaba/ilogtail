#ifdef _MSC_VER
#pragma warning(disable: 4996)  // close deprecated
#endif

#include <gtest/gtest.h>
#include "core/self_monitor.h"

#include <thread>
#include <atomic>

#include "common/Logger.h"
#include "common/TimeProfile.h"
#include "common/NetWorker.h"
#include "common/ConsumeCpu.h"
#include "common/SyncQueue.h"
#include "common/Common.h"
#include "common/Chrono.h"
#include "sic/system_information_collector.h"

using namespace common;
using namespace argus;
using namespace std;

class SelfMonitorTest : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

static void cpuThread(std::mutex &mutexExit) {
    while (!mutexExit.try_lock()) {
        consumeCpu();
    }
    mutexExit.unlock();
    LogDebug("cpuThread exited");
}

static void memoryThread(std::mutex &exit) {
    int memSize = 1024 * 1024 * 100;
    int *p = new int[memSize];
    for (int i = 0; i < memSize; i++) {
        p[i] = i;
    }
    {
        std::lock_guard<std::mutex> _guard(exit);  // 当可以lock时，表明该线程需要结束了
    }
    delete[]p;
    LogDebug("memoryThread exited");
}

TEST_F(SelfMonitorTest, TestCpuUsage) {
    std::mutex mutex;
    mutex.lock();
    std::thread th(cpuThread, std::ref(mutex));
    // th.detach();
    SelfMonitor pMonitor(false);
    pMonitor.mInterval = std::chrono::seconds{1};
    pMonitor.mLimit.cpuPercent = 0.1;

    EXPECT_FALSE(pMonitor.isThreadEnd());
    std::atomic<int> exitCode{0};
    pMonitor.doRun([&](int code) { exitCode = code; });
    EXPECT_EQ(exitCode, EXIT_FAILURE);

    mutex.unlock();
    th.join();
}

TEST_F(SelfMonitorTest, TestMemeryUsage) {
    SelfMonitor pMonitor(false);
    pMonitor.mInterval = std::chrono::milliseconds{250};
    pMonitor.mLimit.rssMemory = 100 * 1024 * 1024;
    std::mutex mutexExit;
    mutexExit.lock();
    std::thread th(memoryThread, std::ref(mutexExit));

    std::atomic<int> exitCode{0};
    pMonitor.doRun([&](int code) { exitCode = code; });
    EXPECT_EQ(exitCode, EXIT_FAILURE);

    mutexExit.unlock();
    th.join();
}

static void fdThread(std::mutex &mutex, SyncQueue<bool> &ready) {
	ready << true;
    {
        // 保持10个打开的文件
        std::vector<std::shared_ptr<NetWorker>> fd(10);
        for (size_t i = 0; i < fd.size(); i++) {
            fd[i] = std::make_shared<NetWorker>(fmt::format("{}.{}", __FUNCTION__, i));
            fd[i]->createTcpSocket();
        }
        mutex.lock();
        mutex.unlock();
    }
	LogDebug("fdThread exited");
}

// 苹果下不支持进程获取打开的文件数
#if !defined(__APPLE__) && !defined(__FreeBSD__)
TEST_F(SelfMonitorTest, TestFdNumber) {
    SelfMonitor pMonitor(false);

    ProcessStat processStat;
    SelfMonitor::GetProcessStat(GetPid(), processStat);
    LogDebug("process({}) openFiles: {}", GetPid(), processStat.fdNum);

    pMonitor.mInterval = std::chrono::milliseconds{250};
    pMonitor.mLimit.fdCount = 10;

    SyncQueue<bool> ready{1};
    std::mutex mutex;
    mutex.lock();
    std::thread th(fdThread, std::ref(mutex), std::ref(ready));
    ready.Wait();// 等待read

    // EXPECT_EXIT起来时会有一个异常捕获句柄。
    std::atomic<int> exitCode{0};
    pMonitor.doRun([&](int code) { exitCode = code; });
    EXPECT_EQ(exitCode, EXIT_FAILURE);
    // EXPECT_EXIT(pMonitor.doRun(), ::testing::ExitedWithCode(EXIT_FAILURE), "");

    mutex.unlock(); // 线程退出
    th.join();
}
#endif

TEST_F(SelfMonitorTest, GetAllAgentStatusMetrics) {
    SelfMonitor pMonitor;
    std::vector<CommonMetric> metrics;
    pMonitor.GetAllAgentStatusMetrics(metrics);
    for (auto &metric: metrics) {
        cout << metric.name << " {";
        for (auto &itTag: metric.tagMap) {
            cout << itTag.first << "=" << itTag.second << " ";
        }
        cout << "} " << metric.value << " " << metric.timestamp << endl;
    }
}

TEST_F(SelfMonitorTest, GetProcessStat) {
    SelfMonitor pMonitor;
    pid_t pid = GetPid();

    ProcessStat stat;
    int64_t start = NowMillis();
    EXPECT_TRUE(pMonitor.GetProcessStat(pid, stat));
    auto cpu1 = stat.total;

    consumeCpu();

    // std::cout << ReadFileContent(file) << std::endl;
    int64_t end = NowMillis();
    EXPECT_TRUE(pMonitor.GetProcessStat(pid, stat));
    auto cpu2 = stat.total;

    LogDebug("cpu1: {}, cpu2: {}", cpu1.count(), cpu2.count());
    cout << "cpu2 - cpu1 => " << (cpu2 - cpu1).count() << endl;
    EXPECT_GT(cpu2, cpu1);

    double cpuUsage = static_cast<double>((cpu2 - cpu1).count()) * 100.0 / static_cast<double>(end - start);
    cout << "cpuUsage=" << cpuUsage << "% (single cpu core)" << endl;
    EXPECT_GT(cpuUsage, 50.0);

    cout << "memUsage=" << stat.rss / (1024.0 * 1024.0) << " MB" << endl;
    EXPECT_GT(stat.rss, decltype(stat.rss)(1));

    cout << "openFile=" << stat.fdNum << endl;
#if !defined(__APPLE__) && !defined(__FreeBSD__)
    EXPECT_GT(stat.fdNum, 0);
#endif
}
