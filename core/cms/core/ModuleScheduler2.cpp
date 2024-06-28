#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif

#include "ModuleScheduler2.h"
#include "argus_manager.h"
#include "module.h"

#include <chrono>

#include "common/Config.h"
#include "common/Logger.h"
#include "common/SequenceCreator.h"
#include "common/StringUtils.h"
#ifdef ONE_AGENT
#include "cms/common/ThreadPool.h"
#else
#include "common/ThreadPool.h"
#endif
#include "common/TimeProfile.h"
#include "common/Common.h"   // typeName, joinOutput
#include "common/CommonMetric.h"
#include "common/ScopeGuard.h"

using namespace std;
using namespace std::chrono;
using namespace common;

namespace argus {
	// MSVC 2017下父类必须“显式”初始化，否则运行时，父类成员未初始化。真是活见鬼了。
    ModuleScheduler2::ModuleScheduler2(bool enableThreadPool):SchedulerT<ModuleItem, ModuleScheduler2State>() {
        // hasModuleShouldExit = false;
        // apr_pool_create(&(this->modulePool), nullptr);
        Config *cfg = SingletonConfig::Instance();
        // // 配置文件中缺省为500毫秒
        // int64_t scheduleInterval = 0;
        // cfg->GetValue("agent.module.schedule.interval", scheduleInterval, 1000 * 1000);
        // if (scheduleInterval >= 1000) {
        //     mScheduleInterval = std::chrono::milliseconds(scheduleInterval / 1000);
        // }
        //模块的执行时间和周期之间的关系，如果执行时间超过周期的1/3,并且持续了4次,则将会降频调度；
        cfg->GetValue("agent.module.max.execute.ratio", mMaxModuleExecuteTimeRatio, 3);
        mMaxModuleExecuteTimeRatio = (mMaxModuleExecuteTimeRatio <= 0 ? 3 : mMaxModuleExecuteTimeRatio);

        cfg->GetValue("agent.module.exceed.count", this->mContinueExceedCount, 3);
        if (enableThreadPool) {
            setThreadPool(ThreadPool::Option{}
                                  .min(cfg->getModuleThreadNum())
                                  .max(cfg->getMaxModuleThreadNum())
                                  .name(typeName(this))
                                  .makePool());
        }
    }

    ModuleScheduler2::~ModuleScheduler2() {
        close();
    }

    std::shared_ptr<Module> ModuleScheduler2::makeModule(const ModuleItem &s) const {
        ModuleType moduleType = (!s.isSoType ? MODULE_NOT_SO_TYPE : MODULE_COLLECT_TYPE);
        return std::make_shared<Module>(s.module, s.name, s.moduleArgs.c_str(), moduleType);
    }

    std::shared_ptr<ModuleScheduler2State> ModuleScheduler2::createScheduleItem(const ModuleItem &s) const {
        if (IsZero(s.scheduleInterval)) {
            return nullptr;
        }

        //加载模块
        auto pModule = makeModule(s);
        if (!pModule->Init()) {
            LogWarn("initial module({}) error", s.module);
            // s.moduleInitOk = false;
            return nullptr;
        }

        auto steadyNow = std::chrono::steady_clock::now();
        auto systemNow = std::chrono::system_clock::now();
        // 延迟启动
        auto delay = getHashDuration(s.scheduleInterval, SingletonConfig::Instance()->getScheduleFactor());
        // auto t0 = getHashStartTime(steadyNow, s.scheduleInterval, factor);
        // auto offset = t0 - steadyNow;
        //判断下次执行的时间是否表达式范围之内，如果不在，则需要调整t0
        if (!s.isTimeEffective(systemNow + delay)) {
            delay = decltype(delay){};  // 复位
        }
        auto t0 = steadyNow + delay;

        auto ss = std::make_shared<ModuleScheduler2State>();
        ss->item = s;
        // ss->moduleItem = s;
        // ss->mid = s.mid;
        // ss->lastBegin = (systemNow + delay) - s.scheduleInterval;
        ss->nextTime = t0;
        // ss->lastFinish = t0;
        ss->p_module = pModule;
        // ss->name = s.name;
        // ss->moduleVersion = s.moduleVersion;
        // ss->moduleArgs = s.moduleArgs;
        // ss->outputVector = s.outputVector;
        ss->maxExecDuration = s.scheduleInterval / mMaxModuleExecuteTimeRatio;
        ss->continueExceedTimes = 0;
        ss->exceedSkipTimes = 0;
        //ss->shouldFinish = t0 + apr_time_from_sec(s.timeOut);
        // m_state.Emplace(s.mid, ss);

        LogInfo("first run module <{}>, outputVector: {}, mid: {}, do hash to {:L}, delay: {:.3f}ms, interval={}",
                s.name, joinOutput(ss->item.outputVector), s.mid, systemNow + delay,
                std::chrono::duration_cast<std::chrono::fraction_millis>(delay).count(),
                s.scheduleInterval);
        return ss;
    }

