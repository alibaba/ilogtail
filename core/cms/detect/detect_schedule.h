#ifndef ARGUS_DETECT_SCHEDULE_H
#define ARGUS_DETECT_SCHEDULE_H
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <utility>
#include <chrono>

#include "core/TaskManager.h"

#include "common/ThreadWorker.h"
#include "common/ModuleData.h"
#include "common/CommonMetric.h"
#include "common/CppTimer.h"
#include "common/Singleton.h"
#include "common/SafeVector.h"
#include "common/SafeMap.h"
#include "common/SafeShared.h"

#include "detect_result.h"
#include "ping_detect.h"
#include "http_detect.h"
#include "sliding_time_window.h"

class ThreadPool;

namespace argus {
    struct PingItem;
    struct HttpItem;
    struct TelnetItem;
}
namespace common
{
#include "common/test_support"
class DetectSchedule : public ThreadWorker
{
public:
    explicit DetectSchedule(bool init = false);
    void Start(int threadNum = 5, int maxThreadNum = 10);
    ~DetectSchedule() override;
    int Collect(CollectData &collectData);
    void DetectPing(const argus::PingItem &pingItem);
    void DetectTelnet(const argus::TelnetItem &telnetItem);
    void DetectHttp(const argus::HttpItem &httpItem);
    void GetStatus(CommonMetric &metric);
    void CollectPingResult();

    bool IsTaskRunning(const std::string& taskId);
    bool ClearTask(const std::string& taskId);

    size_t TaskCount() const;

private:
    void doRun() override;
    void Init(int threadNum = 10, int maxThreadNum = 20);
    void UpdateTelnetResult(const argus::TelnetItem &telnetItem, int status,
                            const std::chrono::microseconds &latency);
    void UpdateHttpResult(const argus::HttpItem &httpItem, const HttpDetectResult &res,
                          const std::chrono::microseconds &latency);
    void GetPingMetricData(const std::string &taskId, PingResult &result, MetricData &metricData);
    void GetTelnetMetricData(const std::string &taskId, TelnetResult &result, MetricData &metricData);
    void GetHttpMetricData(const std::string &taskId, HttpResult &result, MetricData &metricData);
    void CheckAndScheduleTelnetTask();
    void CheckAndScheduleHttpTask();
    void CheckAndSchedulePingTask();
    void ScheduleTelnetTask(std::string const &taskId);
    void ScheduleHttpTask(const std::string &taskId);
    void SchedulePingTask(const std::string &taskId);
    static void SchedulePingTaskUnit(PingDetect *pingDetect, CppTime::timer_id timerID);
    void CleanPingUnitTask(std::string const &taskId);

    // apr_pool_t *mPool{};
    // apr_thread_pool_t *mThreadPool{};
    std::shared_ptr<ThreadPool> mThreadPool;
    std::chrono::milliseconds mDetectScheduleUnit{100};

    std::shared_ptr<std::unordered_map<std::string, argus::PingItem>> mPrevPingItems;
    // InstanceLock mPingItemsLock;
    SafeMap<std::string, std::shared_ptr<argus::PingItem>> mPingItems;

    // InstanceLock mTelnetItemsLock;
    std::shared_ptr<std::unordered_map<std::string, argus::TelnetItem>> mPrevTelnetItems;
    SafeMap<std::string, std::shared_ptr<argus::TelnetItem>> mTelnetItems;

    // InstanceLock mHttpItemsLock;
    std::shared_ptr<std::unordered_map<std::string, argus::HttpItem>> mPrevHttpItems;
    SafeMap<std::string, std::shared_ptr<argus::HttpItem>> mHttpItems;

    // std::list在4.9.1下存在bug: size()复杂度为N
    SafeUnorderedMap<std::string, std::list<std::shared_ptr<DetectTaskResult>>> mTaskResult;

    // InstanceLock mTaskStatusLock;
    // Task Status
    // TaskId ------> isRunning
    SafeUnorderedMap<std::string, bool> mTaskStatusMap;

    std::map<std::string, std::string> mAddTags;
    //socket复用
    // InstanceLock mPingDetectMapLock;
    SafeMap<std::string, std::shared_ptr<PingDetect>> mPingDetectMap;
    
    size_t mSkipCount{0};
    std::atomic_flag mInitFlag = ATOMIC_FLAG_INIT;

    // CppTime::timer_id mCheckTaskTimerId = 0;
    // CppTime::Timer mCheckTaskTimer;
    std::shared_ptr<CppTime::Timer> mScheduleTaskTimer;
    /**
     * ping 任务需要每秒调度一次, interval 时间之后收集结果
     * 所以这个成员负责发送ping, 然后在 ping poll中处理回包
     */
    // InstanceLock mPingScheduleTaskTimerListLock;
    SafeVector<std::shared_ptr<CppTime::Timer>> mPingScheduleTaskTimerList;
    // InstanceLock mPingTaskId2TimerIdLock;
    SafeMap<std::string, std::pair<int, CppTime::timer_id>> mPingTaskId2TimerId;
    // CppTime::Timer mPingResultCollectTimer;
    // CppTime::timer_id mPingResultCollectTimerId = 0;
    std::atomic<bool> mEnd{false};

    SafeMap<std::string, CppTime::timer_id> mTaskTimerId;
};
#include "common/test_support"
}
#endif 