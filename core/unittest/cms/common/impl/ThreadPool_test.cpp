//
// Created by 韩呈杰 on 2023/2/21.
//
#include <gtest/gtest.h>
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif

#include "common/SyncQueue.h"
#include "common/ThreadUtils.h"

#include <thread>

TEST(Common_ThreadPoolTest, Option) {
    auto op = ThreadPool::Option{}.min(0).max(1).maxIdle(std::chrono::milliseconds{100}).capacity(1).name("hello");
    EXPECT_EQ(op.poolName, "hello");
}

TEST(Common_ThreadPoolTest, Pool) {
    ThreadPool threadPool(ThreadPool::Option{}.min(1).max(0));
    EXPECT_EQ(size_t(1), threadPool.threadCount());
    EXPECT_EQ(threadPool.minThreadCount(), threadPool.maxThreadCount());
    EXPECT_EQ(size_t(0), threadPool.taskCount());

    std::mutex mutex;
    mutex.lock();
    std::atomic<int> count{0};
    threadPool.commit({}, [&]() {
        EXPECT_EQ(size_t(0), threadPool.taskCount());
        ++count;
        std::lock_guard<std::mutex> _guard(mutex);
    });
    EXPECT_EQ(size_t(1), threadPool.threadCount());
    mutex.unlock();
}

TEST(Common_ThreadPoolTest, AutoScale) {
    ThreadPool threadPool(ThreadPool::Option{}.min(1).max(3).maxIdle(std::chrono::milliseconds{50}));
    EXPECT_EQ(size_t(1), threadPool.minThreadCount());
    EXPECT_EQ(size_t(3), threadPool.maxThreadCount());
    EXPECT_EQ(size_t(0), threadPool.taskCount());

    SyncQueue<bool> queueExit;
    SyncQueue<std::thread::id> queue, exited(3);
    for (size_t i = 0; i < threadPool.maxThreadCount(); i++) {
        threadPool.commit("", [&]() {
            queue << std::this_thread::get_id();
            queueExit.Wait();
            exited << std::this_thread::get_id();
        });
    }

    for (size_t i = 0; i < threadPool.maxThreadCount(); i++) {
        queue.Wait();
    }
    EXPECT_EQ(threadPool.maxThreadCount(), threadPool.threadCount());
    EXPECT_EQ(threadPool.maxThreadCount(), threadPool._pool.size());

    queueExit << true; // 退出一个线程
    exited.Wait();

    using namespace std::chrono;

    {
        steady_clock::time_point maxWait = steady_clock::now() + seconds{1};
        while (steady_clock::now() < maxWait && threadPool.maxThreadCount() == threadPool.threadCount()) {
            std::this_thread::sleep_for(milliseconds{50});
        }
        EXPECT_EQ(threadPool.maxThreadCount() - 1, threadPool._pool.size());
        EXPECT_EQ(threadPool.maxThreadCount() - 1, threadPool.threadCount());
    }

    queueExit.Close(); // 释放剩余线程

    {
        steady_clock::time_point maxWait = steady_clock::now() + seconds{1};
        while (steady_clock::now() < maxWait && threadPool.threadCount() > threadPool.minThreadCount()) {
            std::this_thread::sleep_for(milliseconds{50});
        }
        EXPECT_EQ(threadPool.minThreadCount(), threadPool.threadCount());
    }

    threadPool.stop();
    threadPool.join();
    EXPECT_EQ(size_t(0), threadPool.threadCount());
}

TEST(Common_ThreadPoolTest, CommitOnClosedThread) {
    ThreadPool threadPool(ThreadPool::Option{}.min(1));
    threadPool.stop();
    EXPECT_FALSE(threadPool.commit({}, []() {}).valid());
}

TEST(Common_ThreadPoolTest, addThreadOnClosedThread) {
    bool caught = false;
    try {
        ThreadPool threadPool(ThreadPool::Option{}.min(1));
        threadPool.stop();
        threadPool.addThread(1);
    } catch (const std::exception &ex) {
        caught = true;
        EXPECT_NE(std::string(ex.what()).find("stopped"), std::string::npos);
    }
    EXPECT_TRUE(caught);
}

TEST(Common_ThreadPoolTest, threadName) {
    ThreadPool threadPool(ThreadPool::Option{}.min(1));
    SyncQueue<std::string> queue(1);
    threadPool.commit("", [&]() {
        queue << GetThreadName(ThreadPool::CurrentThreadId());
    });

    std::string name;
    queue >> name;
    EXPECT_EQ(name, "ThreadPool::worker");
}


TEST(Common_ThreadPoolTest, Future) {
    ThreadPool threadPool(ThreadPool::Option{}.min(1));
    auto future = threadPool.commitTimeout("", std::chrono::seconds{1}, [&]() {
        return 2;
    });
    EXPECT_TRUE(future.valid());
    EXPECT_EQ(future.get(), 2);
}

TEST(Common_ThreadPoolTest, Future2) {
    std::atomic<int32_t> count(0);
    {
        ThreadPool threadPool(ThreadPool::Option{}.min(1).capacity(1));
        SyncQueue<bool> exit;
        // 占住线程
        threadPool.commitTimeout("", std::chrono::seconds{1}, [&]() {
            exit.Wait();
            count++;
        });

        // 占住队列
        auto future = threadPool.commitTimeout("", std::chrono::seconds{1}, [&]() {
            count++;
            return 0;
        });
        EXPECT_TRUE(future.valid());

        // 队列满，commit失败
        auto future2 = threadPool.commitTimeout("", std::chrono::milliseconds{50}, [&]() {
            count++;
            return 0;
        });
        EXPECT_FALSE(future2.valid());

        exit << true;
    }
    EXPECT_EQ(count.load(), int32_t(2));
}
