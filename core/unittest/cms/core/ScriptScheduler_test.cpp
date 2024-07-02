#include <gtest/gtest.h>
#include "core/ScriptScheduler.h"
#include "core/ChannelManager.h"
#include "core/argus_manager.h"

#include <map>
#include <algorithm> // std::min

#include "common/ProcessWorker.h"
#include "common/Logger.h"
#include "common/ErrorCode.h"
#include "common/Uuid.h"
#include "common/SyncQueue.h"
#include "common/Defer.h"
#include "common/ChronoLiteral.h"
#include "common/Config.h"

using namespace std;
using namespace common;
using namespace argus;

#ifdef min
#   undef min
#   undef max
#endif

class ChannelManagerStub : public ChannelManager {
public:
    mutable int msgCount = 0;

    int sendMsgToNextModule(const std::string &name, const std::vector<ChanConf> &outputVector,
                            const std::chrono::system_clock::time_point &timestamp,
                            int exitCode, const std::string &result, bool status,
                            const std::string &mid) const override {
        msgCount++;
        return 0;
    }

    mutable int rawDataCount = 0;

    int SendRawDataToNextModule(const common::RawData &rawData,
                                const std::vector<ChanConf> &outputVector) const override {
        rawDataCount++;
        return 0;
    }

    mutable int metricsCallCount = 0;
    mutable SyncQueue<std::string> queueMetric{10};

    int SendMetricsToNextModule(const std::vector<common::CommonMetric> &metrics,
                                const std::vector<ChanConf> &outputVector) const override {
        size_t index = 0;
        for (const auto &item: metrics) {
            LogInfo("[{}] {}", index++, item.toString());
            queueMetric << item.toString();
        }
        metricsCallCount++;
        return 0;
    }
};

class CoreScriptSchedulerTest : public testing::Test {
public:
    void SetUp() override {
        auto *argusManager = SingletonArgusManager::Instance();
        oldChannelManager = argusManager->mChannelManager;
        argusManager->mChannelManager = std::shared_ptr<ChannelManager>(&channelManager, [](ChannelManager *) {});
    }

    void TearDown() override {
        SingletonArgusManager::Instance()->mChannelManager = oldChannelManager;
    }

    ChannelManagerStub channelManager;
    std::shared_ptr<ChannelManager> oldChannelManager;
};

TEST_F(CoreScriptSchedulerTest, BuildExecuteResult) {
    ScriptScheduler pTest;
    string result = pTest.BuildExecuteResult("test\nName", "ok", 10, 20);
    EXPECT_EQ(result,
              R"XX(__argus_script_status__{__argus_script_name__="test\nName",__argus_script_code__="10",__argus_script_error_msg__="ok"} 20)XX");
}

TEST_F(CoreScriptSchedulerTest, State_getName) {
    ScriptScheduleState state;
    state.name = __FUNCTION__;
    EXPECT_EQ(state.name, state.getName());
}

TEST_F(CoreScriptSchedulerTest, outputResult_RawFormat) {
    ScriptScheduler scheduler;
    auto state = std::make_shared<ScriptScheduleState>();
    state->resultFormat = RAW_FORMAT;

    std::string errMsg;
    EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, ""));
    EXPECT_EQ(channelManager.msgCount, 1);

    state->resultFormat = ScriptResultFormat(-1);
    EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, ""));
    EXPECT_EQ(channelManager.msgCount, 2);
}

TEST_F(CoreScriptSchedulerTest, outputResult_RawDataFormat) {
    ScriptScheduler scheduler;
    auto state = std::make_shared<ScriptScheduleState>();
    state->resultFormat = RAW_DATA_FORMAT;
    state->reportStatus = 1;

    std::string errMsg;
    EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, ""));
    EXPECT_EQ(channelManager.rawDataCount, 1);
}

