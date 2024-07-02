#if 0
// #if defined(WIN32)
// #include <winsock2.h>
// #include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
// #endif
// #include "ModuleScheduler.h"
// #include "argus_manager.h"
//
// #include <chrono>
// #include <sstream>
//
// #include "common/Config.h"
// #include "common/Logger.h"
// #include "common/SequenceCreator.h"
// #include "common/StringUtils.h"
// #include "common/ThreadPool.h"
// #include "common/ThreadWorker.h"
// #include "common/ScopeGuard.h"
//
// using namespace std;
// using namespace std::chrono;
// using namespace common;
//
// namespace argus {
//     void scheduleOnce(const std::weak_ptr<ModuleSchedulerState>&);
//
//     class ModuleSchedulerWorker : public ThreadWorkerEx {
//     };
//
//     ModuleScheduler::ModuleScheduler() {
//         threadWorker = std::make_shared<ModuleSchedulerWorker>();
//         // hasModuleShouldExit = false;
//         // apr_pool_create(&(this->modulePool), nullptr);
//         Config *cfg = SingletonConfig::Instance();
//         // 配置文件中缺省为500毫秒
//         int64_t scheduleInterval = 0;
//         cfg->GetValue("agent.module.schedule.interval", scheduleInterval, 1000 * 1000);
//         if (scheduleInterval >= 1000) {
//             mScheduleInterval = std::chrono::milliseconds(scheduleInterval / 1000);
//         }
//         //模块的执行时间和周期之间的关系，如果执行时间超过周期的1/3,并且持续了4次,则将会降频调度；
//         cfg->GetValue("agent.module.max.execute.ratio", mMaxModuleExecuteTimeRatio, 3);
//         mMaxModuleExecuteTimeRatio = (mMaxModuleExecuteTimeRatio <= 0 ? 3 : mMaxModuleExecuteTimeRatio);
//         cfg->GetValue("agent.module.exceed.count", mContinueExceedCount, 3);
//         int collectNum = cfg->getModuleThreadNum();
//         int maxCollectNum = cfg->getMaxModuleThreadNum();
//         // apr_thread_pool_create(&threadPool, collectNum, maxCollectNum, this->modulePool);
//         threadPool = ThreadPool::Option{}
//                 .min(collectNum)
//                 .max(maxCollectNum)
//                 .name("ModuleScheduler")
//                 .makePool();
//     }
//
//     ModuleScheduler::~ModuleScheduler() {
//         threadWorker.reset();
//         // endThread();
//         // join();
//         m_state.Clear();// = std::make_shared<ModuleStateMap>();
//         // m_stateLock.lock();
//         // // for (auto &it: m_state) {
//         // //     delete it.second;
//         // // }
//         // m_state.clear();
//         // m_stateLock.unlock();
//         // apr_thread_pool_destroy(this->threadPool);
//         threadPool.reset();
//         // apr_pool_destroy(this->modulePool);
//     }
//
//     void ModuleScheduler::runIt() {
//         threadWorker->runAsync([this]() {
//             // TaskManager *pTask = SingletonTaskManager::Instance();
//
//             // std::map<std::string, ModuleItemEx> newItems;
//             bool changed = SingletonTaskManager::Instance()->ModuleItems().Get(mModuleItems);
//             // if (changed) {
//             //     // m_items.clear();
//             //     for (const auto &it: *mModuleItems) {
//             //         ModuleItemEx &item = newItems[it.first];
//             //         static_cast<ModuleItem &>(item) = it.second;
//             //         item.moduleInitOk = true;
//             //     }
//             // }
//             // if (pTask->ModuleItemChanged()) {
//             //     m_items.clear();
//             //     pTask->GetModuleItems(m_items, true);
//             //     //一个模块不能设置成多个采集周期，过滤掉
//             //     //filterSameMuduleItems();
//             // }
//             // size_t itemCount = 0;
//             Sync(m_state) {
//                 if (changed) {
//                     ModuleStateMap newState;
//                     for (auto &newItem: *mModuleItems) {
//                         auto old = m_state.find(newItem.first);
//                         if (old == m_state.end()) {
//                             if (auto r = createScheduleItem(newItem.second)) {
//                                 newState[r->mid] = r;
//                             }
//                         } else {
//                             newState.emplace(*old);
//                         }
//                     }
//                     m_state = newState;
//                 }
//
//                 for (auto &scheduleItem: m_state) {
//                     checkRun(scheduleItem.second);
//                 }
//             }}}
//             // LogInfo("shedule complete, changed: {}",
//             //          (m_state->size() > m_items.size() ? "true" : "false"));
//             //clear state data
//             // if (changed || m_state.Size() > itemCount) {
//             //     Sync(m_state) {
//             //         for (auto it = m_state->begin(); it != m_state->end();) {
//             //             if (m_items.find(it->first) == m_items.end()) {
//             //                 m_state->erase(it++);
//             //             } else {
//             //                 ++it;
//             //             }
//             //         }
//             //     }}}
//             // }
//             return mScheduleInterval;
//         });
//     }
//
//     // bool ModuleScheduler::schedule(ModuleStateMap &moduleMap, const ModuleItem &s) {
//     //     auto it = moduleMap.find(s.mid);
//     //     if (it != moduleMap.end()) {
//     //         if (!it->second->isRunning) {
//     //             checkRun(s, it->second);
//     //         }
//     //     } else {
//     //         return checkFirstRun(s);
//     //     }
//     //
//     //     return true;
//     // }
//
//     static std::string join(const std::vector<std::pair<std::string, std::string>> &output) {
//         std::stringstream ss;
//         ss << "[";
//         const char *sep = "";
//         for (const auto &pair: output) {
//             ss << sep << "{\"" << pair.first << "\":\"" << pair.second << "\"}";
//             sep = ",";
//         }
//         ss << "]";
//         return ss.str();
//     }
//
//     std::shared_ptr<ModuleSchedulerState> ModuleScheduler::createScheduleItem(const ModuleItem &s) const {
//         //判断模块是否已经在m_state中，如果在，则需要老的模块退出之后，新的才能运行
//         /*if(moduleIsRunning(s))
//         {
//             return 0;
//         }*/
//         // //判断时机是否正确
//         auto nowTime = std::chrono::system_clock::now();
//         // if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(nowTime)) {
//         //     return false;
//         // }
//         //加载模块
//         ModuleType moduleType = (!s.isSoType ? MODULE_NOT_SO_TYPE : MODULE_COLLECT_TYPE);
//         auto p_module = std::make_shared<Module>(s.module, s.name, s.moduleArgs.c_str(), moduleType);
//         if (!p_module->Init()) {
//             LogWarn("initial module({}) error", s.module);
//             // s.moduleInitOk = false;
//             return nullptr;
//         }
//
//         //first run, do hash
//         auto factor = SingletonConfig::Instance()->getScheduleFactor();
//         system_clock::time_point t0 = getHashStartTime(nowTime, s.scheduleInterval, factor) - s.scheduleInterval;
//         //判断下次执行的时间是否表达式范围之内，如果不在，则需要调整t0
//         if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(t0 + s.scheduleInterval)) {
//             t0 = nowTime - s.scheduleInterval;
//         }
//         auto ss = std::make_shared<ModuleSchedulerState>();
//         static_cast<ModuleItem &>(*ss) = s;
//         // ss->moduleItem = s;
//         // ss->mid = s.mid;
//         ss->lastBegin = t0;
//         ss->lastFinish = t0;
//         ss->p_module = p_module;
//         // ss->name = s.name;
//         // ss->moduleVersion = s.moduleVersion;
//         // ss->moduleArgs = s.moduleArgs;
//         // ss->outputVector = s.outputVector;
//         ss->maxExecuteTime = s.scheduleInterval / mMaxModuleExecuteTimeRatio;
//         ss->continueExceedTimes = 0;
//         ss->exceedSkipTimes = 0;
//         ss->isRunning = false;
//         //ss->shouldFinish = t0 + apr_time_from_sec(s.timeOut);
//         // m_state.Emplace(s.mid, ss);
//
//         LogInfo("first run module <{}>, outputVector: {}, mid: {}, do hash to {:L}, interval={}",
//                 s.name, join(ss->outputVector), s.mid, t0, s.scheduleInterval);
//         return ss;
//     }
//
//     void ModuleScheduler::checkRun(const std::shared_ptr<ModuleSchedulerState> &state) const {
//         if (!state->isRunning) {
//             auto nowTime = system_clock::now();
//
//             const auto &s = *state;
//             auto const &t1 = s.scheduleInterval;
//             auto t2 = nowTime - state->lastBegin;
//             if (-1 * t1 < t2 && t2 < t1) {
//                 return;
//             }
//             if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(nowTime)) {
//                 return;
//             }
//             runItem(state, nowTime);
//         }
//     }
//
//     void ModuleScheduler::runItem(const std::shared_ptr<ModuleSchedulerState> &state,
//                                   const system_clock::time_point &now) const {
//         const ModuleItem &s = *state;
//         LogDebug("run module({}),mi:{}", s.name, s.mid);
//         int64_t n;
//         //计算上次执行所花费的时间
//         if (now > state->lastBegin) {
//             n = (now - state->lastBegin) / s.scheduleInterval;
//             state->lastBegin += n * s.scheduleInterval;
//         } else {
//             n = (state->lastBegin - now) / s.scheduleInterval;
//             state->lastBegin -= n * s.scheduleInterval;
//         }
//         if (n <= 0) {
//             return;
//         }
//         if (n > 1) {
//             state->skipTimes++;
//             state->skipCount++;
//             //correct lastbegin
//             if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(state->lastBegin)) {
//                 //check next, if next not in, will change lastbegin
//                 auto nextbegin = state->lastBegin + s.scheduleInterval;
//                 if (!s.timePeriod.in(nextbegin)) {
//                     state->lastBegin = now - s.scheduleInterval;
//                 }
//                 state->lastFinish = state->lastBegin;
//                 //state->shouldFinish = state->lastBegin + apr_time_from_sec(s.timeOut);
//                 LogInfo("module({}),mid:{}, skip {} data", s.name, s.mid, n);
//                 return;
//             } else {
//                 LogWarn("module({}),mid:{}, data loss {}", s.name, s.mid, n - 1);
//             }
//         }
//         //add usedTime and delayTime
//         /*
//         DebugTimeInfo ti;
//         ti.usedTime = apr_time_from_sec(s.timeOut) + state->lastFinish - state->shouldFinish;
//         state->delayTime = (n - 1) * apr_time_from_sec(s.scheduleIntv);
//         if (ti.usedTime < 0 || ti.usedTime > apr_time_from_sec(600))
//             ti.usedTime = 0;
//         if (state->delayTime > 0 && state->delayTime < apr_time_from_sec(600))
//             ti.delayTime = state->delayTime;
//         SingletonAgentInfo::Instance()->pushScriptTimeInfo(s.mid, ti);
//         */
//
//         //change state
//         //state->shouldFinish = now + apr_time_from_sec(s.timeOut);
//         state->lastTrueBegin = now;
//         state->runTimes++;
//         if (state->continueExceedTimes >= mContinueExceedCount) {
//             state->exceedSkipTimes = static_cast<int>(state->lastExecuteTime / state->maxExecuteTime) + 1;
//             state->continueExceedTimes = 0;
//         }
//         if (state->exceedSkipTimes > 0) {
//             LogInfo("module({}) will skip one time as lastExecuteTime({}) exceeds maxExecuteTime({}),index: {}",
//                          state->name, state->lastExecuteTime, state->maxExecuteTime, state->exceedSkipTimes);
//             state->exceedSkipTimes--;
//             state->lastFinish = now;
//             state->skipTimes++;
//             state->skipCount++;
//             return;
//         }
//         // apr_thread_pool_push(this->threadPool, threadCollectFunc, (void *) state, APR_THREAD_TASK_PRIORITY_NORMAL,
//         //                      (void *) nullptr);
//         state->isRunning = true;
//         std::weak_ptr<ModuleSchedulerState> weak = state;
//         const bool valid = threadPool->commitTimeout(state->name, 1_s, [weak]() { scheduleOnce(weak); }).valid();
//         if (!valid) {
//             state->isRunning = false;
//         }
//         Log((valid? LEVEL_DEBUG: LEVEL_WARN), "threadPool->commit({}) => {}", state->name, valid);
//     }
//
//     void scheduleOnce(const std::weak_ptr<ModuleSchedulerState> &weak) {
//         using namespace std::chrono;
//
//         auto state = weak.lock();
//         defer(if (state) state->isRunning = false;);
//         if (state == nullptr || state->p_module == nullptr) {
//             LogError("Error module({}) in collect", (state == nullptr? "unknown": state->name));
//             return;
//         }
//
//         // static int maxOutputLen = SingletonConfig::Instance()->getMaxModuleOutputLen();
//         // std::vector<char> buf(maxOutputLen, 0);
//         auto start = system_clock::now();
//         char *buf = nullptr;
//         defer(state->p_module->FreeCollectBuf(buf));
//         int collectLen = state->p_module->Collect(buf);
//         auto now = system_clock::now();
//         auto millis = duration_cast<milliseconds>(now - start).count();
//         Log((millis > 100 ? LEVEL_INFO : LEVEL_DEBUG), "collect module({}) spend {} ms", state->name, convert(millis, true));
//
//         // auto nowMicros = duration_cast<microseconds>(now.time_since_epoch());
//         if (collectLen < 0) {
//             //打包
//             LogWarn("module({}) collect error,ret={}", state->name, collectLen);
//             state->errorTimes++;
//             state->errorCount++;
//             state->lastError = now;
//             // if (collectLen == -2) {
//             //     //调整模块数据采集最大值
//             //     // maxOutputLen += maxOutputLen;
//             // }
//         } else if (collectLen == 0) {
//             //没有采集到数据
//             LogDebug("module({}) collect ok,but no result", state->name);
//         } else {
//             //保证时间戳严格对齐
//             // int64_t timestamp = ToSeconds(state->lastBegin);
//             LogDebug("module({})collect ok,result={}", state->name, buf);
//             if (auto pManager = SingletonArgusManager::Instance()->getChannelManager()) {
// 				pManager->sendMsgToNextModule(state->name, state->outputVector, state->lastBegin,
//                                               0, buf, false, state->mid);
//             }
//         }
//         state->lastFinish = now;
//         state->lastExecuteTime = duration_cast<milliseconds>(now - state->lastTrueBegin);
//         if (state->lastExecuteTime > state->maxExecuteTime) {
//             state->continueExceedTimes++;
//             LogInfo("module({}):lastExecuteTime({}) exceeds maxExecuteTime({}),continueExceedTimes:{}",
//                     state->name, state->lastExecuteTime, state->maxExecuteTime, state->continueExceedTimes);
//         } else {
//             state->continueExceedTimes = 0;
//         }
//     }
//
//     // void *APR_THREAD_FUNC threadCollectFunc(apr_thread_t *, void *para) {
//     //     SetupThread();
//     //     auto state = (ModuleSchedulerState *) (para);
//     //     if (state != nullptr) {
//     //         defer(state->isRunning = false);
//
//     //         safeRun([&]() { scheduleOnce(state); });
//     //     }
//     //     return nullptr;
//     // }
//
//     int ModuleScheduler::getStateMap(ModuleStateMap &stateMap) const {
//         stateMap = m_state.Get();
//         return 0;
//     }
//
//     int ModuleScheduler::getStateMap(ModuleStateMap &stateMap, const std::string &mid) const {
//         std::shared_ptr<ModuleSchedulerState> v;
//         if (m_state.Find(mid, v)) {
//             stateMap[v->mid] = v;
//         }
//         return 0;
//     }
//
//     void ModuleScheduler::GetStatus(CommonMetric &metric) {
//         vector<string> errorTasks;
//         vector<string> okTasks;
//         vector<string> skipTasks;
//         size_t totalNumber = 0;
//         Sync(m_state) {
//             totalNumber = m_state.size();
//             for (auto &it: m_state) {
//                 if (it.second->errorCount > 0) {
//                     errorTasks.push_back(it.second->name);
//                 }
//                 if (it.second->skipCount > 0) {
//                     skipTasks.push_back(it.second->name);
//                 }
//                 if (it.second->skipCount == 0 && it.second->errorCount == 0) {
//                     okTasks.push_back(it.second->name);
//                 }
//                 it.second->skipCount = 0;
//                 it.second->errorCount = 0;
//             }
//         }}}
//
//         double module_status = 0;
//         if (!errorTasks.empty() || !skipTasks.empty()) {
//             module_status = 1;
//         }
//         metric.value = module_status;
//         metric.name = "module_status";
//         metric.timestamp = NowSeconds(); // apr_time_now() / (1000 * 1000);
//         metric.tagMap["number"] = StringUtils::NumberToString(totalNumber);
//         metric.tagMap["ok_list"] = StringUtils::join(okTasks, ",");
//         metric.tagMap["error_list"] = StringUtils::join(errorTasks, ",");
//         metric.tagMap["skip_list"] = StringUtils::join(skipTasks, ",");
//     }
// }

#endif // 0
