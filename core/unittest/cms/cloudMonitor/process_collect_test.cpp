#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#if defined(__APPLE__) || defined(__FreeBSD__)
#include "common/ThrowWithTrace.h"  // 要求 sout << std::exception 前置声明
#endif
#include "cloudMonitor/process_collect.h"

#include "common/Config.h"

#include "common/FilePathUtils.h"
#include "common/Logger.h"
#include "common/ModuleData.h"
#include "common/StringUtils.h"
#include "common/TimeProfile.h"
#include "common/JsonValueEx.h"
#include "common/ModuleTool.h"
#include "common/ConsumeCpu.h"
#include "common/Chrono.h"

#include "cloudMonitor/cloud_module_macros.h"
#include "core/TaskManager.h"

using namespace std;
using namespace common;
using namespace cloudMonitor;
using namespace argus;

DECLARE_CMS_EXTERN_MODULE(process);

class Cms_ProcessCollectTest : public testing::Test {
protected:
    void SetUp() override {
        SingletonConfig::Instance();

        p_shared = new ProcessCollect();
        p_shared->Init(10);
    }

    void TearDown() override {
        SingletonTaskManager::destroyInstance();
        delete p_shared;
        p_shared = nullptr;
    }

    ProcessCollect *p_shared = nullptr;
};

TEST_F(Cms_ProcessCollectTest, CollectUnit) {
    EXPECT_EQ(0, chrono::steady_clock::time_point{}.time_since_epoch().count());
    using namespace chrono;

    TimeProfile tp;
    vector<pid_t> pids;
    EXPECT_EQ(0, p_shared->GetProcessPids(pids, 0, nullptr));
    cout << "pidNum=" << pids.size() << ", spend: " << tp.cost<std::chrono::fraction_micros>() << endl;
#ifndef WIN32
    SystemTaskInfo systemTaskInfo;
    EXPECT_EQ(0, p_shared->GetSystemTask(pids, systemTaskInfo));
    cout << "processNum=" << systemTaskInfo.processCount << endl;
    cout << "threadNum=" << systemTaskInfo.threadCount << endl;
#endif
    //获取topN进程的pid排序
    vector<pid_t> sortPids;
    p_shared->CopyAndSortByCpu(pids, sortPids);
    EXPECT_EQ(size_t(0), sortPids.size());
    //获取topN进程的信息
    vector<ProcessStat> processStats;
    EXPECT_EQ(0, p_shared->GetTopNProcessStat(sortPids, p_shared->mTopN, processStats));
    EXPECT_EQ(size_t(0), processStats.size());
    std::this_thread::sleep_for(std::chrono::seconds{2});
    p_shared->CopyAndSortByCpu(pids, sortPids);
    cout << "sortPidsSize=" << sortPids.size() << endl;
    cout << "topN=" << p_shared->mTopN << endl;
    EXPECT_EQ(0, p_shared->GetTopNProcessStat(sortPids, p_shared->mTopN, processStats));
    cout << "processStatsSize=" << processStats.size() << endl;
    for (auto &processStat: processStats) {
        cout << "pid=" << processStat.processInfo.pid << ",name=" << processStat.processInfo.name << endl;
    }
    auto processCollectItems = std::make_shared<vector<ProcessCollectItem>>();
    {
        ProcessCollectItem processCollectItem1;
        processCollectItem1.name = "staragent";
        processCollectItems->push_back(processCollectItem1);
    }
    {
        ProcessCollectItem processCollectItem2;
        processCollectItem2.name = "argusagent";
        processCollectItem2.tags.emplace_back("app", "argus-test");
        processCollectItems->push_back(processCollectItem2);
    }
    SingletonTaskManager::Instance()->SetProcessCollectItems(processCollectItems);
    // 获取进程匹配数据
    vector<ProcessMatchInfo> processMatchInfos;
    p_shared->GetProcessMatchInfos(pids, processMatchInfos);
    EXPECT_EQ(size_t(2), processMatchInfos.size());
    for (const auto &processMatchInfo: processMatchInfos) {
        cout << "name=" << processMatchInfo.name << ", size=" << processMatchInfo.pids.size() << ": "
             << common::StringUtils::join(processMatchInfo.pids, ", ", "[", "]") << endl;
    }
    CollectData collectData;
    collectData.moduleName = "process";
    for (const auto &processMatchInfo: processMatchInfos) {
        MetricData sysProcessNumber;
        p_shared->GetProcessMatchInfoMetricData("system.process.number", processMatchInfo, sysProcessNumber);
        collectData.dataVector.push_back(sysProcessNumber);

        if (processMatchInfo.name == "argusagent") {
            std::stringstream oss;
            collectData.print(oss);
            EXPECT_TRUE(StringUtils::Contain(oss.str(), R"(__tag__app=argus-test)"));
        }
    }
    std::stringstream oss;
    collectData.print(oss);
    LogInfo(oss.str());

    //agent自身信息统计
    ProcessSelfInfo processSelfInfo;
    EXPECT_EQ(0, p_shared->GetProcessSelfInfo(processSelfInfo));
    cout << "pid=" << processSelfInfo.pid << endl;
    cout << "run_time=" << processSelfInfo.runTime << endl;
    cout << "start_time=" << processSelfInfo.startTime << endl;
    cout << "sys_run_time=" << processSelfInfo.sysRunTime << endl;
    cout << "sys_up_time=" << processSelfInfo.sysUpTime << endl;
    cout << "cpu_percent=" << processSelfInfo.cpuPercent << endl;
    cout << "mem_percent=" << processSelfInfo.memPercent << endl;
    cout << "mem_resident=" << processSelfInfo.memResident << endl;
    cout << "mem_size=" << processSelfInfo.memSize << endl;
    cout << "openfiles=" << processSelfInfo.openfiles << endl;
    cout << "exe_args=" << processSelfInfo.exeArgs << endl;
    cout << "exe_cwd=" << processSelfInfo.exeCwd << endl;
    cout << "exe_name=" << processSelfInfo.exeName << endl;
    cout << "exe_root=" << processSelfInfo.exeRoot << endl;
}

