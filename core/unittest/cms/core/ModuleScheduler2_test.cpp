//
// Created by 韩呈杰 on 2024/2/6.
//
#include <gtest/gtest.h>
#include "core/ModuleScheduler2.h"
#include "core/module.h"

#include <memory>

#include "common/Logger.h"
#include "common/ChronoLiteral.h"
#include "common/SyncQueue.h"
#include "common/Uuid.h"
#include "common/ModuleTool.hpp"
#include "common/StringUtils.h"
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "common/Common.h"   // typeName
#include "common/CommonMetric.h"

using namespace argus;
using namespace common;

class Core_ModuleScheduler2Test : public ::testing::Test {
};

struct FakeModuleScheduler2 {
    std::string s = "hello";
    std::atomic<int> count{0};
    int collectCode = 1;
    SyncQueue<int, Nonblock> chan{10};

    ~FakeModuleScheduler2() {
        LogInfo("{}({})", __FUNCTION__, (void *) this);
    }

    static int Init() {
        return 0;
    }

    int Collect(std::string &d) {
        int cnt = ++count;
        defer(chan << cnt);

        d = fmt::format("{}[{}]", s, cnt);
        return collectCode > 0 ? static_cast<int>(d.size()) : collectCode;
    }
};

IMPLEMENT_MODULE(Fake_Module_Scheduler_43) {
    return module::NewHandler<FakeModuleScheduler2>();
}

struct ModuleScheduler2Stub : public ModuleScheduler2 {
public:
    mutable SyncQueue<module::Handler<FakeModuleScheduler2> *> chanHandler{4};

    explicit ModuleScheduler2Stub(bool enableThreadPool = false) : ModuleScheduler2(enableThreadPool) {
    }

    std::shared_ptr<Module> makeModule(const ModuleItem &s) const override {
        struct ModuleStub : public Module {
            bool Init() override {
                return true;
            }
        };
        auto module = std::make_shared<ModuleStub>();
        module->module_init = Fake_Module_Scheduler_43_init;
        module->module_exit = Fake_Module_Scheduler_43_exit;
        module->module_collect = Fake_Module_Scheduler_43_collect;
        module->module_free_buf = Fake_Module_Scheduler_43_free_buf;
        auto *handler = module::NewHandler<FakeModuleScheduler2>();
        module->moduleHandler = handler;

        chanHandler << handler;
        return module;
    }
};

TEST_F(Core_ModuleScheduler2Test, Do) {
    ModuleScheduler2Stub scheduler(true);
    // scheduler.handler.s = "hello";
    scheduler.runIt();

    auto moduleItems = std::make_shared<std::map<std::string, ModuleItem>>();
    ModuleItem moduleItem;
    moduleItem.name = moduleItem.module = "Fake_Module_Scheduler_43";
    moduleItem.scheduleInterval = 20_ms;
    moduleItems->emplace("ddd", moduleItem);
    moduleItems->emplace("aaa", ModuleItem{}); // 这个就是凑数的
    scheduler.setItems(moduleItems);
    // scheduler.mItems = moduleItems;
    // scheduler.cv_.notify_all();

    module::Handler<FakeModuleScheduler2> *handler = nullptr;
    scheduler.chanHandler >> handler;
    ASSERT_NE(nullptr, handler);
    // 至少3次调度
    std::chrono::steady_clock::time_point last;
    for (int i = 1; i <= 3; i++) {
        int count;
        handler->m.chan >> count;
        auto now = std::chrono::steady_clock::now();
        EXPECT_GT(now - last, 15_ms); // 保证调度均匀
        last = now;
        EXPECT_EQ(i, count);
        LogInfo("------ collect({}) ------", count);
    }

    EXPECT_GE(handler->m.count, 2);
    EXPECT_EQ(scheduler.m_state.size(), size_t(1));
}

TEST_F(Core_ModuleScheduler2Test, GetStatus) {
    ModuleScheduler2 scheduler2{false};

    auto skip = std::make_shared<ModuleScheduler2State>();
    skip->item.name = "skip";
    skip->skipCount = 1;
    scheduler2.m_state["mid-skip"] = skip;

    auto error = std::make_shared<ModuleScheduler2State>();
    error->item.name = "error";
    error->errorCount = 1;
    error->skipCount = 2;
    scheduler2.m_state["mid-error"] = error;

    auto ok = std::make_shared<ModuleScheduler2State>();
    ok->item.name = "ok";
    scheduler2.m_state["mid-ok"] = ok;

    common::CommonMetric metric;
    scheduler2.GetStatus(metric);
    LogInfo("status: {}", metric.toString());
    EXPECT_EQ(1, metric.value);
    EXPECT_EQ(metric.tagMap["ok_list"], "ok");
    EXPECT_EQ(metric.tagMap["error_list"], "error");

    std::set<std::string> skipSet{"error,skip", "skip,error"};
    EXPECT_NE(skipSet.end(), skipSet.find(metric.tagMap["skip_list"]));

    std::string strJson = boost::json::serialize(scheduler2.GetStatus("mid-error"));
    LogInfo("mid-error: {}", strJson);
    EXPECT_TRUE(HasPrefix(strJson, R"XX([{"ITEM_NUM":1,"ModuleScheduleState":[)XX"));

}

