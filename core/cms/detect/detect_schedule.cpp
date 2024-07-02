#ifdef WIN32
#include <WinSock2.h> // depended by Ws2ipdef.h
#include <Ws2ipdef.h> // sockaddr_in6, Available in Windows Vista and later versions of the Windows operating systems.
#include <Windows.h> // 如果没有该句，VC下编译时会报：语法错误: 标识符“LPMSG”
#endif

#include "common/ThrowWithTrace.h"
#include "detect_schedule.h"

#include <memory>
#include <utility>
#include <exception>

#include "telnet_detect.h"
#include "http_detect.h"

#include "common/Logger.h"
#include "common/StringUtils.h"
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "common/TimeProfile.h"
#include "common/Common.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/TimeFormat.h"
#include "common/Chrono.h"
#include "common/Exceptions.h"

using namespace std;
using namespace std::chrono;
using namespace argus;

namespace common {

    DetectSchedule::DetectSchedule(bool init) {
        mScheduleTaskTimer = std::make_shared<CppTime::Timer>();
        mDetectScheduleUnit = std::chrono::milliseconds{100};
        if (init) {
            Init();
        }
        LogInfo("in DetectSchedule, init = {}", init);
    }

    void DetectSchedule::Start(int threadNum, int maxThreadNum) {
        Init(threadNum, maxThreadNum);
        this->runIt();
    }

    void DetectSchedule::Init(int threadNum, int maxThreadNum) {
        if (!mInitFlag.test_and_set()) {
            mEnd = false;
            mThreadPool = ThreadPool::Option{}.min(threadNum).max(maxThreadNum).name("DetectSchedule").makePool();

            {
                // LockGuard lockGuard(mPingScheduleTaskTimerListLock);
                int pingSchedulerTimerSize = 1;
                for (int i = 0; i < pingSchedulerTimerSize; ++i) {
                    mPingScheduleTaskTimerList.PushBack(std::make_shared<CppTime::Timer>());
                }
            }
        }
    }

    DetectSchedule::~DetectSchedule() {
        endThread();
        join();

        mScheduleTaskTimer.reset(); // 停止timer

        // {
        //     LockGuard lockGuard(mPingScheduleTaskTimerListLock);
        //     mPingScheduleTaskTimerList.clear();
        mPingScheduleTaskTimerList.Clear();
        // }

        // // 停止检查调度
        // // mCheckTaskTimer.remove(mCheckTaskTimerId);
        // //用于避免在第一个detect模块卸载后（因为周期不一样，会卸载老的）,导致新的detect模块一直不能感受到配置的问题
        // TaskManager *pTask = SingletonTaskManager::Instance();
        // pTask->SetTelnetItems(MakeShared(*mPrevTelnetItems));
        // pTask->SetPingItems(MakeShared(*mPrevPingItems));
        // pTask->SetHttpItems(MakeShared(*mPrevHttpItems));

        mThreadPool.reset();

        LogPrintInfo("DetectSchedule will exit!");
    }


    std::string detectUri(const PingItem &task) {
        return task.host;
    }

    template<typename T>
    std::string detectUri(const T &task) {
        return task.uri;
    }

