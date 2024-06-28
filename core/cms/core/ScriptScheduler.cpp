#if defined(WIN32)
#include <winsock2.h>
#include <ws2tcpip.h> // 解决: 1) apr_sockaddr_t::sin6 使用未定义的struct sockaddr_in6。 2） LPMSG
#endif
#include "ScriptScheduler.h"
#include "SchedulerStat.h"
#include "argus_manager.h"
#include "exporter_metric.h"
#include "ali_script_metric.h"
#include "ChannelManager.h"

#include "common/Common.h"
#include "common/Logger.h"
#include "common/ProcessWorker.h"
#include "common/SequenceCreator.h"
#include "common/ErrorCode.h"
#include "common/StringUtils.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/CommonKeys.h"
#include "common/PrometheusMetric.h"
#include "common/Config.h"

using namespace std;
using namespace common;

#define MAX_OUTPUT_BUF (64*1024)

namespace argus
{
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ScriptScheduler
ScriptScheduler::ScriptScheduler(const std::function<std::shared_ptr<common::ProcessWorker>()> &f) {
    createProcessWorker = (f ? f : []() { return std::make_shared<ProcessWorker>(); });
}

ScriptScheduler::~ScriptScheduler()
{
    endThread();
    join();

    // m_stateLock.lock();
    // for (auto &it: m_state) {
    //     delete it.second;
    // }
    m_state.Clear();
    // m_stateLock.unlock();
}

void ScriptScheduler::onTimer() {
    checkForEnd();

    for (auto & m_item : m_items) {
        schedule(m_item.second);
    }

    checkForEnd();

    //clear state data
    if (m_state.Size() > m_items.size()) {
        // std::lock_guard<decltype(m_stateLock)> guard(m_stateLock);
        Sync(m_state) {
            for (auto it = m_state.begin(); it != m_state.end();) {
                if (!it->second->isRunning && m_items.find(it->first) == m_items.end()) {
                    // delete it->second;
                    m_state.erase(it++);
                } else {
                    ++it;
                }
            }
        }}}
        // m_stateLock.unlock();
    }
}

void ScriptScheduler::doRun()
{
    TaskManager *pTask = SingletonTaskManager::Instance();
    Config *cfg = SingletonConfig::Instance();

    std::shared_ptr<std::map<std::string, ScriptItem>> scriptItems;
    std::shared_ptr<std::map<std::string, ScriptItem>> aliScriptItems;
    for (; !isThreadEnd(); sleepFor(cfg->getScheduleIntv())) {
        CpuProfile("ScriptScheduler::doRun");

        bool isChanged = pTask->ScriptItems().Get(scriptItems);
        if (isChanged) {
            LogDebug("ScriptItems changed: {}", scriptItems->size());
        }
        // bool isChanged = false;
        // if (pTask->ScriptItemChanged())
        // {
        //     m_scriptItems.clear();
        //     pTask->GetScriptItems(m_scriptItems, true);
        //     isChanged = true;
        //     LogDebug("ScriptItems changed:{}",m_scriptItems.size());
        // }
        if (pTask->AliScriptItems().Get(aliScriptItems)) {
            isChanged = true;
            LogDebug("AliScriptItems changed:{}", aliScriptItems->size());
        }
        // if (pTask->AliScriptItemChanged())
        // {
        //     m_aliScriptItems.clear();
        //     pTask->GetAliScriptItems(m_aliScriptItems, true);
        //     isChanged = true;
        //     LogDebug("AliScriptItems changed:{}",m_aliScriptItems.size());
        // }
        if (isChanged) {
            m_items = *scriptItems;
            m_items.insert(aliScriptItems->begin(), aliScriptItems->end());
            if (m_items.size() != scriptItems->size() + aliScriptItems->size()) {
                LogWarn("m_scriptItems and m_aliScriptItems conflicts! Some aliScript items may be discarded!");
            }
        }

        onTimer();
    }
}

int ScriptScheduler::schedule(const ScriptItem &s)
{
    int ret = 0;
    std::shared_ptr<ScriptScheduleState> item;
    if (m_state.Find(s.mid, item)) {
        if (!item->isRunning) {
            ret = checkRun(s, item);
        }
    } else {
        ret = checkFirstRun(s);
    }

    return ret;
}

