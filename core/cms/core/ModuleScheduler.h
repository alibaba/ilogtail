#if 0
// #ifndef _MODULE_SCHEDULER_H_
// #define _MODULE_SCHEDULER_H_
//
// #include <map>
// #include <string>
// #include <memory>
//
// #include "common/Common.h"
// #include "common/TimePeriod.h"
// #include "common/common_metric.h"
// #include "common/SafeMap.h"
//
// #include "module.h"
// #include "TaskManager.h"
//
// class ThreadPool;
// class ThreadWorkerEx;
//
// namespace argus {
//     struct ModuleSchedulerState : ModuleItem {
//         // std::string mid;
//         long errorTimes = 0;
//         long errorCount = 0;
//         long runTimes = 0;
//         std::chrono::system_clock::time_point lastBegin;
//         std::chrono::system_clock::time_point lastFinish;
//         std::chrono::system_clock::time_point lastError;
//         std::chrono::milliseconds lastExecuteTime{0};
//         std::chrono::system_clock::time_point lastTrueBegin;
//         std::chrono::milliseconds maxExecuteTime{0};
//         int continueExceedTimes = 0;
//         int exceedSkipTimes = 0;
//         long skipTimes = 0;
//         long skipCount = 0;
//         std::string output;
//         std::atomic<bool> isRunning{false};
//         // std::string name;
//         // std::string moduleVersion;
//         // std::string moduleArgs;
//         std::shared_ptr<Module> p_module;
//         // std::vector<std::pair<std::string, std::string>> outputVector;
//     };
//
//     typedef std::map<std::string, std::shared_ptr<ModuleSchedulerState>> ModuleStateMap;
//
// #include "common/test_support"
// class ModuleScheduler {
// public:
//     ModuleScheduler();
//     ~ModuleScheduler();
//     int getStateMap(ModuleStateMap &stateMap) const;
//     int getStateMap(ModuleStateMap &stateMap, const std::string &mid) const;
//     void GetStatus(common::CommonMetric &metric);
//
//     void runIt();
// private:
//     // void doRun();
//     // int schedule(ModuleStateMap &moduleMap, const ModuleItem &s);
//     std::shared_ptr<ModuleSchedulerState> createScheduleItem(const ModuleItem &s) const;
//     void checkRun(const std::shared_ptr<ModuleSchedulerState> &state) const;
//     void runItem(const std::shared_ptr<ModuleSchedulerState> &state,
//                  const std::chrono::system_clock::time_point &now) const;
//
// private:
//     // std::map<std::string, ModuleItemEx> m_items;
//     // 原始配置
//     std::shared_ptr<std::map<std::string, ModuleItem>> mModuleItems;
//     // ModuleStateMap m_state;  //mid -> state
//     // InstanceLock m_stateLock;
//     // 运行时
//     SafeMap<std::string, std::shared_ptr<ModuleSchedulerState>> m_state;
//
//     // apr_pool_t *modulePool = nullptr;
//     // apr_thread_pool_t *threadPool = nullptr;
//     std::shared_ptr<ThreadPool> threadPool;
//     // std::atomic<bool> hasModuleShouldExit{false};
//     std::chrono::milliseconds mScheduleInterval{1000}; // 单位：毫秒
//     int mMaxModuleExecuteTimeRatio = 3;
//     int mContinueExceedCount = 0;
//
//     std::shared_ptr<ThreadWorkerEx> threadWorker;
// };
// #include "common/test_support"
//
// }
//
// #endif

#endif // 0