TEST_F(CoreScriptSchedulerTest, outputResult_PrometheusFormat) {
    ScriptScheduler scheduler;
    auto state = std::make_shared<ScriptScheduleState>();
    state->resultFormat = PROMETHEUS_FORMAT;

    std::string errMsg;
    EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, ""));
    EXPECT_EQ(channelManager.metricsCallCount, 1);

    struct {
        int reportStatus;
        const char *metric;
    } testCases[] = {
            {1, R"XX(__argus_script_status__{__argus_script_parse_msg__="success",__argus_script_raw_msg__=""} 1 )XX"},
            {2, R"XX(__argus_script_status__{__argus_script_parse_msg__="success",__argus_script_raw_msg__="__argus_script_status__{} 1"} 1)XX"},
    };
    for (const auto &tc: testCases) {
        state->reportStatus = tc.reportStatus;
        const char *szResult = R"XX(
__argus_script_status__{} 1)XX";
        EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, szResult + 1));
        std::string metric;
        channelManager.queueMetric >> metric;
        EXPECT_TRUE(HasPrefix(metric, tc.metric));
    }
}

TEST_F(CoreScriptSchedulerTest, outputResult_AlimonitorJsonFormat) {
    ScriptScheduler scheduler;
    auto state = std::make_shared<ScriptScheduleState>();
    state->resultFormat = ALIMONITOR_JSON_FORMAT;

    std::string errMsg;
    EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, ""));
    EXPECT_EQ(channelManager.metricsCallCount, 1);

    const char *szResult = R"({"MSG": {"__argus_script_status__": 0}, "collection_flag": 0, "error_info": ""})";
    struct {
        int reportStatus;
        const char *metric;
    } testCases[] = {
            {1, R"XX(__argus_script_status__{__argus_script_parse_msg__="success",__argus_script_raw_msg__=""} 0 )XX"},
            {2, R"XX(__argus_script_status__{__argus_script_parse_msg__="success",__argus_script_raw_msg__="{\"MSG\": {\"__argus_script_status__\": 0}, \"collection_flag\": 0, \"error_info\": \"\"}"} 0 )XX"},
    };
    for (const auto &tc: testCases) {
        state->reportStatus = tc.reportStatus;
        EXPECT_EQ(0, scheduler.outputResult(state, 0, errMsg, szResult));
        std::string metric;
        EXPECT_TRUE(channelManager.queueMetric.TryTake(metric));
        EXPECT_TRUE(HasPrefix(metric, tc.metric));
    }
}

TEST_F(CoreScriptSchedulerTest, getStateMap) {
    ScriptScheduler scheduler;
    scheduler.m_state.Emplace("hello", std::make_shared<ScriptScheduleState>());
    scheduler.m_state.Emplace("world", std::make_shared<ScriptScheduleState>());

    std::map<std::string, std::shared_ptr<ScriptScheduleState>> stateMap;
    scheduler.getStateMap(stateMap, "hello");
    EXPECT_NE(nullptr, stateMap["hello"].get());
}

TEST_F(CoreScriptSchedulerTest, schedule_checkFirstRun_01) {
    ScriptScheduler scheduler;

    ScriptItem item;
    item.scheduleExpr = QUARTZ_ALL;
    item.scheduleIntv = 15_s;
    item.mid = NewUuid();
    EXPECT_EQ(0, scheduler.schedule(item));
    EXPECT_EQ(size_t(1), scheduler.m_state.Size());
}

TEST_F(CoreScriptSchedulerTest, schedule_checkFirstRun_02) {
    std::chrono::seconds offsets[] = {24_h, -24_h};
    for (const auto offset: offsets) {
        ScriptItem item;
        item.scheduleExpr = QUARTZ_ALL;
        item.scheduleIntv = 15_s;
        item.mid = NewUuid().substr(0, 8);
        item.firstSche = std::chrono::system_clock::now() + offset;

        ScriptScheduler scheduler;
        EXPECT_EQ(0, scheduler.schedule(item));
        EXPECT_EQ(size_t(1), scheduler.m_state.Size());
    }
}

TEST_F(CoreScriptSchedulerTest, schedule_checkFirstRun_03) {
    ScriptItem item;
    item.scheduleExpr = "[* * * * * ? 2002]"; // 2002年
    item.timePeriod.parse(item.scheduleExpr.c_str());
    item.scheduleIntv = 15_s;
    item.mid = NewUuid();

    ScriptScheduler scheduler;
    EXPECT_EQ(-1, scheduler.schedule(item));
}