int ScriptScheduler::checkFirstRun(const ScriptItem &s)
{
    auto nowTime = std::chrono::system_clock::now();
    if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(nowTime)) {
        return -1;
    }
    std::chrono::system_clock::time_point t0;
    //判断是否设置首次调度时间
    if (s.firstSche.time_since_epoch().count() > 0) {
        //首次调度时间小于当前时间
        if (nowTime <= s.firstSche) {
            t0 = s.firstSche - s.scheduleIntv;
        } else {
            // int64_t intvNum = (nowSeconds - s.firstSche) / s.scheduleIntv;
            int64_t diff = std::chrono::duration_cast<std::chrono::seconds>(nowTime - s.firstSche).count();
            t0 = s.firstSche + std::chrono::seconds{diff - diff % s.scheduleIntv.count()}; // ) *1000*1000;
        }
        LogInfo("set t0 with the first sche=\"{:L}\"", t0);
    } else {
        auto factor = s.duration;
        if (factor.count() <= 0) {
            factor = SingletonConfig::Instance()->getScheduleFactor();
        }
        t0 = getHashStartTime(nowTime, s.scheduleIntv, factor) - s.scheduleIntv;
        int hashTimes = 0;
        while (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(t0 + s.scheduleIntv) && hashTimes < 100) {
            t0 = getHashStartTime(nowTime, s.scheduleIntv, factor) - s.scheduleIntv;
            hashTimes++;
        }
        LogInfo("set t0 with random time t0=\"{:L}\", hashTimes={}", t0, hashTimes);
    }
    if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(t0 + s.scheduleIntv)) {
        t0 = nowTime - s.scheduleIntv;
    }
    auto ss = std::make_shared<ScriptScheduleState>();
    ss->mid = s.mid;
    ss->name = s.name;
    ss->lastBegin = t0;
    ss->lastFinish = t0;
    ss->shouldFinish = t0 + s.timeOut;
    ss->outputVector = s.outputVector;
    ss->reportStatus = s.reportStatus;
    ss->labelAddInfos = s.labelAddInfos;
    ss->resultFormat = s.resultFormat;
    ss->isAliScript = s.isAliScript;
    ss->metricMetas = s.metricMetas;
    ss->metricFilterInfos = s.metricFilterInfos;
    // m_stateLock.lock();
    // m_state[s.mid] = ss;
    // m_stateLock.unlock();
    m_state.Set(s.mid, ss);
    LogDebug("first run mi: {}, do hash to {:L}", s.mid, t0);
    return 0;
}

int ScriptScheduler::checkRun(const ScriptItem &s, const std::shared_ptr<ScriptScheduleState> &state) {
    auto nowTime = std::chrono::system_clock::now();
    if (nowTime < state->lastBegin) {
        return -1;
    }
    const auto &t1 = s.scheduleIntv;
    auto t2 = nowTime - state->lastBegin;
    if (-1 * t1 < t2 && t2 < t1) {
        return -2;
    }

    if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(nowTime)) {
        return -3;
    }

    //should run
    if (checkSlot()) {
        runItem(s, state, nowTime);
    }
    return 0;
}

