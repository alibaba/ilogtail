#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif

#include <gtest/gtest.h>
#include <apr-1/apr_file_io.h>
#include "detect/detect_schedule.h"
#include "detect/detect_schedule_status.h"

#include <iostream>

#include "common/Logger.h"
#include "common/CommonUtils.h"
#include "common/ModuleData.h"
#include "common/StringUtils.h"
#include "common/Uuid.h"

#include "detect/telnet_detect.h"
#include "detect/http_detect.h"
#include "detect/detect_ping_poll.h"

#include "common/Exceptions.h"
#include "common/Nop.h"

using namespace std;
using namespace std::chrono;
using namespace common;
using namespace argus;

#define HTTP "http"

class DetectScheduleTest : public testing::Test {
    DetectSchedule *pShared = nullptr;
protected:
    void SetUp() override {
        // pShared = new DetectSchedule(true);
        // pShared->runIt();
    }

    void TearDown() override {
        delete pShared;
        SingletonTaskManager::destroyInstance();
    }

    DetectSchedule *schedule(bool init = true) {
        if (pShared == nullptr) {
            pShared = new DetectSchedule(init);
            pShared->runIt();
        }
        return pShared;
    }
};

TEST_F(DetectScheduleTest, PingCollect) {
    // DetectPingPoll* pPoll = SingletonDetectPingPoll::Instance();
    // pPoll->runIt();

    // std::this_thread::sleep_for(seconds{ 1 });

    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));

    TaskManager *pTask = SingletonTaskManager::Instance();
    std::unordered_map<std::string, PingItem> pingItems;
    pingItems["ping001"] = PingItem("ping001", "www.baidu.com", seconds{1});
    pingItems["ping002"] = PingItem("ping002", "www.taobao.com", seconds{1});
    pingItems["ping003"] = PingItem("ping003", "127.0.0.1", seconds{1});
    pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
    std::this_thread::sleep_for(seconds{1});

    auto pred = [&]() {
        return pTask->PingItems().Get() == schedule()->mPrevPingItems && schedule()->mPingItems.Size() == size_t(3);
    };
    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{1500}); // 再等1.5秒，至少有一次ping结果
    }

    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));

    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingDetectMap.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskStatusMap.Size(), size_t(3));
    if (schedule()->mTaskResult.Contains("ping001")) {
        EXPECT_GE(schedule()->mTaskResult["ping001"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping002")) {
        EXPECT_GE(schedule()->mTaskResult["ping002"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping003")) {
        LogInfo("TaskID=ping003, count={}, lostCount={}, rt={}",
                schedule()->mTaskResult["ping003"].front()->mPingResult.count,
                schedule()->mTaskResult["ping003"].front()->mPingResult.lostCount,
                schedule()->mTaskResult["ping003"].front()->mPingResult.windowSharePtr->GetAvgValue());
    }

    LogInfo("");
    LogInfo("**********************************************************************");
    LogInfo("");
    pingItems.clear();
    pingItems["ping0001"] = PingItem("ping0001", "www.baidu.com", seconds{1});
    pingItems["ping0002"] = PingItem("ping0002", "www.taobao.com", seconds{1});
    pingItems["ping0003"] = PingItem("ping0003", "127.0.0.1", seconds{1});
    pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));

    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{700});
    }
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(3));
    EXPECT_GE(schedule()->mTaskResult.Size(), size_t(2));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));

    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));
    EXPECT_GE(schedule()->mPingDetectMap.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskStatusMap.Size(), size_t(3));
    if (schedule()->mTaskResult.Contains("ping0001")) {
        EXPECT_GE(schedule()->mTaskResult["ping0001"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping0002")) {
        EXPECT_GE(schedule()->mTaskResult["ping0002"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping0003")) {
        LogInfo("TaskID=ping0003, count={}, lostCount={}, rt={}",
                schedule()->mTaskResult["ping0003"].front()->mPingResult.count,
                schedule()->mTaskResult["ping0003"].front()->mPingResult.lostCount,
                schedule()->mTaskResult["ping0003"].front()->mPingResult.windowSharePtr->GetAvgValue());
    }

    CollectData collectData;
    schedule()->Collect(collectData);
    std::cout << "collectData: " << collectData.toJson() << std::endl;
    EXPECT_EQ(collectData.dataVector.size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    EXPECT_GE(schedule()->mPingDetectMap.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskStatusMap.Size(), size_t(3));

    // SingletonDetectPingPoll::destroyInstance();
}

TEST_F(DetectScheduleTest, TelnetCollect) {
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));

    TaskManager *pTask = SingletonTaskManager::Instance();
    std::unordered_map<std::string, TelnetItem> telnetItems;
    telnetItems["telnet001"] = TelnetItem("telnet001", "www.baidu.com:80", seconds{1});
    telnetItems["telnet002"] = TelnetItem("telnet002", "www.taobao.com:80", seconds{1});
    telnetItems["telnet003"] = TelnetItem("telnet003", "127.0.0.1:22", seconds{1});
    pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));

    auto pred = [&]() {
        return pTask->TelnetItems().Get() == schedule()->mPrevTelnetItems && schedule()->mTelnetItems.Size() == size_t(3);
    };
    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{1500});
    }
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    if (schedule()->mTaskResult.Contains("telnet001")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet001"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : "
             << schedule()->mTaskResult["telnet001"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet002")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet002"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : " << schedule()->mTaskResult["telnet002"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet003")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet003"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : " << schedule()->mTaskResult["telnet003"].front()->mTelnetResult.latency << endl;
    }

    telnetItems.clear();
    telnetItems["telnet0001"] = TelnetItem("telnet0001", "www.baidu.com:80", seconds{1});
    telnetItems["telnet0002"] = TelnetItem("telnet0002", "www.taobao.com:80", seconds{1});
    telnetItems["telnet0003"] = TelnetItem("telnet0003", "127.0.0.1:22", seconds{1});
    pTask->SetTelnetItems(std::make_shared<std::unordered_map<std::string, TelnetItem>>(telnetItems));

    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{800});
    }
    if (schedule()->mTaskResult.Contains("telnet0001")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet0001"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : "
             << schedule()->mTaskResult["telnet0001"].front()->mTelnetResult.latency
             << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet0002")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet0002"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : "
             << schedule()->mTaskResult["telnet0002"].front()->mTelnetResult.latency
             << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet0003")) {
        cout << "status " << TELNET_SUCCESS << " : "
             << schedule()->mTaskResult["telnet0003"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << " : "
             << schedule()->mTaskResult["telnet0003"].front()->mTelnetResult.latency <<
             endl;
    }
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(3));
#ifdef WIN32
    constexpr const size_t expectResultSize = 2;  // 127.0.0.1:22 在windows下是不通的