TEST_F(CoreScriptSchedulerTest, schedule_checkFirstRun_04) {
    auto nowTime = std::chrono::system_clock::now();
    auto tm = fmt::localtime(std::chrono::system_clock::to_time_t(nowTime));

    ScriptItem item;
    item.scheduleExpr = fmt::format("[* * * {} * ?]", tm.tm_mday);
    item.timePeriod.parse(item.scheduleExpr.c_str());
    item.scheduleIntv = 15_s;
    item.firstSche = std::chrono::system_clock::now() + 24_h;
    item.mid = NewUuid();

    ScriptScheduler scheduler;
    ASSERT_EQ(0, scheduler.schedule(item));
    EXPECT_LE(scheduler.m_state.GetCopy()[item.mid]->lastBegin, nowTime + item.scheduleIntv);
}

TEST_F(CoreScriptSchedulerTest, checkRun) {
    ScriptScheduler scheduler;

    ScriptItem item;
    item.scheduleIntv = 30_min; // 30分钟
    auto state = std::make_shared<ScriptScheduleState>();
    state->lastBegin = std::chrono::system_clock::now() + 1_h;
    EXPECT_EQ(-1, scheduler.checkRun(item, state));

    state->lastBegin = std::chrono::system_clock::now() - 1_min;
    EXPECT_EQ(-2, scheduler.checkRun(item, state));

    state->lastBegin = std::chrono::system_clock::now() - 40_min;
    item.scheduleExpr = "[* * * * * ? 2002]";
    EXPECT_TRUE(item.timePeriod.parse(item.scheduleExpr.c_str()));
    EXPECT_EQ(-3, scheduler.checkRun(item, state));

    class ScriptSchedulerStub : public ScriptScheduler {
    public:
        int runItem(const ScriptItem &s,
                    const std::shared_ptr<ScriptScheduleState> &state,
                    const std::chrono::system_clock::time_point &now) override {
            m_running["ScriptSchedulerStub"] = std::shared_ptr<ProcessWorker>();
            return 0;
        }
    } stub;

    item.scheduleExpr = QUARTZ_ALL;
    item.timePeriod.parse(item.scheduleExpr.c_str());
    EXPECT_EQ(0, stub.checkRun(item, state));
    EXPECT_NE(stub.m_running.find("ScriptSchedulerStub"), stub.m_running.end());
}

TEST_F(CoreScriptSchedulerTest, runItem) {
    class ProcessWorkerStub : public ProcessWorker {
    public:
        int retCreate = 0;

        int create2(const char *prog, const char *const *args,
                    const char *user, int login, bool childBlock) override {
            return retCreate;
        }
    };
    ProcessWorkerStub pw;

    ScriptScheduler scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    ScriptItem item;
    item.resultFormat = RAW_DATA_FORMAT;
    item.scheduleIntv = 30_min; // 30分钟
    item.scheduleExpr = "[* * * 1-31 * ?]";
    item.timePeriod.parse(item.scheduleExpr.c_str());
    item.mid = "mid-" + NewUuid().substr(0, 8);
    EXPECT_EQ(0, scheduler.checkFirstRun(item));
    ASSERT_TRUE(scheduler.m_state.Contains(item.mid));

    std::shared_ptr<ScriptScheduleState> state;
    EXPECT_TRUE(scheduler.m_state.Find(item.mid, state));
    EXPECT_TRUE((bool) state);
    auto now = std::chrono::system_clock::now();

    state->lastBegin = now;
    EXPECT_EQ(-2, scheduler.runItem(item, state, now));

    state->lastBegin = now - 1_min - 2 * item.scheduleIntv;
    auto oldTimePeriod = item.timePeriod;
    item.timePeriod = TimePeriod{};
    EXPECT_EQ(-3, scheduler.runItem(item, state, now));
    item.timePeriod = oldTimePeriod;

    state->lastBegin = now - 1_min - 2 * item.scheduleIntv;
    pw.retCreate = -1;
    EXPECT_EQ(-1, scheduler.runItem(item, state, now));

    EXPECT_EQ(scheduler.m_running.find(item.mid), scheduler.m_running.end());
    state->lastBegin = now - 1_min - 2 * item.scheduleIntv;
    pw.retCreate = 0;
    EXPECT_EQ(0, scheduler.runItem(item, state, now));
    EXPECT_NE(scheduler.m_running.find(item.mid), scheduler.m_running.end());
}