int ScriptScheduler::runItem(const ScriptItem &s,
                             const std::shared_ptr<ScriptScheduleState> &state,
                             const std::chrono::system_clock::time_point &now) {
    LogDebug("run mi:{}", s.mid);
    int64_t n = 0;
    if (now > state->lastBegin) {
        n = (now - state->lastBegin) / s.scheduleIntv;
        state->lastBegin += n * s.scheduleIntv;
    } else {
        n = (state->lastBegin - now) / s.scheduleIntv;
        state->lastBegin -= n * s.scheduleIntv;
    }
    if (n <= 0) return -2;
    if (n > 1) {
        state->skipTimes++;
        state->skipCount++;
        //correct lastbegin
        if (s.scheduleExpr != QUARTZ_ALL && !s.timePeriod.in(state->lastBegin)) {
            //check next, if next not in, will change lastbegin
            auto nextBegin = state->lastBegin + s.scheduleIntv;
            if (!s.timePeriod.in(nextBegin)) {
                state->lastBegin = now - s.scheduleIntv;
            }
            state->lastFinish = state->lastBegin;
            state->shouldFinish = state->lastBegin + s.timeOut;
            LogInfo("mid:{}, skip {} data", s.mid.c_str(), n);
            return -3;
        } else
            LogWarn("mid:{}, data loss {}", s.mid.c_str(), n - 1);
    }

    //add usedTime and delayTime
    #if 0
    // DebugTimeInfo ti;
    // ti.usedTime = apr_time_from_sec(s.timeOut) + state->lastFinish - state->shouldFinish;
    // state->delayTime = (n - 1) * apr_time_from_sec(s.scheduleIntv);
    // if (ti.usedTime < 0 || ti.usedTime > apr_time_from_sec(600))
    //     ti.usedTime = 0;
    // if (state->delayTime > 0 && state->delayTime < apr_time_from_sec(600))
    //     ti.delayTime = state->delayTime;
    // SingletonAgentInfo::Instance()->pushScriptTimeInfo(s.mid, ti);
    #endif

    //change state
    state->shouldFinish = now + s.timeOut;
    state->runTimes++;
    state->startTime = now;
    auto pw = createProcessWorker(); // std::make_shared<ProcessWorker>();
    // Config *cfg = SingletonConfig::Instance();
    string prog = s.collectUrl;
    prog = replaceMacro(prog);
    prog = enableString(prog);

    // char **args;
    // apr_pool_t *p;
    // apr_pool_create(&p, NULL);
    // apr_tokenize_to_argv(prog.c_str(), &args, p);
    std::vector<std::string> cache;
    std::vector<const char *> args;
    StringUtils::ParseCmdLine(prog, cache, args, true);
    //int res = pw->create(args[0], args, NULL, s.scriptUser.c_str(), NULL, NULL, true);
    int res = pw->create2(args[0], &args[0], s.scriptUser.c_str(), 0, true);
    // apr_pool_destroy(p);
    if (res == 0) {
        state->isRunning = true;
        state->output.clear();
        m_running[s.mid] = pw;
        return 0;
    } else {
        LogWarn("create process error, cmd({}):{}", prog.c_str(), getErrorString(res).c_str());
        // delete pw;
        state->lastFinish = state->lastBegin;
        state->lastError = state->lastBegin;
        state->errorTimes++;
        state->errorCount++;
        //create error msg
        int errCode = res;
        string errMsg = getErrorString(res);
        if (state->resultFormat != RAW_FORMAT) {
            errMsg = BuildExecuteResult(state->name, errMsg, errCode, -1);
        }
        outputResult(state, errCode, errMsg, string(""));
        return -1;
    }
}

// int ScriptScheduler::waitForSlot()
// {
//     Config *cfg = SingletonConfig::Instance();
//     while ((int)m_running.size() >= cfg->getMaxProcNum())
//     {
//         checkForEnd();
//         std::this_thread::sleep_for(10_ms); // std::chrono::microseconds{10000});
//     }
//     return 0;
// }

bool ScriptScheduler::checkSlot() const {
    return (int) m_running.size() < SingletonConfig::Instance()->getMaxProcNum();
}

