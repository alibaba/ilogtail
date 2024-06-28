//
// Created by 韩呈杰 on 2024/2/26.
//
#include <gtest/gtest.h>
#include "core/SchedulerT.h"
#include "common/Chrono.h"

using namespace argus;

struct SchedulerStateStub : SchedulerState {
    std::string name;

    SchedulerStateStub(const std::string &s = {}) : name(s) {}

    std::string getName() const override {
        return name;
    }

    std::chrono::milliseconds interval() const override {
        return 15_s;
    }
};

TEST(CoreSchedulerT, deadlock) {
    struct SchedulerStub : SchedulerT<SchedulerStateStub, SchedulerStateStub> {
        SchedulerStub() {
            setThreadPool(ThreadPool::Option().min(1).max(1).makePool());
        }

        std::shared_ptr<SchedulerStateStub> createScheduleItem(const SchedulerStateStub &s) const override {
            using namespace std::chrono;
            auto r = std::make_shared<SchedulerStateStub>(s);
            auto steadyNow = steady_clock::now();
            if (s.name == "aaa") {
                r->nextTime = steadyNow;
            } else {
                r->nextTime = steadyNow + 10_s;
            }

            auto delay = duration_cast<system_clock::duration>(r->nextTime - steadyNow);
            LogInfo("first run exporter <{}>, do hash to {:L}, delay: {:.3f}ms",
                    r->getName(), system_clock::now() + delay,
                    duration_cast<fraction_millis>(delay).count());
            return r;
        }

        SyncQueue<int, Block> queue{0};

        int scheduleOnce(SchedulerStateStub &) override {
            return 0;
        }

        int executeItem(std::weak_ptr<SchedulerStateStub> weak,
                        const std::chrono::steady_clock::time_point &steadyNow) override {
            int code = SchedulerT::executeItem(weak, steadyNow);
            // 第一个通知，已调度
            // 第二个阻塞，队列关闭
            queue << 1 << 2;
            return code;
        }

    };

    SchedulerStub scheduler;
    scheduler.runIt();

    auto d = std::make_shared<std::map<std::string, SchedulerStateStub>>();
    d->emplace("aaa", SchedulerStateStub{"aaa"});
    d->emplace("bbb", SchedulerStateStub{"bbb"});
    scheduler.setItems(d);

    int n;
    scheduler.queue >> n; // 确保已调度
    EXPECT_EQ(n, 1);

    SyncQueue<int> threadState{2};
    std::thread thread([&]() {
        threadState << 1;
        scheduler.close(); // 这里会死锁在join上
        threadState << 2;
    });
    threadState.Wait();

    std::this_thread::sleep_for(10_ms);
    EXPECT_FALSE(threadState.TryTake(n)); // thread依然在等待

    scheduler.queue >> n;
    thread.join();  // 依然在死锁
}
