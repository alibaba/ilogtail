//
// Created by 韩呈杰 on 2023/4/13.
//
#include <gtest/gtest.h>
#include "common/ThreadWorker.h"

#include <thread>
#include <chrono>
#include <boost/lockfree/queue.hpp>

#include "common/SyncQueue.h"

using namespace std::chrono;

class StubThreadWorker: public common::ThreadWorker {
public:
    boost::lockfree::queue<int> queue{2};
    void doRun() override {
        int count = 0;
        steady_clock::time_point expire = steady_clock::now() + seconds{15};
        while(!isThreadEnd()) {
            count++;
            while(!queue.push(count) && steady_clock::now() < expire){}
        }
    }
};

TEST(ThreadWorkerTest, RunOK) {
    StubThreadWorker worker;
    EXPECT_TRUE(worker.runIt());
    int count;
    steady_clock::time_point expire = steady_clock::now() + seconds{15};
    while(!worker.queue.pop(count)){
        EXPECT_LT(steady_clock::now(), expire);
        if (steady_clock::now() >= expire) {
            break;
        }
    }
    EXPECT_EQ(count, 1);
    worker.endThread();
    worker.join();
}

TEST(ThreadWorkerTest, ThreadWorkerEx) {
    class ThreadWorkerExSub: public ThreadWorkerEx{};

    ThreadWorkerExSub threadWorkerEx;
    EXPECT_FALSE(threadWorkerEx.IsRunning());

    SyncQueue<int> queue(1);
    EXPECT_TRUE(threadWorkerEx.runAsync([&](){
        queue << 1;
        return std::chrono::milliseconds{5 * 1000};
    }));
    // 失败
    EXPECT_FALSE(threadWorkerEx.runAsync([&](){
        queue << 2;
        return std::chrono::milliseconds{5 * 1000};
    }));
    int queueId;
    queue >> queueId;
    EXPECT_EQ(1, queueId);
    EXPECT_TRUE(threadWorkerEx.IsRunning());

    // 确保执行时间超1ms，便于观察defer日志是否正确。
    std::this_thread::sleep_for(std::chrono::milliseconds{2});

    threadWorkerEx.stopAsync();
    EXPECT_FALSE(threadWorkerEx.IsRunning());
}

TEST(ThreadWorkerTest, WorkerKeepAliveLog) {
    {
        WorkerKeepAliveLog tmp("test-00");
        EXPECT_FALSE(tmp.KeepAlive());
    }
    {
        WorkerKeepAliveLog ka("test-01", std::chrono::milliseconds{1});
        std::this_thread::sleep_for(std::chrono::microseconds{1100}); // 1.1 ms
        EXPECT_TRUE(ka.KeepAlive());
        EXPECT_EQ(size_t(1), ka.Count());
    }
}