    template<typename T>
    void scheduleTask(const std::function<bool(const std::string &)> &setTaskRunning, size_t &skipCount,
                      SafeMap<std::string, std::shared_ptr<T>> &mapTask,
                      const std::string &taskId, ThreadPool &threadPool, const std::string &taskTypeName,
                      std::function<void(const T &)> fnDetect) {
        using namespace std::chrono;

        std::shared_ptr<T> detectItem;
        if (mapTask.Find(taskId, detectItem)) {
            auto now = std::chrono::Now<ByMicros>();
            auto steadyNow = steady_clock::now();

            auto writeLog = [&](const char *action) {
                if (SingletonLogger::Instance()->isLevelOK(LEVEL_DEBUG)) {
                    const auto &item = *detectItem;

                    system_clock::duration actualInterval{0};
                    system_clock::time_point actualLast;
                    if (!IsZero(item.lastScheduleTime)) {
                        actualInterval = duration_cast<system_clock::duration>(steadyNow - item.lastScheduleTime);
                        actualLast = now - actualInterval;
                    };
                    LogDebug("TaskID: <{}>[{}], {} <{}>, last: {:L%F %T.000 %z}, "
                             "interval{{\"expect\": {}, \"actual\": {:.3f}s}}, effective: <{}>",
                             item.taskId, detectUri(item), action, taskTypeName, actualLast, item.interval,
                             duration_cast<fraction_seconds>(actualInterval).count(), item.effective.string());
                }
            };

            if (setTaskRunning(taskId)) {
                writeLog("skip unexpected running");
                skipCount++;
                return;
            }

            bool isEffective = detectItem->effective.in(now);
            writeLog(isEffective ? "start" : "skip not effective");
            if (isEffective) {
                detectItem->lastScheduleTime = steadyNow;

                size_t threadPoolQueueSize = threadPool.taskCount();
                size_t threadPoolCapacity = threadPool.maxThreadCount();
                constexpr const int maxTimes = 5;
                if (threadPoolQueueSize > threadPoolCapacity * maxTimes) {
                    LogWarn("TaskID: {}, The task was discarded because the queue exceeds {} times it's pool size. "
                            "current queue size {}, thread pool max size {}, PLEASE increase the thread pool max size",
                            detectItem->taskId, maxTimes, threadPoolQueueSize, threadPoolCapacity);
                } else {
                    threadPool.commit(taskTypeName, [=]() { fnDetect(*detectItem); });
                }
            }
        }
    }