int ScriptScheduler::checkForEnd()
{
    recycleKilled();

    std::vector<char> m_buf((size_t) MAX_OUTPUT_BUF);

    // map<string, ProcessWorker*>::iterator it1, it2;
    // ScriptStateMap::iterator it3;
    auto it1 = m_running.begin();
    auto it2 = m_running.end();
    size_t maxOutputLen = SingletonConfig::Instance()->getMaxOutputLen();
    while (it1 != it2)
    {
        string mid = it1->first;
        std::shared_ptr<ProcessWorker> pw = it1->second;

        std::shared_ptr<ScriptScheduleState> item;
        // auto it3 = m_state.find(mid);
        // if (it3 == m_state.end())
        if (!m_state.Find(mid, item))
        {
            LogError("program error, mid: {}", mid);
            it1++;
            continue;
        }

        auto t = std::chrono::system_clock::now();
        if (pw->wait(ProcessWorker::NOWAIT)) // finished
        {
            //get output
            size_t len;
            while (0 != (len = pw->output(&m_buf[0], m_buf.size() - 1))) {
                m_buf[len] = '\0';
                item->output += &m_buf[0];
            }
            //if stdout empty, read stderr
            if (item->output.empty()) {
                while (0 != (len = pw->errput(&m_buf[0], m_buf.size() - 1))) {
                    m_buf[len] = '\0';
                    item->output += &m_buf[0];
                }
            }

            if (item->output.size() > maxOutputLen) {
                LogWarn("cmd({}) output too long({} > {})", mid, item->output.size(), maxOutputLen);
                int errCode = E_OutputTooLong;
                string errMsg = "output too long";
                if (item->resultFormat != RAW_FORMAT) {
                    errMsg = BuildExecuteResult(item->name, errMsg, errCode, -1);
                }
                outputResult(item, errCode, errMsg, string(""));
            } else {
                LogDebug("mi:{}, out: {}", mid, item->output);
                int exitCode = pw->getExitCode();
                string msg;
                if (item->resultFormat != RAW_FORMAT) {
                    msg = BuildExecuteResult(item->name, "ok", exitCode, 0);
                    item->output += "\n";
                }
                outputResult(item, exitCode, msg, item->output);
            }

            item->lastFinish = t;
            item->isRunning = false;
            item->output.clear();
            // delete pw;
            m_running.erase(it1++);
            continue;
        }
        else
        {
            bool failed = false;
            size_t len = pw->output(&m_buf[0], m_buf.size() - 1);
            m_buf[len] = '\0';
            item->output += &m_buf[0];
            if (item->output.size() > maxOutputLen) {
                LogWarn("cmd({}) output too long({} > {})", mid, item->output.size(), maxOutputLen);
                failed = true;
            }

            if (item->shouldFinish <= t || failed)
            {
                pw->kill(9);
                std::this_thread::sleep_for(std::chrono::microseconds{1});
                if (!pw->wait(ProcessWorker::NOWAIT)) {
                    // still not returned, put it to the killed process list
                    m_killed.push_back(pw);
                }

                item->lastFinish = t;
                item->lastError = t;
                item->errorTimes++;
                item->errorCount++;
                item->isRunning = false;
                item->output.clear();
                if (!failed) {
                    LogWarn("mid:{}, run timeout", mid.c_str());
                    int errCode = E_TimeOut;
                    string errMsg = "cmd run timeout";
                    if (item->resultFormat != RAW_FORMAT) {
                        errMsg = BuildExecuteResult(item->name, errMsg, errCode, -1);
                    }
                    outputResult(item, errCode, errMsg, string(""));
                } else {
                    int errCode = E_OutputTooLong;
                    string errMsg = "output too long";
                    if (item->resultFormat != RAW_FORMAT) {
                        errMsg = BuildExecuteResult(item->name, errMsg, errCode, -1);
                    }
                    outputResult(item, errCode, errMsg, string(""));
                }
                m_running.erase(it1++);
                continue;
            }
        }
        it1++;
    }

    return 0;
}

int ScriptScheduler::recycleKilled()
{
    auto ptr = m_killed.begin();
    while (ptr != m_killed.end())
    {
        if ((*ptr)->wait(ProcessWorker::NOWAIT))
        {
            // delete *ptr;
            ptr = m_killed.erase(ptr);
            continue;
        }
        ++ptr;
    }
    return 0;
}

int ScriptScheduler::getStateMap(std::map<std::string, std::shared_ptr<ScriptScheduleState>> &stateMap) const {
    // m_stateLock.lock();
    // for (const auto & it : m_state) {
    //     stateMap[it.first] = it.second;
    // }
    // m_stateLock.unlock();
    Sync(m_state) {
        stateMap = m_state;
    }}}
    return 0;
}