#else
    constexpr const size_t expectResultSize = 3;
#endif
    EXPECT_EQ(schedule()->mTaskResult.Size(), expectResultSize);
    CollectData collectData;
    schedule()->Collect(collectData);
    std::cout << "collectData: " << collectData.toJson() << std::endl;
    EXPECT_EQ(collectData.dataVector.size(), expectResultSize);
    EXPECT_EQ(schedule()->mTaskResult.Size(), expectResultSize);
}

TEST_F(DetectScheduleTest, HttpCollect) {
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));
    TaskManager *pTask = SingletonTaskManager::Instance();
    std::unordered_map<std::string, HttpItem> httpItems;
    httpItems["http001"] = HttpItem("http001", HTTP "://www.baidu.com", seconds{1}, seconds{1});
    httpItems["http002"] = HttpItem("http002", HTTP "://www.taobao.com", seconds{1}, seconds{1});
    httpItems["http003"] = HttpItem("http003", HTTP "://10.137.71.5", seconds{1}, seconds{1}, "Got");
    pTask->SetHttpItems(std::make_shared<decltype(httpItems)>(httpItems));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));

    auto pred = [&]() {
        return pTask->HttpItems().Get() == schedule()->mPrevHttpItems && schedule()->mHttpItems.Size() == size_t(3);
    };
    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{1500});
    }
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    auto printResult = [&](const decltype(schedule()->mTaskResult) &result, const std::string &taskId) {
        Sync(result)
                {
                    auto it = result.find(taskId);
                    cout << "HTTP[" << taskId << "], exist: " << (it != result.end() ? "true" : "false");
                    if (it != result.end()) {
                        cout << ", code: " << it->second.front()->mHttpResult.code;
                        cout << ", latency: " << it->second.front()->mHttpResult.latency;
                    }
                    std::cout << std::endl;
                }}}
    };
    printResult(schedule()->mTaskResult, "http001");
    printResult(schedule()->mTaskResult, "http002");
    printResult(schedule()->mTaskResult, "http003");

    httpItems.clear();
    httpItems["http0001"] = HttpItem("http0001", HTTP "://www.baidu.com", seconds{1}, seconds{1});
    httpItems["http0002"] = HttpItem("http0002", HTTP "://www.taobao.com", seconds{1}, seconds{1});
    httpItems["http0003"] = HttpItem("http0003", HTTP "://10.137.71.5", seconds{1}, seconds{1}, "Got");
    httpItems["http0003"].effective = QUARTZ_ALL;
    pTask->SetHttpItems(MakeCopyShared(httpItems));

    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{800});
    }
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    printResult(schedule()->mTaskResult, "http0001");
    printResult(schedule()->mTaskResult, "http0002");
    printResult(schedule()->mTaskResult, "http0003");

    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskStatusMap.Size(), size_t(3));
    CollectData collectData;
    schedule()->Collect(collectData);
    std::cout << "collectData: " << collectData.toJson() << std::endl;
    EXPECT_EQ(collectData.dataVector.size(), size_t(3));
    EXPECT_EQ(schedule()->mTaskResult.Size(), size_t(3));
}