TEST_F(Core_ModuleScheduler2Test, makeModule) {
    ModuleScheduler2 scheduler2(false);
    ModuleItem item;
    item.module = item.name = "cpu";
    item.isSoType = false;
    EXPECT_TRUE((bool) scheduler2.makeModule(item));
}

TEST_F(Core_ModuleScheduler2Test, createScheduleItem01) {
    ModuleScheduler2 scheduler2(false);
    {
        ModuleItem item;
        item.scheduleInterval = 0_s;
        EXPECT_FALSE((bool) scheduler2.createScheduleItem(item));
    }
    {
        ModuleItem item;
        item.scheduleInterval = 15_s;
        item.module = "cpu:" + NewUuid().substr(0, 8);
        EXPECT_FALSE((bool) scheduler2.createScheduleItem(item));
    }
}

TEST_F(Core_ModuleScheduler2Test, createScheduleItem02) {
    ModuleScheduler2Stub scheduler2(false);
    ModuleItem item;
    item.scheduleInterval = 15_s;
    item.scheduleExpr = "[* * * * * ? 2002]";
    item.timePeriod.parse(item.scheduleExpr.c_str());
    item.outputVector.resize(1);
    item.outputVector[0].first = "cms";
    item.outputVector[0].second = "{}";

    auto *logger = SingletonLogger::Instance();
    logger->switchLogCache(true);
    defer(logger->switchLogCache(false));

    auto state = scheduler2.createScheduleItem(item);
    std::string cache = logger->getLogCache();
    std::cout << cache;

    EXPECT_TRUE((bool) state);
    EXPECT_TRUE(StringUtils::Contain(cache, "delay: 0"));
    auto diff = std::chrono::steady_clock::now() - state->nextTime;
    EXPECT_LT(diff, 500_s);
}

TEST_F(Core_ModuleScheduler2Test, afterScheduleOnce) {
    ModuleScheduler2 scheduler2(false);
    scheduler2.mContinueExceedCount = 3;

    {
        ModuleScheduler2State state;
        state.maxExecDuration = 10_ms;
        state.continueExceedTimes = 1;
        scheduler2.afterScheduleOnce(state, 5_ms);
        EXPECT_EQ(state.lastExecDuration, 5_ms);
        EXPECT_EQ(state.continueExceedTimes, 0);
        EXPECT_EQ(state.exceedSkipTimes, 0);
    }
    {
        ModuleScheduler2State state;
        state.maxExecDuration = 10_ms;
        state.continueExceedTimes = 1;
        scheduler2.afterScheduleOnce(state, 50_ms);
        EXPECT_EQ(state.lastExecDuration, 50_ms);
        EXPECT_EQ(state.continueExceedTimes, 2);
        EXPECT_EQ(state.exceedSkipTimes, 0);
    }
    {
        ModuleScheduler2State state;
        state.maxExecDuration = 10_ms;
        state.continueExceedTimes = scheduler2.mContinueExceedCount - 1;
        scheduler2.afterScheduleOnce(state, 15_ms);
        EXPECT_EQ(state.lastExecDuration, 15_ms);
        EXPECT_EQ(state.continueExceedTimes, 0);
        EXPECT_EQ(state.exceedSkipTimes, 2);
    }
}