    template<typename TaskItem>
    void checkAndScheduleTask(DetectSchedule *scheduler, const std::string &taskName,
                              std::shared_ptr<CppTime::Timer> &scheduleTaskTimer,
                              SafeMap<std::string, CppTime::timer_id> &taskTimerId,
                              const std::function<bool(std::shared_ptr<std::unordered_map<std::string, TaskItem>> &)> &GlobalTaskItems,
                              std::shared_ptr<std::unordered_map<std::string, TaskItem>> &prevItems,
                              SafeMap<std::string, std::shared_ptr<TaskItem>> &taskItems,
                              void (DetectSchedule::*DoSchedule)(const std::string &)) {
        std::shared_ptr<std::unordered_map<std::string, TaskItem>> latestItems = prevItems;
        if (scheduler->isThreadEnd() || !GlobalTaskItems(latestItems)) {
            return;
        }
        defer(prevItems = latestItems);

        LogInfo("{} Changed! Current {} num is {}", taskName, taskName, latestItems->size());
        defer(LogInfo("{} ~Changed! Current {} num is {}", taskName, taskName, taskItems.Size()));
        std::unordered_map<std::string, TaskItem> &curTaskItems = *latestItems;

        std::vector<std::string> newTaskIds;
        enum EnumPurpose {
            ForCreate,
            ForDelete,
        };
        std::map<std::string, EnumPurpose> deleteTaskIds;
        // std::vector<std::string> modifiedTaskIds;

        // 新增的任务
        Sync(taskItems) {
            // 删除的任务
            for (auto &entry: taskItems) {
                const auto &taskId = entry.first;
                if (curTaskItems.find(taskId) == curTaskItems.end()) {
                    deleteTaskIds[taskId] = ForDelete;
                }
            }

            // 新建或修改的任务
            for (auto &item: curTaskItems) {
                string taskId = item.first;

                auto oldIt = taskItems.find(item.first);
                bool exists = (oldIt != taskItems.end());
                if (!exists || *oldIt->second != item.second) {
                    newTaskIds.push_back(taskId);
                    if (exists) {
                        deleteTaskIds[taskId] = ForCreate;
                    }
                    taskItems[taskId] = std::make_shared<TaskItem>(item.second);
                } // else // 有且相等，保持不变
            }
        }}}

        for (const auto &entry: deleteTaskIds) {
            // scheduleTaskTimer->remove(taskTimerId[taskId]);
            const auto &taskId = entry.first;
            taskTimerId.IfExist(taskId, [&](const CppTime::timer_id timerId) { scheduleTaskTimer->remove(timerId); });
            if (entry.second == ForDelete) {
                taskTimerId.Erase(taskId);
                taskItems.Erase(taskId);
            }
            scheduler->ClearTask(taskId);
        }

        for (const auto &taskId: newTaskIds) {
            decltype(TaskItem::interval) interval = taskItems[taskId]->interval;
            // std::string copiedId{taskId};
            auto timer = [=](CppTime::timer_id) { (scheduler->*DoSchedule)(taskId); };
            taskTimerId.Set(taskId, scheduleTaskTimer->add(seconds(0), timer, seconds{interval}));
        }

        // for (const auto &taskId: modifiedTaskIds) {
        //     // 先删除
        //     // scheduleTaskTimer->remove(taskTimerId[taskId]);
        //     taskTimerId.IfExist(taskId, [&](const CppTime::timer_id timerId) { scheduleTaskTimer->remove(timerId); });
        //     scheduler->ClearTask(taskId);
        //     // 后添加
        //     decltype(TaskItem::interval) interval;
        //     {
        //         //     LockGuard lockGuard(mHttpItemsLock);
        //         auto curItem = std::make_shared<TaskItem>(curTaskItems[taskId]);
        //         interval = curItem->interval;
        //         taskItems.Set(taskId, curItem);
        //     }
        //     std::string copiedId{taskId};
        //     taskTimerId.Set(taskId, scheduleTaskTimer->add(
        //             seconds(0), [=](CppTime::timer_id) { (scheduler->*DoSchedule)(copiedId); }, seconds(interval)));
        // }
    }

    void DetectSchedule::CheckAndScheduleTelnetTask() {
        auto globalItems = [](std::shared_ptr<std::unordered_map<std::string, TelnetItem>> &param) {
            TaskManager *pTask = SingletonTaskManager::Instance();
            return pTask->TelnetItems().Get(param);
        };
        checkAndScheduleTask<TelnetItem>(this, "TelnetItems", mScheduleTaskTimer, mTaskTimerId,
                                         globalItems, mPrevTelnetItems, mTelnetItems,
                                         &DetectSchedule::ScheduleTelnetTask);
    }

    size_t DetectSchedule::TaskCount() const {
        return mTelnetItems.Size() + mHttpItems.Size() + mPingItems.Size();
    }

    void DetectSchedule::ScheduleTelnetTask(string const &taskId) {
        scheduleTask<TelnetItem>([&](const std::string& taskId) {return IsTaskRunning(taskId); },
                                 this->mSkipCount, mTelnetItems, taskId, *mThreadPool, "telnet detect",
                                 [this](const TelnetItem &task) { this->DetectTelnet(task); });
    }

    void DetectSchedule::ScheduleHttpTask(const string &taskId) {
        scheduleTask<HttpItem>([&](const std::string& taskId) {return IsTaskRunning(taskId); },
                               this->mSkipCount, mHttpItems, taskId, *mThreadPool, "http detect",
                               [this](const HttpItem &task) { this->DetectHttp(task); });
    }

    void DetectSchedule::SchedulePingTask(const string &taskId) {
        scheduleTask<PingItem>([&](const std::string& taskId) {return IsTaskRunning(taskId); },
                               this->mSkipCount, mPingItems, taskId, *mThreadPool, "ping detect",
                               [this](const PingItem &task) { this->DetectPing(task); });
    }

    void DetectSchedule::CheckAndScheduleHttpTask() {
        auto globalItems = [](std::shared_ptr<std::unordered_map<std::string, HttpItem>> &param) {
            TaskManager *pTask = SingletonTaskManager::Instance();
            return pTask->HttpItems().Get(param);
        };
        checkAndScheduleTask<HttpItem>(this, "HttpItems", mScheduleTaskTimer, mTaskTimerId,
                                       globalItems, mPrevHttpItems, mHttpItems,
                                       &DetectSchedule::ScheduleHttpTask);
    }

    void DetectSchedule::CheckAndSchedulePingTask() {
        auto globalItems = [](std::shared_ptr<std::unordered_map<std::string, PingItem>> &param) {
            TaskManager *pTask = SingletonTaskManager::Instance();
            return pTask->PingItems().Get(param);
        };
        checkAndScheduleTask<PingItem>(this, "PingItems", mScheduleTaskTimer, mTaskTimerId,
                                       globalItems, mPrevPingItems, mPingItems,
                                       &DetectSchedule::SchedulePingTask);
    }

    void DetectSchedule::CleanPingUnitTask(string const &taskId) {
        // LockGuard lockGuard(mPingTaskId2TimerIdLock);
        std::pair<int, CppTime::timer_id> v;
        if (mPingTaskId2TimerId.Find(taskId, v)) {
            int index = v.first;
            CppTime::timer_id timerId = v.second;
            mPingTaskId2TimerId.Erase(taskId);
            // LockGuard pingScheduleTaskTimerListLockGuard(mPingScheduleTaskTimerListLock);
            mPingScheduleTaskTimerList.At(index, [&](std::shared_ptr<CppTime::Timer> &timer){
                timer->remove(timerId);
                LogPrintWarn("TaskID: %s remove unit timer %d of %d", taskId, timerId, index);
            });
            // if (static_cast<int>(mPingScheduleTaskTimerList.Size()) > index) {
            //     mPingScheduleTaskTimerList[index]->remove(timerId);
            //     LogPrintWarn("TaskID: %s remove unit timer %d", taskId.c_str(), timerId);
            // }
        }
    }

    void DetectSchedule::doRun() {
#ifdef ENABLE_COVERAGE
        seconds checkDelay{0};
        seconds resultCollectDelay{0};
#else
        seconds checkDelay{2};
        seconds resultCollectDelay{1};
#endif
        auto fnCheck = [this](CppTime::timer_id) {
            CpuProfile("DetectSchedule::fnCheck");

            CheckAndScheduleTelnetTask();
            CheckAndScheduleHttpTask();
            CheckAndSchedulePingTask();
        };
        CppTime::Timer checkTaskTimer;
        auto checkId = checkTaskTimer.add(checkDelay, fnCheck, mDetectScheduleUnit);
        LogInfo("DetectSchedule::doRun: checkTaskTimer(delay: {}, interval: {})", checkDelay, mDetectScheduleUnit);

        auto fnCollect = [this](CppTime::timer_id) {
            CpuProfile("DetectSchedule::fnCollect");
            CollectPingResult();
        };
        CppTime::Timer pingResultCollectTimer;
        auto collectId = pingResultCollectTimer.add(resultCollectDelay, fnCollect, mDetectScheduleUnit);
        LogInfo("DetectSchedule::doRun: pingResultCollectTimer(delay: {}, interval: {})",
                resultCollectDelay, mDetectScheduleUnit);

        while (!isThreadEnd()) {
            sleepFor(mDetectScheduleUnit);
        }

        pingResultCollectTimer.remove(collectId);
        checkTaskTimer.remove(checkId);
    }

    void DetectSchedule::CollectPingResult() {
        if (mEnd) {
            return;
        }
        // LockGuard lockGuard(mPingDetectMapLock);
        // for (auto &pingDetectEntry: mPingDetectMap) {
        Sync(mPingDetectMap) {
            for (auto it = mPingDetectMap.begin(); it != mPingDetectMap.end(); ++it) {
                const auto& taskId = it->first;
                auto &pingDetect = it->second;
                // std::shared_ptr<PingDetect> &pingDetect = pingDetectEntry.second;
                if (pingDetect == nullptr) {
                    continue;
                }
                PingResult pingResult = pingDetect->GetPingResult();
                // 1.检查有没有超时的ping
                //   a. 不是刚清理过的ping任务（lastScheduleTime == 0）
                //   b. 当前时间大于调度时间，且间隔大于超时配置
                auto steadyNow = steady_clock::now(); // Now<ByMicros>();
                if (!IsZero(pingResult.lastScheduleTime) && !pingResult.receiveEchoPackage &&
                    steadyNow > pingResult.lastScheduleTime + pingDetect->GetTimeout()) {
                    // LogPrintWarn( "TaskID: %s reach the timeout, count=%d",
                    //          pingDetect->GetTaskId().c_str(),
                    //          pingResult.count);
                    pingDetect->AddTimeoutItem();
                }
                // 2.判断有没有已经完成一个周期的调度任务，收集结果
                //  a. 调度次数达到任务设置
                //  b. 收到最后一个包的回包
                // LogDebug("PING[??][{}], taskId: {}, received: {}, count: {}, interval: {}", pingResult.count,
                //          taskId, pingResult.receiveEchoPackage, pingResult.count, pingDetect->GetInterval());
                if (pingResult.receiveEchoPackage && pingResult.count >= ToSeconds(pingDetect->GetInterval())) {
                    // string taskId = pingDetectEntry.first;
                    {
                        // LockGuard taskStatusLockGuard(mTaskStatusLock);
                        // 判断当前任务是否还在任务列表中
                        if (!mTaskStatusMap.SetIfExist(taskId, false)) {
                            LogDebug("skip CollectPingResult(taskId: {}), because of not in mTaskStatusMap", taskId);
                            continue;
                        }
                        // mTaskStatusMap[taskId] = false;
                    }
                    {
                        auto taskResult = std::make_shared<DetectTaskResult>();
                        taskResult->type = PING_DETECT;
                        taskResult->ts = std::chrono::system_clock::now();
                        taskResult->mPingResult = pingResult;

                        Sync(mTaskResult) {
                            mTaskResult[taskId].push_back(taskResult);
                        }}}
                        LogPrintDebug("TaskID: %s collect ping result, count=%d, lostCount=%d, rt=%f",
                                      pingDetect->GetTaskId(), pingResult.count, pingResult.lostCount,
                                      pingResult.windowSharePtr->GetAvgValue());
                        pingDetect->CleanResult();
                        CleanPingUnitTask(taskId);

                        if (pingResult.lostCount > pingResult.count / 2) {
                            // 若某个周期内的探测成功率低于50%，则会重新创建icmp socket
                            LogDebug("TaskID: {} clean ping socket because of The packet loss rate is too high",
                                          pingDetect->GetTaskId());
                            pingDetect.reset();
                        }
                    }
                }
            }
            for (auto it = mPingDetectMap.begin(); it != mPingDetectMap.end(); ) {
                if (it->second.get() == nullptr) {
                    it = mPingDetectMap.erase(it);
                }
                else {
                    ++it;
                }
            }
        }}}
    }

    void DetectSchedule::DetectPing(const PingItem &pingItem) {
        try {
            auto pDetect = mPingDetectMap.ComputeIfAbsent(pingItem.taskId, [&pingItem]() {
                auto detect = std::make_shared<PingDetect>(pingItem.host, pingItem.taskId, pingItem.interval);
                if (!detect->Init()) {
                    throw ExpectedFailedException("ping init failed");
                }
                return detect;
            });

            CppTime::timer_id timerId;
            int randomIndex = mPingScheduleTaskTimerList.Random([&, pDetect](int idx, std::shared_ptr<CppTime::Timer> &v) {
                auto callback = [=](CppTime::timer_id id) { SchedulePingTaskUnit(pDetect.get(), id); };
                timerId = v->add(seconds(0), callback, seconds(1));
                LogWarn("TaskID: {} add unit timer {} of {}, pingDetect: {}", pingItem.taskId, timerId, idx, (bool)pDetect);
            });
            if (randomIndex < 0) {
                LogWarn("TaskID: {} add unit timer failed: mPingScheduleTaskTimerList empty", pingItem.taskId);
                return;
            }

            // 记录正在调度的 ping 探测任务
            mPingTaskId2TimerId.Set(pingItem.taskId, std::pair<int, CppTime::timer_id>(randomIndex, timerId));
        } catch (const std::exception &ex) {
            mTaskStatusMap.SetIfExist(pingItem.taskId, false);
            LogWarn("{}, TaskID: {}, Host: {}, {}: {}",
                    __FUNCTION__, pingItem.taskId, pingItem.host, typeName(&ex), ex.what());
        }
    }

    void DetectSchedule::SchedulePingTaskUnit(PingDetect *pingDetect, CppTime::timer_id timerID) {
        if (pingDetect == nullptr) {
            LogPrintDebug("skip because of pingDetect null");
            return;
        }
        LogDebug("TaskID: {} schedule unit ping, send icmp package, count={}, timer={}",
                 pingDetect->GetTaskId(), pingDetect->GetPingResult().count, timerID);

        // 只发不收，在poll的事件中处理回包
        auto res = pingDetect->PingSend();
        if (!res) {
            LogWarn("skip ({}) because of ping send error", pingDetect->GetTaskId());
        }
    }

    void DetectSchedule::DetectTelnet(const TelnetItem &telnetItem) {
        TelnetDetect detect(telnetItem.uri, telnetItem.requestBody, telnetItem.keyword, telnetItem.negative);
        TimeProfile tp;
        int status = detect.Detect();
        UpdateTelnetResult(telnetItem, status, tp.cost<std::chrono::microseconds>());
    }

    void DetectSchedule::DetectHttp(const HttpItem &httpItem) {
        HttpDetect detect(httpItem.taskId,
                          httpItem.uri,
                          httpItem.method,
                          httpItem.requestBody,
                          httpItem.header,
                          httpItem.keyword,
                          httpItem.negative,
                          httpItem.timeout);
        TimeProfile tp;
        HttpDetectResult res = detect.Detect();
        UpdateHttpResult(httpItem, res, tp.cost<std::chrono::microseconds>());
    }

    void DetectSchedule::UpdateTelnetResult(const TelnetItem &telnetItem, int status,
                                            const std::chrono::microseconds &latency) {
        // 判断当前任务是否还在任务列表中
        if (!mTaskStatusMap.SetIfExist(telnetItem.taskId, false)) {
            return;
        }

        auto taskResult = std::make_shared<DetectTaskResult>();
        taskResult->type = TELNET_DETECT;
        taskResult->ts = std::chrono::system_clock::now();
        taskResult->mTelnetResult.keyword = telnetItem.keyword;
        taskResult->mTelnetResult.uri = telnetItem.uri;
        taskResult->mTelnetResult.status = status;
        taskResult->mTelnetResult.latency = latency;
        Sync(mTaskResult) {
            mTaskResult[telnetItem.taskId].push_back(taskResult);
        }}}
    }

    void DetectSchedule::UpdateHttpResult(const HttpItem &httpItem, const HttpDetectResult &res,
                                          const std::chrono::microseconds &latency) {
        {
            // LockGuard lockGuard(mTaskStatusLock);
            // 判断当前任务是否还在任务列表中
            if (!mTaskStatusMap.SetIfExist(httpItem.taskId, false)) {
                LogPrintDebug("skip update http result to mTaskResult,taskId=%s", httpItem.taskId);
                return;
            }
            // mTaskStatusMap[httpItem.taskId] = false;
        }
        // LockGuard lockGuard(mTaskResultLock);
        auto taskResult = std::make_shared<DetectTaskResult>();
        taskResult->type = HTTP_DETECT;
        taskResult->ts = std::chrono::system_clock::now();
        taskResult->mHttpResult.keyword = httpItem.keyword;
        taskResult->mHttpResult.uri = httpItem.uri;
        taskResult->mHttpResult.bodyLen = res.bodyLen;
        taskResult->mHttpResult.code = res.code;
        taskResult->mHttpResult.latency = latency;
        //LogPrintDebug("update http result to mTaskResult,taskId=%s",httpItem.taskId.c_str());
        Sync(mTaskResult) {
            mTaskResult[httpItem.taskId].push_back(taskResult);
        }}}
    }

    int DetectSchedule::Collect(CollectData &collectData) {
        TaskManager *pTask = SingletonTaskManager::Instance();
        pTask->GetCmsDetectAddTags(mAddTags);
        collectData.dataVector.clear();

        // LockGuard lockGuard(mTaskResultLock);
        // LogPrintDebug("mTaskResult.size()=%d",mTaskResult.size());
        // for (auto &resultEntry: mTaskResult) {
        // mTaskResult.ForEach([&](const std::string &taskId, std::queue<std::shared_ptr<DetectTaskResult>> &results) {
        std::map<DETECT_TYPE, std::function<void(const std::string &, DetectTaskResult &, MetricData &)>> mapProc{
                {PING_DETECT,   [&](const std::string &taskId, DetectTaskResult &result, MetricData &data) {
                    GetPingMetricData(taskId, result.mPingResult, data);
                }},
                {TELNET_DETECT, [&](const std::string &taskId, DetectTaskResult &result, MetricData &data) {
                    GetTelnetMetricData(taskId, result.mTelnetResult, data);
                }},
                {HTTP_DETECT,   [&](const std::string &taskId, DetectTaskResult &result, MetricData &data) {
                    GetHttpMetricData(taskId, result.mHttpResult, data);
                }},
        };
        Sync(mTaskResult) {
            for (auto &it: mTaskResult) {
                const std::string &taskId = it.first;
                auto &results = it.second;
                // while (!results.empty()) {
                //     auto result = results.front();
                //     results.pop();
                for (auto &result: results) {
                    auto itProc = mapProc.find(result->type);
                    if (itProc == mapProc.end()) {
                        LogError("An undefined task type was detected");
                        continue;
                    }

                    MetricData metricData;
                    itProc->second(taskId, *result, metricData);
#ifdef ENABLE_COVERAGE
                    metricData.tagMap["collectTime"] = date::format(result->ts, "L%H-%M-%S");
#endif
                    collectData.dataVector.push_back(metricData);
                }
                results.clear();
            }
        }}}
        return static_cast<int>(collectData.dataVector.size());
    }

    void DetectSchedule::GetPingMetricData(const string &taskId, PingResult &result, MetricData &metricData) {
        metricData.tagMap.insert(mAddTags.begin(), mAddTags.end());

        metricData.tagMap["metricName"] = "detect.ping";
        metricData.tagMap["taskId"] = taskId;
        metricData.tagMap["ns"] = "acs/detect";
        metricData.tagMap["targetHost"] = result.targetHost;
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["avg"] = result.windowSharePtr->GetAvgValue() / 1000;
        metricData.valueMap["max"] = double(result.windowSharePtr->GetMaxValue()) / 1000;
        metricData.valueMap["min"] = double(result.windowSharePtr->GetMinValue()) / 1000;
        result.windowSharePtr->RemoveTimeoutValue();
        metricData.valueMap["count"] = result.count;
        metricData.valueMap["lostCount"] = result.lostCount;
        metricData.valueMap["lostRate"] = result.count > 0 ? (100.0 * result.lostCount) / result.count : 0.0;
    }

    void DetectSchedule::GetTelnetMetricData(const string &taskId, TelnetResult &result, MetricData &metricData) {
        metricData.tagMap.insert(mAddTags.begin(), mAddTags.end());

        metricData.tagMap["metricName"] = "detect.telnet";
        metricData.tagMap["ns"] = "acs/detect";
        metricData.tagMap["taskId"] = taskId;
        metricData.tagMap["keyword"] = result.keyword;
        metricData.tagMap["uri"] = result.uri;
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["status"] = result.status;
        using namespace std::chrono;
        metricData.valueMap["latency"] = duration_cast<fraction_millis>(result.latency).count();
    }

    void DetectSchedule::GetHttpMetricData(const string &taskId, HttpResult &result, MetricData &metricData) {
        metricData.tagMap.insert(mAddTags.begin(), mAddTags.end());

        metricData.tagMap["metricName"] = "detect.http";
        metricData.tagMap["ns"] = "acs/detect";
        metricData.tagMap["taskId"] = taskId;
        metricData.tagMap["keyword"] = result.keyword;
        metricData.tagMap["uri"] = result.uri;
        metricData.valueMap["metricValue"] = 0;
        metricData.valueMap["code"] = result.code;
#ifndef ENABLE_CLOUD_MONITOR
        metricData.valueMap["bodyLen"] = result.bodyLen;
#endif
        using namespace std::chrono;
        metricData.valueMap["latency"] = duration_cast<fraction_millis>(result.latency).count();
    }

    void DetectSchedule::GetStatus(CommonMetric &metric) {
        metric.value = (mSkipCount > 0 ? 1 : 0);
        metric.name = "detect_status";
        metric.timestamp = NowSeconds();
        metric.tagMap["number"] = StringUtils::NumberToString(mTaskResult.Size());
        metric.tagMap["skip"] = StringUtils::NumberToString(mSkipCount);
        mSkipCount = 0;
    }

    bool DetectSchedule::ClearTask(const string &taskId) {
        //clear TaskResult
        mTaskResult.Erase(taskId);

        //clear TaskStatusMap;
        mTaskStatusMap.Erase(taskId);

        //clear mPingDetectMap
        mPingDetectMap.Erase(taskId);
        CleanPingUnitTask(taskId);

        return true;
    }

    bool DetectSchedule::IsTaskRunning(const string &taskId) {
        bool isRunning = false;
        // LockGuard lockGuard(mTaskStatusLock);
        mTaskStatusMap.MustSet(taskId, true, &isRunning);
        return isRunning;

        // if (mTaskStatusMap.find(taskId) == mTaskStatusMap.end()) {
        //     mTaskStatusMap[taskId] = true;
        //     isRunning = false;
        // } else {
        //     if (mTaskStatusMap[taskId]) {
        //         isRunning = true;
        //     } else {
        //         mTaskStatusMap[taskId] = true;
        //         isRunning = false;
        //     }
        // }
        // return isRunning;
    }
}