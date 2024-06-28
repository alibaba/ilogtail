//
// Created by 韩呈杰 on 2023/5/25.
//
#include <gtest/gtest.h>
#include "common/ResourceConsumptionRecord.h"

#include "common/Common.h" // GetThisThreadId
#include "common/Logger.h"
#include "common/StringUtils.h"
#include "common/ChronoLiteral.h"
#include "common/Chrono.h"

using namespace std::chrono;
using namespace common;

TEST(ResourceConsumptionRecorderTest, ResourceConsumptionRecord) {
    ResourceConsumption record;
    record.TaskName = "cpu";
    record.Millis = 36_ms;
    record.ThreadId = 114660;

    EXPECT_EQ(record.str(), "thread: 114660, millis:   36, taskName: cpu");
    LogInfo("{}", record.str());
}

TEST(ResourceConsumptionRecorderTest, PrintTasks) {
    ResourceConsumptionRecorder record;
    // record.EnableRecording(false);
    EXPECT_TRUE(record.IsRecording());
    // EXPECT_FALSE(record.AddTask("cpu", 15_ms, GetThisThreadId()));

    // record.EnableRecording(true);
    EXPECT_TRUE(record.AddTask("cpu", 15_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("disk", 20_ms, GetThisThreadId()));

    std::vector<ResourceConsumption> tasks;
    EXPECT_EQ((size_t) 2, record.GetTopTasks(tasks));
    std::cout << ResourceConsumptionRecorder::PrintTasks(tasks) << std::endl;

    std::string s = record.PrintTasks();
    auto lines = StringUtils::split(s, '\n', {false, false});
    EXPECT_EQ((size_t) 2, lines.size());
}

TEST(ResourceConsumptionRecorderTest, TopN) {
    ResourceConsumptionRecorder record;
    // record.EnableRecording(true);
    EXPECT_TRUE(record.AddTask("1-cpu", 99_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("2-disk", 95_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("3-cpu", 90_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("4-disk", 85_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("5-cpu", 80_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("6-disk", 75_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("7-cpu", 70_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("8-disk", 65_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("9-cpu", 60_ms, GetThisThreadId()));
    EXPECT_TRUE(record.AddTask("A-disk", 55_ms, GetThisThreadId())); // 这个会被挤出
    EXPECT_TRUE(record.AddTask("B-disk", 80_ms, GetThisThreadId()));
    EXPECT_EQ((size_t) 10, record.Count());

    std::vector<ResourceConsumption> tasks;
    EXPECT_EQ((size_t) record.topN, record.GetTopTasks(tasks));
    std::string s = ResourceConsumptionRecorder::PrintTasks(tasks);
    std::cout << s << std::endl;

    EXPECT_NE(s.find("9-cpu"), std::string::npos);
    EXPECT_NE(s.find("B-disk"), std::string::npos);
    EXPECT_EQ(s.find("A-disk"), std::string::npos);

    // record.EnableRecording(false);
    // EXPECT_EQ((size_t) 0, record.Count());
}

TEST(ResourceConsumptionRecorderTest, Task1) {
    ResourceConsumptionRecorder record;

    {
        ResourceConsumptionRecorder::Task task(__FUNCTION__, std::chrono::steady_clock::now() - 2_ms, &record);
        std::string working = record.PrintTasks();
        LogInfo(working);
        EXPECT_FALSE(working.empty());
    }

    std::string s = record.PrintTasks();
    LogInfo(s);
    auto lines = StringUtils::split(s, '\n', {false, false});
    EXPECT_EQ((size_t) 1, lines.size());
}

TEST(ResourceConsumptionRecorderTest, Task2) {
    ResourceConsumptionRecorder record;
    ResourceConsumptionRecorder::Task task(__FUNCTION__, &record);
    EXPECT_FALSE(IsZero(task.startTime));
}

TEST(ResourceConsumptionRecorderTest, Task3_Macro) {
    ResourceConsumptionRecorder record;
    {
        CpuProfile(__FUNCTION__, std::chrono::steady_clock::now() - 1_min, &record);
    }
    std::string s = record.PrintTasks();
    LogInfo(s);
    auto lines = StringUtils::split(s, '\n', {false, false});
    EXPECT_EQ((size_t) 1, lines.size());
    EXPECT_TRUE(Contain(s, fmt::format("{}, millis:", GetThisThreadId())));
}

TEST(ResourceConsumptionRecorderTest, Task3_topN) {
    ResourceConsumptionRecorder record;
    Random<int> rand(1, 60);
    std::vector<std::shared_ptr<ResourceConsumptionRecorder::Task>> working;
    for (size_t i = 0; i < ResourceConsumptionRecorder::topN * 1.5; i++)
    {
        working.push_back(std::make_shared<ResourceConsumptionRecorder::Task>(
                fmt::format("{}.{}", __FUNCTION__, i), std::chrono::steady_clock::now() - rand.next() * 1_s, &record));
    }
    EXPECT_GT(record.workingTask.size(), ResourceConsumptionRecorder::topN);
    std::string s = record.PrintTasks();
    LogInfo("\n{}", s);
    auto lines = StringUtils::split(s, '\n', {false, false});
    EXPECT_EQ((size_t) ResourceConsumptionRecorder::topN, lines.size());
}