TEST_F(Core_ModuleScheduler2Test, scheduleOnce) {
    {
        ModuleScheduler2 scheduler(false);
        // EXPECT_EQ(-1, scheduler.scheduleOnce(std::weak_ptr<ModuleScheduler2State>{}));
        {
            // 不在生效期
            auto state = std::make_shared<ModuleScheduler2State>();
            state->item.scheduleExpr = "[* * * * * ? 2002]";
            EXPECT_TRUE(state->item.timePeriod.parse(state->item.scheduleExpr.c_str()));
            EXPECT_EQ(-2, scheduler.scheduleOnce(*state));
        }
        {
            // 还在暂停期
            auto state = std::make_shared<ModuleScheduler2State>();
            state->exceedSkipTimes = 2;
            EXPECT_EQ(-3, scheduler.scheduleOnce(*state));
        }
    }
    {
        ModuleScheduler2Stub scheduler;
        auto state = std::make_shared<ModuleScheduler2State>();
        state->p_module = scheduler.makeModule(state->item);
        module::Handler<FakeModuleScheduler2> *handler = nullptr;
        scheduler.chanHandler >> handler;
        ASSERT_NE(nullptr, handler);
        handler->m.collectCode = -1;

        EXPECT_EQ(0, state->errorCount);
        EXPECT_EQ(0, scheduler.scheduleOnce(*state));
        EXPECT_EQ(1, state->errorCount);
    }
    {
        ModuleScheduler2Stub scheduler;
        auto state = std::make_shared<ModuleScheduler2State>();
        state->p_module = scheduler.makeModule(state->item);
        module::Handler<FakeModuleScheduler2> *handler = nullptr;
        scheduler.chanHandler >> handler;
        ASSERT_NE(nullptr, handler);
        handler->m.collectCode = 0;

        EXPECT_EQ(0, scheduler.scheduleOnce(*state));
        EXPECT_EQ(0, state->errorCount);
    }
    {
        ModuleScheduler2Stub scheduler;
        auto state = std::make_shared<ModuleScheduler2State>();
        state->p_module = scheduler.makeModule(state->item);
        module::Handler<FakeModuleScheduler2> *handler = nullptr;
        scheduler.chanHandler >> handler;
        ASSERT_NE(nullptr, handler);
        handler->m.collectCode = 1;

        EXPECT_EQ(0, scheduler.scheduleOnce(*state));
        EXPECT_EQ(0, state->errorCount);
    }
}

TEST_F(Core_ModuleScheduler2Test, scheduleNext01) {
    ModuleScheduler2 scheduler(false);

    auto state = std::make_shared<ModuleScheduler2State>();
    state->item.scheduleInterval = 15_s;
    auto now = std::chrono::steady_clock::now() - 5_s - state->item.scheduleInterval;
    state->nextTime = now;

    scheduler.scheduleNextTime(state, now, true);
    EXPECT_EQ(1, state->skipCount);
    EXPECT_EQ(state->nextTime, now + 2 * state->item.scheduleInterval);
}

TEST_F(Core_ModuleScheduler2Test, executeItem) {
    ModuleScheduler2 scheduler(false);
    scheduler.threadPool = ThreadPool::Option{}.min(0).max(1).name(typeName(this)).makePool();
    scheduler.threadPool->_tasks.Close();

    auto state = std::make_shared<ModuleScheduler2State>();
    state->item.name = "commitFail";
    state->item.scheduleInterval = 15_s;
    auto now = std::chrono::steady_clock::now() - 5_s - state->item.scheduleInterval;
    state->nextTime = now;

    EXPECT_EQ(-1, scheduler.executeItem(state, now));
}

TEST_F(Core_ModuleScheduler2Test, TimerEventLess) {
    ModuleScheduler2::TimerEvent event1, event2;
    event1.next = std::chrono::steady_clock::now();
    event2.next = event1.next + 1_ms;
    EXPECT_LT(event1, event2);
}

TEST_F(Core_ModuleScheduler2Test, updateItems) {
    ModuleScheduler2 scheduler2(false);

    ModuleItem item;
    item.scheduleInterval = 15_s;
    item.isSoType = false;
    item.name = item.module = "cpu";
    scheduler2.mItems = std::make_shared<std::map<std::string, ModuleItem>>();
    scheduler2.mItems->emplace("hello", item);

    LogInfo("{:->60}", "");
    std::shared_ptr<std::map<std::string, ModuleItem>> prev;
    {
        scheduler2.updateItems(prev);
        EXPECT_EQ(prev.get(), scheduler2.mItems.get());
        EXPECT_EQ(scheduler2.m_state.size(), size_t(1));
    }

    LogInfo("{:->60}", "");
    {
        auto old = prev;
        auto oldItem = scheduler2.m_state.find("hello")->second;
        scheduler2.mItems = std::make_shared<std::map<std::string, ModuleItem>>();
        scheduler2.mItems->emplace("hello", item);
        scheduler2.updateItems(prev);
        EXPECT_NE(prev.get(), old.get());
        EXPECT_EQ(oldItem.get(), scheduler2.m_state.find("hello")->second.get());
    }
    LogInfo("{:->60}", "");
}
