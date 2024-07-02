//
// Created by 韩呈杰 on 2023/11/7.
//
#include <gtest/gtest.h>
#include <thread>
#include "common/SyncQueue.h"
#include "common/ChronoLiteral.h"
#include "common/Logger.h"

using namespace std::chrono;

TEST(CommonSyncQueueTest, PopQueue) {
    std::mutex mutex;
    std::list<int> lst;

    int v;
    EXPECT_FALSE(PopQueue(mutex, lst, v));

    lst.push_back(1);
    v = 0;
    EXPECT_TRUE(PopQueue(mutex, lst, v));
    EXPECT_EQ(1, v);
}

// 非阻塞队列
TEST(CommonSyncQueueTest, NonBlockedSyncQueue01) {
    SyncQueue<int, Nonblock> queue(1);

    queue << 1 << 2;  // 不阻塞，但会抛弃旧的数据
    EXPECT_EQ(size_t(1), queue.Size());

    EXPECT_TRUE(queue.Push(3));
    EXPECT_EQ(size_t(1), queue.Size());
    EXPECT_EQ(size_t(2), queue.DiscardCount());

    int n;
    queue >> n;
    EXPECT_EQ(3, n);
    EXPECT_EQ(size_t(0), queue.Size());

    queue.Close();
    EXPECT_FALSE(queue.Push(4)); // close后的push不计入discardCount
    EXPECT_EQ(size_t(2), queue.DiscardCount());
}

// 非阻塞队列
TEST(CommonSyncQueueTest, NonBlockedSyncQueue02) {
    std::list<int> data;
    SyncQueue<int, Nonblock> queue(data, 1);

    queue << 1 << 2;  // 不阻塞，但会抛弃旧的数据
    EXPECT_EQ(size_t(1), queue.Size());

    EXPECT_TRUE(queue.Push(3));
    EXPECT_EQ(size_t(1), queue.Size());
    EXPECT_EQ(size_t(2), queue.DiscardCount());

    EXPECT_EQ(3, data.front());

    int n;
    queue >> n;
    EXPECT_EQ(3, n);
    EXPECT_EQ(size_t(0), queue.Size());
    EXPECT_TRUE(data.empty());

    queue.Close();
    EXPECT_FALSE(queue.Push(4)); // close后的push不计入discardCount
    EXPECT_EQ(size_t(2), queue.DiscardCount());
}


TEST(CommonSyncQueueTest, PushOnStopped) {
    SyncQueue<int> queue(1);
    queue.Close();
    EXPECT_FALSE(queue.Push(1));
}

TEST(CommonSyncQueueTest, BlockedPush) {
    SyncQueue<int> queue;

    SyncQueue<bool> exited(1);
    std::thread thread{[&]() {
        queue << 1; // 这里会阻塞
        exited << true;
    }};
    EXPECT_FALSE(exited.Wait(milliseconds{50})); // 确保此时线程里的数据仍未推送进来
    EXPECT_TRUE(queue.Wait(milliseconds{500}));  // 取到数据

    bool isExited{ false };
    EXPECT_TRUE(exited.Take(isExited, milliseconds{500}));
    EXPECT_TRUE(isExited);
    thread.join();
}

TEST(CommonSyncQueueTest, BlockedPush2) {
    SyncQueue<int> queue;

    std::thread thread{[&]() {
        EXPECT_FALSE(queue.Push(1)); // 阻塞直到队列关闭
    }};
    std::this_thread::sleep_for(milliseconds{50});

    queue.Close(); // 队列关闭，
    thread.join();
}

TEST(CommonSyncQueueTest, ConditionVariableWaitZero) {
    boost::condition_variable_any cv;
    std::mutex mutex;

    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, boost::chrono::seconds::zero()); // 0秒就是不阻塞
}

TEST(CommonSyncQueueTest, BlockedWithTimeout01) {
    SyncQueue<int> queue(1);
    int v;
    EXPECT_FALSE(queue.TryTake(v));

    EXPECT_TRUE(queue.Push(1, 0_s));
    EXPECT_FALSE(queue.Push(2, 0_s));

    EXPECT_TRUE(queue.TryTake(v));
    EXPECT_EQ(v, 1);
    EXPECT_FALSE(queue.TryTake(v));
}

TEST(CommonSyncQueueTest, BlockedWithTimeout02) {
    SyncQueue<int> queue;
    int v;
    EXPECT_FALSE(queue.TryTake(v));

    SyncQueue<int> chan(10);
    std::thread thread01([&]() {
        chan << ((1 << 4) | (queue.Push(1, 0_s) ? 1 : 0));
    });
    std::thread thread02([&]() {
        chan << ((2 << 4) | (queue.Push(2, 0_s) ? 1 : 0));
    });

    EXPECT_TRUE(chan.Take(v));
    LogInfo("chan -> {:02x}", v); // 失败的
    EXPECT_NE(0, (v & ~0xF));

    EXPECT_FALSE(chan.Wait(200_ms));

    queue.Wait(); // 消费，以使两个线程均终止
    EXPECT_TRUE(chan.Take(v));
    LogInfo("chan -> {:02x}", v);
    EXPECT_NE(1, (v & ~0xF));

    thread01.join();
    thread02.join();
}

TEST(CommonSyncQueueTest, BlockedWithTimeout03) {
    SyncQueue<int> queue;
    int v;
    EXPECT_FALSE(queue.TryTake(v)); // 不阻塞
}