TEST_F(CoreScriptSchedulerTest, recycleKilled) {
    class ProcessWorkerStub : public ProcessWorker {
    public:
        int counter = 0;
        bool wait(EnumWaitHow) override {
            counter++;
            return (counter % 2) == 0;
        }
    };
    ProcessWorkerStub pw;

    ScriptScheduler scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });
    scheduler.m_killed.push_back(scheduler.createProcessWorker());
    scheduler.m_killed.push_back(scheduler.createProcessWorker());
    scheduler.recycleKilled();
    EXPECT_EQ(size_t(1), scheduler.m_killed.size());
}

namespace stub {
    class ProcessWorkerStub : public ProcessWorker {
    public:
        bool exited = false;

        bool wait(EnumWaitHow) override {
            return exited;
        }

        int createCode = 0;

        int create2(const char *prog, const char *const *args,
                    const char *user, int login, bool childBlock) override {
            return createCode;
        }

        mutable std::string stdOut;
        mutable std::string stdErr;

        size_t output(char *buf, size_t len) const override {
            return read(stdOut, buf, len);
        }

        size_t errput(char *buf, size_t len) const override {
            return read(stdErr, buf, len);
        }

        bool kill(int) override {
            return true;
        }

    private:
        static size_t read(std::string &data, char *buf, size_t len) {
            len = std::min(len, data.size());
            if (len > 0) {
                memcpy(buf, data.c_str(), len);
                data = data.substr(len);
            }
            return len;
        }
    };

    class ScriptSchedulerStub : public ScriptScheduler {
    public:
        std::string output;

        explicit ScriptSchedulerStub(const std::function<std::shared_ptr<ProcessWorker>()> &f)
                : ScriptScheduler(f) {
        }

        int lastErrCode = 0;

        int outputResult(const std::shared_ptr<ScriptScheduleState> &state,
                         int exitCode, std::string &errMsg, const std::string &result) override {
            lastErrCode = exitCode;
            output = result;
            return 0;
        }

        std::string BuildExecuteResult(const std::string &scriptName, std::string errorMsg,
                                       int exitCode, int metricNum) override {
            return errorMsg;
        }
    };
}

void initForCheckForEnd(stub::ScriptSchedulerStub &scheduler) {
    std::map<std::string, ScriptItem> &mapItem = scheduler.m_items;
    for (int i = 0; i < 2; i++) {
        ScriptItem item;
        item.resultFormat = RAW_DATA_FORMAT;
        item.scheduleIntv = 30_min; // 30分钟
        item.scheduleExpr = "[* * * 1-31 * ?]";
        item.timePeriod.parse(item.scheduleExpr.c_str());
        item.mid = "mid-" + NewUuid().substr(0, 8);
        EXPECT_EQ(0, scheduler.checkFirstRun(item));
        ASSERT_TRUE(scheduler.m_state.Contains(item.mid));
        mapItem[item.mid] = item;
    }
    EXPECT_EQ(scheduler.m_state.Size(), size_t(2));
    auto &state = scheduler.m_state;
    Sync(state)
            {
                auto now = std::chrono::system_clock::now();
                for (const auto &item: state) {
                    item.second->lastBegin = now - 31_min;
                    scheduler.runItem(mapItem[item.first], item.second, now);
                }
            }}}
    scheduler.m_state.Erase(mapItem.begin()->first);
    EXPECT_EQ(scheduler.m_state.Size(), size_t(1));
    std::shared_ptr<ScriptScheduleState> item;
    EXPECT_TRUE(scheduler.m_state.Find(mapItem.rbegin()->first, item));
}

TEST_F(CoreScriptSchedulerTest, checkForEnd001) {
    stub::ProcessWorkerStub pw;
    stub::ScriptSchedulerStub scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    initForCheckForEnd(scheduler);
    std::shared_ptr<ScriptScheduleState> item;
    EXPECT_TRUE(scheduler.m_state.Find(scheduler.m_items.rbegin()->first, item));

    pw.exited = false;
    pw.stdOut = "hello";
    EXPECT_EQ(0, scheduler.checkForEnd());
    EXPECT_EQ(item->output, "hello");

    pw.stdOut = " world";
    pw.exited = true;
    EXPECT_EQ(0, scheduler.checkForEnd());
    EXPECT_EQ(scheduler.output, "hello world\n");
}