int ScriptScheduler::getStateMap(std::map<std::string, std::shared_ptr<ScriptScheduleState>> &stateMap,
                                 const std::string &mid) const {
    // m_stateLock.lock();
    // const auto it = m_state.find(mid);
    // if (it != m_state.end()) {
    //     stateMap[it->first] = it->second;
    // }
    // m_stateLock.unlock();
    Sync(m_state) {
        const auto it = m_state.find(mid);
        if (it != m_state.end()) {
            stateMap[it->first] = it->second;
        }
    }}}
    return 0;
}
int ScriptScheduler::outputResult(const std::shared_ptr<ScriptScheduleState> &state,int exitCode,string &errMsg, const string &result){
    const auto timestamp = state->startTime; // /(1000*1000);
    auto pManager = SingletonArgusManager::Instance()->getChannelManager();

    auto validate = [&](BaseParseMetric::ParseErrorType ret, std::vector<CommonMetric> &commonMetrics) {
        bool errMsgValid = false;
        for (auto it = commonMetrics.begin(); it != commonMetrics.end(); it++) {
            if (it->name == "__argus_script_status__") {
                it->value = commonMetrics.size() > 1 ? static_cast<double>(commonMetrics.size() - 1) : it->value;
                it->tagMap["__argus_script_parse_msg__"] = BaseParseMetric::ParseErrorString[ret];
                if (state->reportStatus) {
                    if (ret == BaseParseMetric::PARSE_SUCCESS && state->reportStatus == 1) {
                        it->tagMap["__argus_script_raw_msg__"] = "";
                    } else {
                        it->tagMap["__argus_script_raw_msg__"] = result;
                    }
                }
                errMsgValid = true;
                break;
            }
        }
        if (!errMsgValid) {
            LogWarn("scriptTask:{}, output error message:{}, invalid", state->name, errMsg);
        }
    };

    if (state->resultFormat == RAW_FORMAT) {
        bool reportStatus = (state->reportStatus != 0);
        if (pManager) {
            pManager->sendMsgToNextModule(
                    state->name, state->outputVector, timestamp, exitCode, result + errMsg, reportStatus, state->mid);
        }
    } else if (state->resultFormat == RAW_DATA_FORMAT) {
        RawData rawData;
        rawData.type = CommonKeys::SCRIPT_TYPE;
        ResultMap resultMap;
        resultMap.ts = timestamp;
        resultMap.stringMap["result"] = result;
        if (state->reportStatus != 0) {
            resultMap.stringMap["status"] = errMsg;
        }
        rawData.resultMaps.push_back(resultMap);
        BaseParseMetric::ParseAddLabelInfo(state->labelAddInfos, rawData.labelMap, nullptr);
        if (pManager) {
            pManager->SendRawDataToNextModule(rawData, state->outputVector);
        }
    } else if (state->resultFormat == PROMETHEUS_FORMAT) {
        //prometheus格式解析
        vector<MetricFilterInfo> metricFilterInfos;
        ExporterMetric pExporterMetric(metricFilterInfos, state->labelAddInfos);
        enum BaseParseMetric::ParseErrorType ret = pExporterMetric.AddMetric(result + errMsg);
        if (result.empty()) {
            ret = BaseParseMetric::EMPTY_STRING;
        }
        vector<CommonMetric> commonMetrics;
        pExporterMetric.GetCommonMetrics(commonMetrics);
        validate(ret, commonMetrics);
        // for(auto it = commonMetrics.begin();it != commonMetrics.end();it++)
        // {
        //     if(it->name =="__argus_script_status__")
        //     {
        //         it->value = commonMetrics.size() > 1 ? static_cast<double>(commonMetrics.size() - 1) : it->value;
        //         it->tagMap["__argus_script_parse_msg__"]=BaseParseMetric::ParseErrorString[ret];
        //         if(state->reportStatus){
        //             if(ret==BaseParseMetric::PARSE_SUCCESS && state->reportStatus==1){
        //                 it->tagMap["__argus_script_raw_msg__"]="";
        //             }else{
        //                 it->tagMap["__argus_script_raw_msg__"]=result;
        //             }
        //         }
        //         errMsgValid=true;
        //         break;
        //     }
        // }
        // if(!errMsgValid){
        //     LogWarn("scriptTask:{}, output error message:{}, invalid",state->name,errMsg);
        // }
        if (pManager) {
            pManager->SendMetricsToNextModule(commonMetrics, state->outputVector);
        }
    } else if (state->resultFormat == ALIMONITOR_JSON_FORMAT || state->resultFormat == ALIMONITOR_TEXT_FORMAT) {
        AliScriptMetric scriptMetric(state->metricFilterInfos, state->labelAddInfos, state->metricMetas,
                                     state->resultFormat);
        enum BaseParseMetric::ParseErrorType ret = scriptMetric.AddMetric(result);
        vector<CommonMetric> commonMetrics;
        scriptMetric.GetCommonMetrics(commonMetrics);
        //errMsg单独解析。
        vector<MetricFilterInfo> metricFilterInfos;
        vector<LabelAddInfo> LabelAddInfos = state->labelAddInfos;
        if (state->resultFormat == ALIMONITOR_JSON_FORMAT) {
            vector<LabelAddInfo> jsonLabelAddInfos;
            scriptMetric.GetJsonLabelAddInfos(jsonLabelAddInfos);
            LabelAddInfos.insert(LabelAddInfos.end(), jsonLabelAddInfos.begin(), jsonLabelAddInfos.end());
        }
        ExporterMetric pExporterMetric(metricFilterInfos, LabelAddInfos);
        pExporterMetric.AddMetric(errMsg);
        pExporterMetric.GetCommonMetrics(commonMetrics);
        validate(ret, commonMetrics);
        // for(auto it = commonMetrics.begin();it != commonMetrics.end();it++)
        // {
        //     if(it->name =="__argus_script_status__")
        //     {
        //         it->value = commonMetrics.size() > 1 ? static_cast<double>(commonMetrics.size() - 1) : it->value;
        //         it->tagMap["__argus_script_parse_msg__"]=BaseParseMetric::ParseErrorString[ret];
        //         if(state->reportStatus){
        //             if(ret==BaseParseMetric::PARSE_SUCCESS && state->reportStatus==1){
        //                 it->tagMap["__argus_script_raw_msg__"]="";
        //             }else{
        //                 it->tagMap["__argus_script_raw_msg__"]=result;
        //             }
        //         }
        //         errMsgValid=true;
        //         break;
        //     }
        // }
        // if(!errMsgValid){
        //     LogWarn("scriptTask:{} output error message:{} invalid",state->name.c_str(),errMsg.c_str());
        // }
        if (pManager) {
            pManager->SendMetricsToNextModule(commonMetrics, state->outputVector);
        }
    } else {
        LogWarn("Invalid result format: {}, will treat it as raw format!", static_cast<int>(state->resultFormat));
        if (pManager) {
            pManager->sendMsgToNextModule(state->name, state->outputVector, timestamp, exitCode, result + errMsg,
                                          state->reportStatus, state->mid);
        }
    }
    return 0;
}
string  ScriptScheduler::BuildExecuteResult(const string& scriptName,string errorMsg,int exitCode,int metricNum)
{
    const char *szPattern = R"XX(__argus_script_status__{{__argus_script_name__="{}",__argus_script_code__="{}",__argus_script_error_msg__="{}"}} {})XX";
    return fmt::format(szPattern, _PromEsc(scriptName), exitCode, _PromEsc(errorMsg), metricNum);
}
void ScriptScheduler::GetStatus(CommonMetric &metric)
{
    Sync(m_state) {
                statStatus("script_status", m_state, metric);
    }}}
    // vector<string> errorTasks;
    // vector<string> okTasks;
    // vector<string> skipTasks;
    // // m_stateLock.lock();
    // size_t totalNumber = 0;
    // Sync(m_state) {
    //     totalNumber = m_state.size();
    //     for (auto &it: m_state) {
    //         if (it.second->errorCount > 0) {
    //             errorTasks.push_back(it.second->name);
    //         }
    //         if (it.second->skipCount > 0) {
    //             skipTasks.push_back(it.second->name);
    //         }
    //         if (it.second->skipCount == 0 && it.second->errorCount == 0) {
    //             okTasks.push_back(it.second->name);
    //         }
    //         it.second->skipCount = 0;
    //         it.second->errorCount = 0;
    //     }
    //     // m_stateLock.unlock();
    // }}}
    //
    // double script_status = 0;
    // if(!errorTasks.empty())
    // {
    //     script_status =1;
    // }
    // metric.value = script_status;
    // metric.name= "script_status";
    // metric.timestamp = NowSeconds();
    // metric.tagMap["number"] = StringUtils::NumberToString(totalNumber);
    // metric.tagMap["ok_list"] = StringUtils::join(okTasks,",");
    // metric.tagMap["error_list"] = StringUtils::join(errorTasks,",");
    // metric.tagMap["skip_list"] = StringUtils::join(skipTasks,",");
}

boost::json::array ScriptScheduler::getInfo(const std::string &opValue) const {
    std::map<std::string, std::shared_ptr<ScriptScheduleState>> stateMap;
    if (opValue.empty()) {
        getStateMap(stateMap);
    } else {
        getStateMap(stateMap, opValue);
    }

    boost::json::array array;
    array.reserve(stateMap.size());
    for (const auto &it: stateMap) {
        array.push_back(boost::json::value{
                {"MID",        it.first},
                {"RunTimes",   it.second->runTimes},
                {"ErrorTimes", it.second->errorTimes},
                {"LastBegin",  ToMicros(it.second->lastBegin)},
                {"LastFinish", ToMicros(it.second->lastFinish)},
                {"LastError",  ToMicros(it.second->lastError)},
                {"DelayTime",  0}, // it.second->delayTime},
        });
    }
    return array;
}

}
