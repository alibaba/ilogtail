//
// Created by 韩呈杰 on 2023/7/11.
//
#ifdef WIN32
#include <WinSock2.h>
#endif

#include <gtest/gtest.h>
#include <exception>

#include "common/CppTimer.h"
#include "common/SyncQueue.h"
#include "common/Logger.h"
#include "common/ThrowWithTrace.h"
#include "common/Chrono.h"

TEST(CommonCppTimerTest, ThrowException) {
    CppTime::Timer timer;

    auto timerId = timer.add(0, [&](CppTime::timer_id) {
        ThrowWithTrace<std::runtime_error>("ByDesign");
    }, 0);
    EXPECT_GT(timerId, CppTime::timer_id{0});
    EXPECT_EQ(timer.events.size(), size_t(1));

    // 异常抛出后，timer仍能正常调度
    SyncQueue<int> queue(1);
    timer.add(0, [&](CppTime::timer_id) {
        queue << 1;
    }, 0);
    queue.Wait();
}

TEST(CommonCppTimerTest, Add) {
    CppTime::Timer timer;

    SyncQueue<int> queue{100};
    int callCount = 0;
    auto timerId = timer.add(CppTime::duration::zero(), [&](CppTime::timer_id) {
        callCount++;
        LogInfo("count: {}", callCount);
        queue << callCount;
    }, std::chrono::milliseconds{1});
    EXPECT_GT(timerId, CppTime::timer_id{0});
    EXPECT_EQ(timer.events.size(), size_t(1));

    EXPECT_TRUE(queue.Wait(std::chrono::milliseconds{2})); // 最多等两毫秒
    timer.remove(timerId);
}

TEST(CommonCppTimerTest, LoopCount) {
    CppTime::Timer timer;

    unsigned int loopCount = 0;
    for (int i = 0; i < 10 && loopCount == 0; i++) {
        loopCount = timer.loopCount;
        if (loopCount == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }
    EXPECT_GT(loopCount, decltype(loopCount){0});

    CppTime::timer_id timerId = timer.add(CppTime::duration::zero(), [](CppTime::timer_id) {}, std::chrono::minutes{1});
    EXPECT_GT((int) timerId, 0);
}

TEST(CommonCppTimerTest, TimerEvent) {
    CppTime::detail::Time_event te;

    auto now = CppTime::clock::now();
    te.next = CppTime::timestamp{std::chrono::duration_cast<CppTime::duration>(now.time_since_epoch())};
    // 确保now只比te.next大微秒级别
    int tryCount = 0;
    while (te.next == now) {
        tryCount++;
        std::this_thread::sleep_for(std::chrono::microseconds{1});
        now = CppTime::clock::now();
        te.next = CppTime::timestamp{std::chrono::duration_cast<CppTime::duration>(now.time_since_epoch())};
    }
    if (tryCount > 0) {
        LogInfo("tryCount: {}", tryCount);
    }
    EXPECT_LT(te.next, now);
    EXPECT_EQ(te.next, CppTime::timestamp{std::chrono::duration_cast<CppTime::duration>(now.time_since_epoch())});

    te.updateNextTime(now, std::chrono::seconds{15});
    EXPECT_GT(te.next, now);
}

TEST(CommonCppTimerTest, TimerEventUpdateNextTime) {
    CppTime::detail::Time_event te;
    auto now = CppTime::clock::now();
    te.next = now - 59_min;
    te.updateNextTime(now, 5_min);
    EXPECT_GT(te.next, now);

    te.next = now - 30_s;
    te.updateNextTime(now, 1_min);
    EXPECT_GT(te.next, now);

    te.next = now - 1_min;
    te.updateNextTime(now, 1_min);
    EXPECT_EQ(te.next, now);
}

TEST(CommonCppTimerTest, SharedPtrEvent) {
    CppTime::Timer timer;

    std::atomic<int> counter{0};
    auto data = std::make_shared<int>(1);
    SyncQueue<decltype(data.use_count())> queue;
    timer.add(0_s, [&, data, queue](CppTime::timer_id) mutable {
        int c = counter++;
        std::cout << "[" << c << "] use_count: " << data.use_count() << std::endl;
        queue << data.use_count();
    }, 10_ms);

    decltype(data.use_count()) refCount = 0;
    queue >> refCount;
    EXPECT_EQ(decltype(data.use_count())(2), refCount);  // data:1, timer:2

    data.reset();
    queue >> refCount; // 避开干扰

    queue >> refCount;
    EXPECT_EQ(decltype(data.use_count())(1), refCount);
}