TEST_F(CoreScriptSchedulerTest, checkForEnd002_exit_otuput_overflow) {
    stub::ProcessWorkerStub pw;
    stub::ScriptSchedulerStub scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    initForCheckForEnd(scheduler);
    std::shared_ptr<ScriptScheduleState> item;
    EXPECT_TRUE(scheduler.m_state.Find(scheduler.m_items.rbegin()->first, item));

    size_t maxOutputLen = SingletonConfig::Instance()->getMaxOutputLen();
    defer(SingletonConfig::Instance()->m_config.maxOutputLen = maxOutputLen);
    SingletonConfig::Instance()->m_config.maxOutputLen = 2;

    pw.exited = true;
    pw.stdErr = "hello";
    EXPECT_EQ(0, scheduler.checkForEnd());
    EXPECT_EQ(size_t(0), scheduler.m_killed.size());
    EXPECT_EQ(E_OutputTooLong, scheduler.lastErrCode);
}

TEST_F(CoreScriptSchedulerTest, checkForEnd002_timeout_shouldFinished_overflow) {
    stub::ProcessWorkerStub pw;
    stub::ScriptSchedulerStub scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    initForCheckForEnd(scheduler);
    std::shared_ptr<ScriptScheduleState> item;
    EXPECT_TRUE(scheduler.m_state.Find(scheduler.m_items.rbegin()->first, item));

    pw.exited = false;
    pw.stdOut = "hello";
    item->shouldFinish = std::chrono::system_clock::now() - 1_min;
    EXPECT_EQ(0, scheduler.checkForEnd());
    EXPECT_EQ(size_t(1), scheduler.m_killed.size());
    EXPECT_EQ(E_TimeOut, scheduler.lastErrCode);
}

TEST_F(CoreScriptSchedulerTest, checkForEnd002_timeout_maxOutputLen_overflow) {
    stub::ProcessWorkerStub pw;
    stub::ScriptSchedulerStub scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    initForCheckForEnd(scheduler);
    std::shared_ptr<ScriptScheduleState> item;
    EXPECT_TRUE(scheduler.m_state.Find(scheduler.m_items.rbegin()->first, item));

    size_t maxOutputLen = SingletonConfig::Instance()->getMaxOutputLen();
    defer(SingletonConfig::Instance()->m_config.maxOutputLen = maxOutputLen);
    SingletonConfig::Instance()->m_config.maxOutputLen = 2;

    pw.exited = false;
    pw.stdOut = "hello";
    item->shouldFinish = std::chrono::system_clock::now() - 1_min;
    EXPECT_EQ(0, scheduler.checkForEnd());
    EXPECT_EQ(size_t(1), scheduler.m_killed.size());
    EXPECT_EQ(E_OutputTooLong, scheduler.lastErrCode);
}

TEST_F(CoreScriptSchedulerTest, onTimer) {
    stub::ProcessWorkerStub pw;
    stub::ScriptSchedulerStub scheduler([&]() {
        return std::shared_ptr<ProcessWorker>{&pw, [](ProcessWorker *) {}};
    });

    const auto interval = 30_min;
    constexpr const size_t itemCount = 2;
    for (size_t i = 0; i < itemCount; i++) {
        ScriptItem item;
        item.resultFormat = RAW_DATA_FORMAT;
        item.scheduleIntv = interval; // 30分钟
        item.scheduleExpr = "[* * * 1-31 * ?]";
        item.timePeriod.parse(item.scheduleExpr.c_str());
        item.mid = "mid-" + NewUuid().substr(0, 8);
        scheduler.m_items[item.mid] = item;
    }
    const std::string wildMid = "mid-" + NewUuid().substr(0, 8);
    scheduler.m_state.Emplace(wildMid, std::make_shared<ScriptScheduleState>());
    EXPECT_EQ(scheduler.m_state.Size(), size_t(1));

    scheduler.onTimer();  // checkFirstRun

    EXPECT_EQ(scheduler.m_state.Size(), itemCount);
    EXPECT_FALSE(scheduler.m_state.Contains(wildMid));
    auto &state = scheduler.m_state;
    Sync(state) {
        for (auto &item: state) {
            EXPECT_FALSE(item.second->isRunning);
            item.second->lastBegin = std::chrono::system_clock::now() - interval - 1_min; // 下次onTimer时触发
        }
    }}}

    scheduler.onTimer();  // checkRun

    EXPECT_EQ(scheduler.m_state.Size(), itemCount);
    Sync(state) {
        for (const auto &item: state) {
            EXPECT_TRUE(item.second->isRunning);
        }
    }}}
}