TEST_F(Cms_ProcessCollectTest, ToJson) {
    vector<pid_t> pids;
    EXPECT_EQ("[]", p_shared->ToJson(pids));
    pids.push_back(1);
    EXPECT_EQ("[1]", p_shared->ToJson(pids));
    pids.push_back(2);
    EXPECT_EQ("[1,2]", p_shared->ToJson(pids));
}

TEST_F(Cms_ProcessCollectTest, Collect) {
    //构造进程统计配置
    auto processCollectItems = std::make_shared<vector<ProcessCollectItem>>();
    ProcessCollectItem processCollectItem1;
    processCollectItem1.name = "staragent";
    ProcessCollectItem processCollectItem2;
    processCollectItem2.name = "argusagent";
    processCollectItems->push_back(processCollectItem1);
    processCollectItems->push_back(processCollectItem2);
    SingletonTaskManager::Instance()->SetProcessCollectItems(processCollectItems);
    string collectStr;
    CollectData collectData1, collectData2;
    p_shared->Collect(collectStr);
    ModuleData::convertStringToCollectData(collectStr, collectData1);
    EXPECT_EQ(collectData1.moduleName, p_shared->mModuleName);
    collectData1.print();
    std::this_thread::sleep_for(std::chrono::seconds{2});
    p_shared->Collect(collectStr);
    ModuleData::convertStringToCollectData(collectStr, collectData2);
    EXPECT_EQ(collectData2.moduleName, p_shared->mModuleName);
    collectData2.print();
}

TEST_F(Cms_ProcessCollectTest, CollectNoProcMatch) {
    //构造进程统计配置
    auto processCollectItems = std::make_shared<vector<ProcessCollectItem>>();
    // ProcessCollectItem processCollectItem1;
    // processCollectItem1.name = "staragent";
    // ProcessCollectItem processCollectItem2;
    // processCollectItem2.name = "argusagent";
    // processCollectItems->push_back(processCollectItem1);
    // processCollectItems->push_back(processCollectItem2);
    SingletonTaskManager::Instance()->SetProcessCollectItems(processCollectItems);
    string collectStr;
    CollectData collectData1, collectData2;
    p_shared->Collect(collectStr);
    ModuleData::convertStringToCollectData(collectStr, collectData1);
    EXPECT_EQ(collectData1.moduleName, p_shared->mModuleName);
    collectData1.print();
    std::this_thread::sleep_for(std::chrono::seconds{2});
    p_shared->Collect(collectStr);
    ModuleData::convertStringToCollectData(collectStr, collectData2);
    EXPECT_EQ(collectData2.moduleName, p_shared->mModuleName);
    collectData2.print();
}

TEST_F(Cms_ProcessCollectTest, cpu) {
    EXPECT_EQ(-1, cloudMonitor_process_collect(nullptr, nullptr));

    auto *handler = cloudMonitor_process_init("");
    EXPECT_NE(nullptr, handler);
    defer(cloudMonitor_process_exit(handler));

    consumeCpu();
    char *data = nullptr;
    {
        EXPECT_GT(cloudMonitor_process_collect(handler, &data), 0);
        LogDebug("{}", data);
    }
    CollectData collectData1;
    EXPECT_TRUE(ModuleData::convertStringToCollectData(data, collectData1));
    EXPECT_EQ(collectData1.moduleName, p_shared->mModuleName);
    size_t expectCount = (isWin32? 1: 2);
#if !defined(WIN32) && defined(ENABLE_CMS_SYS_TASK)
    expectCount++;
#endif
    EXPECT_EQ(collectData1.dataVector.size(), expectCount); // windows下不采集system.task、vm.ProcessCount
    collectData1.print();
    consumeCpu();
    {
        EXPECT_GT(cloudMonitor_process_collect(handler, &data), 0);
        LogDebug("{}", data);
    }

    CollectData collectData2;
    EXPECT_TRUE(ModuleData::convertStringToCollectData(data, collectData2));
    EXPECT_EQ(collectData2.moduleName, p_shared->mModuleName);
    // EXPECT_EQ(collectData2.dataVector.size(), 8);
    collectData2.print();
    // EXPECT_EQ(0, cloudMonitor_process_exit());
}