    void ModuleScheduler2::afterScheduleOnce(ModuleScheduler2State &state,
                                             const std::chrono::milliseconds &millis) const {
        // state.lastFinish = now;
        state.lastExecDuration = millis;
        // 判断是否执行超时，如连续超时则暂停一段时间
        if (state.maxExecDuration.count() > 0 && state.lastExecDuration > state.maxExecDuration) {
            state.continueExceedTimes++;
            LogInfo("module({})[{}]:lastExecuteTime({}) exceeds maxExecuteTime({}),continueExceedTimes:{}",
                    state.getName(), state.runTimes, state.lastExecDuration, state.maxExecDuration,
                    state.continueExceedTimes);
            // 连续执行时间超限，则降频
            if (state.continueExceedTimes < this->mContinueExceedCount) {
                // 此处只记录状态，实际是否抑制，由子类决定
                return;
            }
            state.exceedSkipTimes = static_cast<int>(state.lastExecDuration / state.maxExecDuration) + 1;
        }
        state.continueExceedTimes = 0;
    }

    int ModuleScheduler2::scheduleOnce(ModuleScheduler2State &state) {
        using namespace std::chrono;

        // 判断是否在『预期』的采集时间内
        if (!state.item.isNowEffective()) {
            return -2;
        }

        // 是否还在暂停期间
        if (state.exceedSkipTimes > 0) {
            LogInfo("module({}) will skip one time as lastExecuteTime({}) exceeds maxExecuteTime({}), index: {}",
                    state.item.name, state.lastExecDuration, state.maxExecDuration, state.exceedSkipTimes);
            state.exceedSkipTimes--;
            state.skipCount++;
            return -3;
        }

        state.runTimes++;

        auto start = system_clock::now();
        TimeProfile tp;
        defer(afterScheduleOnce(state, tp.cost()));

        char *buf = nullptr;
        defer(state.p_module->FreeCollectBuf(buf));
        int collectLen = state.p_module->Collect(buf);
        // auto now = system_clock::now();
        // auto millis = duration_cast<milliseconds>(now - start).count();
        auto millis = tp.millis(false);
        Log((millis > 100 ? LEVEL_INFO : LEVEL_DEBUG), "collect module({})[{}] spend {} ms",
            state.item.name, state.runTimes, convert(millis, true));

        if (collectLen < 0) {
            LogWarn("module({})[{}] collect error,ret={}", state.item.name, state.runTimes, collectLen);
            state.errorCount++;
            // state.lastError = now;
            // if (collectLen == -2) {
            //     //调整模块数据采集最大值
            //     // maxOutputLen += maxOutputLen;
            // }
        } else if (collectLen == 0) {
            //没有采集到数据
            LogDebug("module({})[{}] collect ok,but no result", state.item.name, state.runTimes);
        } else {
            //保证时间戳严格对齐
            // int64_t timestamp = ToSeconds(state.lastBegin);
            LogDebug("module({})[{}] collect ok, result={}", state.item.name, state.runTimes, buf);
            if (auto pManager = SingletonArgusManager::Instance()->getChannelManager()) {
                const auto &item = state.item;
                pManager->sendMsgToNextModule(item.name, item.outputVector, start, 0, buf, false, item.mid);
            }
        }

        return 0;
    }

    boost::json::value ModuleScheduler2::GetStatus(const std::string &mid) const {
        Module2StateMap stateMap;
        {
            std::unique_lock<std::mutex> lock(this->mutex_);
            if (mid.empty()) {
                stateMap = m_state;
            } else {
                auto it = m_state.find(mid);
                if (it != m_state.end()) {
                    stateMap[mid] = it->second;
                }
            }
        }

        boost::json::array array;
        array.reserve(stateMap.size());
        for (const auto &it: stateMap) {
            array.push_back(boost::json::value{
                    {"MID",             it.first},
                    {"RunTimes",        it.second->runTimes},
                    {"ErrorTimes",      it.second->errorCount},
                    // {"LastBegin",       ToMicros(it.second->lastBegin)},
                    // {"LastFinish",      ToMicros(it.second->lastFinish)},
                    // {"LastError",       ToMicros(it.second->lastError)},
                    {"SkipTimes",       it.second->skipCount},
                    {"LastExecuteTime", it.second->lastExecDuration.count()},
            });
        }
        return boost::json::value{boost::json::object{
                {"ITEM_NUM",            stateMap.size()},
                {"ModuleScheduleState", array},
        }};
    }
}
