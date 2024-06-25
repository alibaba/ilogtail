#ifndef _MODULE_SCRIPT_SCHEDULER_H_
#define _MODULE_SCRIPT_SCHEDULER_H_

#include <map>
#include <list>
#include <chrono>
#include <functional>

#include "common/ThreadWorker.h"
#include "common/TimePeriod.h"
#include "common/SafeMap.h"

#include "TaskManager.h"

namespace common {
    class ProcessWorker;

    struct CommonMetric;
}
namespace argus {
    struct ScriptScheduleState {
        std::string mid;
        long errorTimes = 0;
        long errorCount = 0;
        long runTimes = 0;
        long skipTimes = 0;
        long skipCount = 0;
        std::chrono::system_clock::time_point lastBegin;
        std::chrono::system_clock::time_point lastFinish;
        std::chrono::system_clock::time_point shouldFinish;
        std::chrono::system_clock::time_point lastError;
        // apr_time_t delayTime = 0;
        std::string output;
        std::string name;
        int reportStatus = 0;
        bool isRunning = false;
        bool isAliScript = false;
        //针对于云监控阿里版脚本
        enum ScriptResultFormat resultFormat{RAW_FORMAT};
        std::vector<LabelAddInfo> labelAddInfos;
        std::vector<MetricFilterInfo> metricFilterInfos;
        std::vector<MetricMeta> metricMetas;
        std::chrono::system_clock::time_point startTime;
        std::vector<std::pair<std::string, std::string>> outputVector;

        std::string getName() const {
            return name;
        }
    };

    typedef std::map<std::string, std::shared_ptr<ScriptScheduleState>> ScriptStateMap;

#include "common/test_support"
class ScriptScheduler : public common::ThreadWorker {
public:
    explicit ScriptScheduler(const std::function<std::shared_ptr<common::ProcessWorker>()> & = nullptr);
    ~ScriptScheduler() override;

    int getStateMap(ScriptStateMap &stateMap) const;
    int getStateMap(ScriptStateMap &stateMap, const std::string &mid) const;
    void GetStatus(common::CommonMetric &metric);

    boost::json::array getInfo(const std::string &opValue) const;
private:
    void doRun() override;
    void onTimer();

    int schedule(const ScriptItem &s);
    int checkFirstRun(const ScriptItem &s);
    int checkRun(const ScriptItem &s, const std::shared_ptr<ScriptScheduleState> &state);

    VIRTUAL int runItem(const ScriptItem &s,
                        const std::shared_ptr<ScriptScheduleState> &state,
                        const std::chrono::system_clock::time_point &now);

    // int waitForSlot();
    bool checkSlot() const;

    int checkForEnd();

    int recycleKilled();
    VIRTUAL int outputResult(const std::shared_ptr<ScriptScheduleState> &state,
                             int exitCode, std::string &errMsg, const std::string &result);
    VIRTUAL std::string BuildExecuteResult(const std::string &scriptName, std::string errorMsg,
                                           int exitCode, int metricNum);
private:
    std::map<std::string, ScriptItem> m_items;
    std::map<std::string, std::shared_ptr<common::ProcessWorker>> m_running;
    std::list<std::shared_ptr<common::ProcessWorker>> m_killed;

    SafeMap<std::string, std::shared_ptr<ScriptScheduleState>> m_state;  //mid -> state
    // common::InstanceLock m_stateLock;
    std::function<std::shared_ptr<common::ProcessWorker>()> createProcessWorker;
};
#include "common/test_support"

}
#endif