TEST_F(Cms_ProcessCollectTest, ParseTopType) {
    EXPECT_EQ(ProcessCollect::ParseTopType("cpu"), ProcessCollect::Cpu);
    EXPECT_EQ(ProcessCollect::ParseTopType("fd"), ProcessCollect::Fd);
    EXPECT_EQ(ProcessCollect::ParseTopType("mem"), ProcessCollect::Memory);
    EXPECT_EQ(ProcessCollect::ParseTopType("memory"), ProcessCollect::Memory);

    bool caught = false;
    try {
        EXPECT_EQ(ProcessCollect::ParseTopType("other"), ProcessCollect::Fd);
    } catch (const std::exception &) {
        caught = true;
    }
    EXPECT_TRUE(caught);
}

TEST_F(Cms_ProcessCollectTest, toolCollectTopHelp) {
    char *argv[] = {
            (char *) "argus-agent",
            (char *) "tool",
            (char *) "top",
            (char *) "-h",
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    ASSERT_EQ(4, argc);
    std::stringstream ss;
    ProcessCollect::toolCollectTopN(argv[0], argc - 2, argv + 2, ss);
    std::cout << ss.str() << std::endl;
    EXPECT_TRUE(HasPrefix(ss.str(), fmt::format("Usage: {}", argv[0])));
}

TEST_F(Cms_ProcessCollectTest, toolCollectTopParametersError) {
    char *argv[] = {
            (char *) "argus-agent",
            (char *) "tool",
            (char *) "top",
            (char *) "--by=disk",
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    ASSERT_EQ(4, argc);

    bool caught = false;
    try {
        std::stringstream ss;
        ProcessCollect::toolCollectTopN(argv[0], argc - 2, argv + 2, ss);
    } catch (const std::exception &ex) {
        caught = true;
        std::cout << (sout{} << ex).str() << std::endl;
    }
    EXPECT_TRUE(caught);
}

TEST_F(Cms_ProcessCollectTest, toolCollectTopFd) {
    char *argv[] = {
            (char *) "argus-agent",
            (char *) "tool",
            (char *) "top",
            (char *) "-b",
            (char *) "fd",
            (char *) "--number=2"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    ASSERT_EQ(6, argc);
    std::stringstream ss;
    ProcessCollect::toolCollectTopN(argv[0], argc - 2, argv + 2, ss);
    std::cout << ss.str() << std::endl;

    json::Array array = json::parseArray(ss.str());
    EXPECT_EQ(size_t(2), array.size());
}

TEST_F(Cms_ProcessCollectTest, toolCollectTopCpu) {
    char *argv[] = {
            (char *) "argus-agent",
            (char *) "tool",
            (char *) "top",
            (char *) "--by=cpu",
            (char *) "-n",
            (char *) "2",
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    ASSERT_EQ(6, argc);
    std::stringstream ss;
    ProcessCollect::toolCollectTopN(argv[0], argc - 2, argv + 2, ss);
    std::cout << ss.str() << std::endl;

    json::Array array = json::parseArray(ss.str());
    EXPECT_EQ(size_t(2), array.size());
}

TEST_F(Cms_ProcessCollectTest, toolCollectTopMemory) {
    char *argv[] = {
            (char *) "argus-agent",
            (char *) "tool",
            (char *) "top",
            (char *) "--by=memory",
            (char *) "-n",
            (char *) "2",
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    ASSERT_EQ(6, argc);
    std::stringstream ss;
    ProcessCollect::toolCollectTopN(argv[0], argc - 2, argv + 2, ss);
    std::cout << ss.str() << std::endl;

    json::Array array = json::parseArray(ss.str());
    EXPECT_EQ(size_t(2), array.size());
}

TEST_F(Cms_ProcessCollectTest, ProcessMatchInfo_isMatch) {
    {
        ProcessMatchInfo matchInfo;
        matchInfo.processName = "A";

        ProcessInfo pi;
        pi.name = "b";
        EXPECT_FALSE(matchInfo.isMatch(pi));
    }
    {
        ProcessMatchInfo matchInfo;
        matchInfo.processUser = "A";

        ProcessInfo pi;
        pi.user = "b";
        EXPECT_FALSE(matchInfo.isMatch(pi));
    }
    {
        ProcessMatchInfo matchInfo;
        matchInfo.name = "A";

        ProcessInfo pi;
        pi.path = "b";
        EXPECT_FALSE(matchInfo.isMatch(pi));
    }
    {
        ProcessMatchInfo matchInfo;
        matchInfo.name = "a"; // 这里需要小写

        ProcessInfo pi;
        pi.path = "/usr/A";
        EXPECT_TRUE(matchInfo.isMatch(pi));
    }
}