TEST_F(DetectScheduleTest, Collect00) {
    auto curResult = std::make_shared<DetectTaskResult>();
    curResult->type = DETECT_TYPE(-1);


    auto *p = schedule(false);
    auto &taskResult = p->mTaskResult;
    Sync(taskResult) {
        taskResult["sss"].push_back(curResult);
    }}}

    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    CollectData data;
    EXPECT_EQ(0, p->Collect(data));
    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, "undefined task type"));
}

TEST_F(DetectScheduleTest, Collect01) {
    extern bool curlVerbose;

    bool old = curlVerbose;
    curlVerbose = false;
    defer(curlVerbose = old);

    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(0));
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(0));
    std::unordered_map<std::string, PingItem> pingItems;
    std::unordered_map<std::string, TelnetItem> telnetItems;
    std::unordered_map<std::string, HttpItem> httpItems;
    TaskManager *pTask = SingletonTaskManager::Instance();
    //set ping
    pingItems["ping001"] = PingItem("ping001", "www.baidu.com", seconds{1});
    pingItems["ping002"] = PingItem("ping002", "www.taobao.com", seconds{1});
    pingItems["ping003"] = PingItem("ping003", "127.0.0.1", seconds{1});
    pTask->SetPingItems(std::make_shared<std::unordered_map<std::string, PingItem>>(pingItems));
    //set telnet
    telnetItems["telnet001"] = TelnetItem("telnet001", "www.baidu.com:80", seconds{1});
    telnetItems["telnet002"] = TelnetItem("telnet002", "www.taobao.com:80", seconds{1});
    telnetItems["telnet003"] = TelnetItem("telnet003", "127.0.0.1:22", seconds{1});
    pTask->SetTelnetItems(MakeCopyShared(telnetItems));
    //set http
    httpItems["http001"] = HttpItem("http001", HTTP "://www.baidu.com", seconds{1}, seconds{1});
    httpItems["http002"] = HttpItem("http002", HTTP "://www.taobao.com", seconds{1}, seconds{1});
    httpItems["http003"] = HttpItem("http003", HTTP "://10.137.71.5", seconds{1}, seconds{1});
    pTask->SetHttpItems(MakeCopyShared(httpItems));

    auto pred = [&]() {
        return pTask->HttpItems().Get() == schedule()->mPrevHttpItems && schedule()->mHttpItems.Size() == size_t(3)
               && pTask->TelnetItems().Get() == schedule()->mPrevTelnetItems && schedule()->mTelnetItems.Size() == size_t(3)
               && pTask->PingItems().Get() == schedule()->mPrevPingItems && schedule()->mPingItems.Size() == size_t(3);
    };
    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{800});
    }

    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(3));
    EXPECT_GT(schedule()->mTaskResult.Size(), size_t(0));
    //check ping
    if (schedule()->mTaskResult.Contains("ping001")) {
        EXPECT_GE(schedule()->mTaskResult["ping001"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping002")) {
        EXPECT_GE(schedule()->mTaskResult["ping002"].front()->mPingResult.count, 1);
    }
    if (schedule()->mTaskResult.Contains("ping003")) {
        cout << "ping003--count=" << schedule()->mTaskResult["ping003"].front()->mPingResult.count << ",lostCount="
             << schedule()->mTaskResult["ping003"].front()->mPingResult.lostCount << endl;
    }
    //check telnet
    if (schedule()->mTaskResult.Contains("telnet001")) {
        cout << "status" << TELNET_SUCCESS << ":" << schedule()->mTaskResult["telnet001"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet001"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet002")) {
        cout << "status" << TELNET_SUCCESS << ":"
             << schedule()->mTaskResult["telnet002"].front()->mTelnetResult.status << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet002"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet003")) {
        cout << "status" << TELNET_SUCCESS << ":" << schedule()->mTaskResult["telnet003"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet003"].front()->mTelnetResult.latency << endl;
    }
    //check http
    if (schedule()->mTaskResult.Contains("http001")) {
        cout << "code:" << 200 << ":" << schedule()->mTaskResult["http001"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http001"].front()->mHttpResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("http002")) {
        cout << "code:" << 200 << ":" << schedule()->mTaskResult["http002"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http002"].front()->mHttpResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("http003")) {
        cout << "code:" << HTTP_TIMEOUT << ":" << schedule()->mTaskResult["http003"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http003"].front()->mHttpResult.latency << endl;
    }

    pingItems.clear();
    telnetItems.clear();
    httpItems.clear();
    //set ping
    pingItems["ping0001"] = PingItem("ping0001", "www.baidu.com", seconds{1});
    pingItems["ping0002"] = PingItem("ping0002", "www.taobao.com", seconds{1});
    pingItems["ping0003"] = PingItem("ping0003", "127.0.0.1", seconds{1});
    pTask->SetPingItems(MakeCopyShared(pingItems));
    //set telnet
    telnetItems["telnet0001"] = TelnetItem("telnet0001", "www.baidu.com:80", seconds{1});
    telnetItems["telnet0002"] = TelnetItem("telnet0002", "www.taobao.com:80", seconds{1});
    telnetItems["telnet0003"] = TelnetItem("telnet0003", "127.0.0.1:22", seconds{1});
    pTask->SetTelnetItems(MakeCopyShared(telnetItems));
    //set http
    httpItems["http0001"] = HttpItem("http0001", HTTP "://www.baidu.com", seconds{1}, seconds{1});
    httpItems["http0002"] = HttpItem("http0002", HTTP "://www.taobao.com", seconds{1}, seconds{1});
    httpItems["http0003"] = HttpItem("http0003", HTTP "://10.137.71.5", seconds{1}, seconds{1});
    pTask->SetHttpItems(MakeCopyShared(httpItems));

    auto start = std::chrono::steady_clock::now();
    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{800});
    }

    // 这里虽然二次赋值，则其taskId，以及内容均未变
    pTask->SetPingItems(MakeCopyShared(pingItems));
    pTask->SetTelnetItems(MakeCopyShared(telnetItems));
    pTask->SetHttpItems(MakeCopyShared(httpItems));

    if (WaitFor(pred, seconds{5})) {
        std::this_thread::sleep_for(milliseconds{800});
    }

    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(3));
    EXPECT_GE(schedule()->mTaskResult.Size(), size_t(7)); // 实际应为9，允许失败2次
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingDetectMap.Size(), size_t(3));
    //check ping
    if (schedule()->mTaskResult.Contains("ping0001")) {
        EXPECT_GT(schedule()->mTaskResult["ping0001"].front()->mPingResult.count, 0);
    }
    if (schedule()->mTaskResult.Contains("ping0002")) {
        EXPECT_GT(schedule()->mTaskResult["ping0002"].front()->mPingResult.count, 0);
    }
    //check telnet
    if (schedule()->mTaskResult.Contains("telnet0001")) {
        cout << "status" << TELNET_SUCCESS << ":" << schedule()->mTaskResult["telnet0001"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet0001"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet0002")) {
        cout << "status" << TELNET_SUCCESS << ":" << schedule()->mTaskResult["telnet0002"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet0002"].front()->mTelnetResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("telnet0003")) {
        cout << "status" << TELNET_SUCCESS << ":" << schedule()->mTaskResult["telnet0003"].front()->mTelnetResult.status
             << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["telnet0003"].front()->mTelnetResult.latency << endl;
    }
    //check http
    if (schedule()->mTaskResult.Contains("http0001")) {
        cout << "code:" << 200 << ":" << schedule()->mTaskResult["http0001"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http0001"].front()->mHttpResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("http0002")) {
        cout << "code:" << 200 << ":" << schedule()->mTaskResult["http0002"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http0002"].front()->mHttpResult.latency << endl;
    }
    if (schedule()->mTaskResult.Contains("http0003")) {
        cout << "code:" << HTTP_TIMEOUT << ":" << schedule()->mTaskResult["http0003"].front()->mHttpResult.code << endl;
        cout << "latency" << ":" << schedule()->mTaskResult["http0003"].front()->mHttpResult.latency << endl;
    }

    auto end = std::chrono::steady_clock::now();
    CollectData collectData;
    schedule()->Collect(collectData);
    int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    EXPECT_EQ(schedule()->mHttpItems.Size(), size_t(3));
    EXPECT_GT(schedule()->mTaskResult.Size(), size_t(6)); // 实际应为9, 允许失败3次
    EXPECT_EQ(schedule()->mTelnetItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingItems.Size(), size_t(3));
    EXPECT_EQ(schedule()->mPingDetectMap.Size(), size_t(3));
    size_t dataSize = collectData.dataVector.size();
    LogInfo("collectData.dataVector.size() => {}", dataSize);
    // 网络好的话，http和telnet可以执行2次，ping有固定间隔只能执行1次
    int64_t minSeconds = seconds - 2;
    minSeconds = minSeconds < 0 ? 0 : minSeconds;
    EXPECT_GE(dataSize, size_t(minSeconds * 4/*两个telnet、两个http*/ + 16));
    EXPECT_LE(dataSize, size_t((minSeconds+2) * 4/*两个telnet、两个http*/ + 10 * 2));

    std::cout << "[Module] " << collectData.moduleName << std::endl;
    size_t index = 0;
    for (const auto &it: collectData.dataVector) {
        std::cout << "[" << (index++) << "] " << it.tagMap.find("taskId")->second
                  << ": " << ModuleData::convert(it) << std::endl;
    }
}

TEST_F(DetectScheduleTest, GetStatus) {
    common::CommonMetric metric;
    schedule(false)->GetStatus(metric);
    EXPECT_EQ(metric.name, "detect_status");
}

TEST_F(DetectScheduleTest, SchedulePingTaskUnit) {
    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    {
        DetectSchedule::SchedulePingTaskUnit(nullptr, 1);
        std::string cache = SingletonLogger::Instance()->getLogCache(true);
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "pingDetect null"));
    }
    {
        struct PingDetectStub : PingDetect {
            PingDetectStub(const std::string &destHost, const std::string &taskId) : PingDetect(destHost, taskId) {
            }

            bool PingSend() override {
                return false;
            }
        } pingDetect("www.baidu.com", "ping-" + NewUuid().substr(0, 8));
        DetectSchedule::SchedulePingTaskUnit(&pingDetect, 1);
        std::string cache = SingletonLogger::Instance()->getLogCache();
        std::cout << cache;
        EXPECT_TRUE(StringUtils::Contain(cache, "send error"));
    }
}

TEST_F(DetectScheduleTest, DetectPingFail01) {
    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    PingItem item;
    schedule(false)->DetectPing(item);

    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, boost::core::demangle(typeid(ExpectedFailedException).name())));
    EXPECT_FALSE(schedule()->mTaskStatusMap.Contains(item.taskId));
}

TEST_F(DetectScheduleTest, DetectPingFail02) {
    SingletonLogger::Instance()->switchLogCache(true);
    defer(SingletonLogger::Instance()->switchLogCache(false));

    PingItem item;
    item.taskId = "ping-" + NewUuid().substr(0, 8);
    item.host = "www.aliyun.com";
    item.interval = 15_s;
    schedule(false)->DetectPing(item);

    std::string cache = SingletonLogger::Instance()->getLogCache();
    std::cout << cache;
    EXPECT_TRUE(StringUtils::Contain(cache, "mPingScheduleTaskTimerList empty"));
    EXPECT_FALSE(schedule()->mTaskStatusMap.Contains(item.taskId));
}

TEST_F(DetectScheduleTest, DetectScheduleStatus) {
    DetectScheduleStatus status;
    auto p = std::shared_ptr<DetectSchedule>(schedule(false), NopDelete<DetectSchedule>);
    status.SetDetectSchedule(p);
    common::CommonMetric metric;
    status.GetStatus(metric);
    EXPECT_FALSE(metric.name.empty());
}
