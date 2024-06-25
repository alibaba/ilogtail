#ifndef ARGUS_CORE_TASK_MONITOR_H
#define ARGUS_CORE_TASK_MONITOR_H
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include "TaskManager.h"
#include "common/JsonValueEx.h"
#include "common/ThreadWorker.h"

namespace argus
{
    size_t parseOutput(const json::Array &arr,
                       std::vector<std::pair<std::string, std::string>> &output,
                       const std::string &defType);

#include "common/test_support"
class TaskMonitor : public common::ThreadWorker{
public:
    TaskMonitor();
    ~TaskMonitor() override;
    int ParseExporterTaskInfo(const std::string &json,bool writeLocalFile = false);
    int ParseShennongExporterTaskInfo(const std::string &json,bool writeLocalFile = false);
    static int ParseExporterTaskInfo(const std::string &json, std::vector<ExporterItem> &items);

    int ParseUnifiedConfig(const std::string &jsonConfig, const std::string &md5 = "");
private:
    void doRun() override;
    int LoadModuleTaskFromFile();
    int LoadScriptTaskFromFile();
    int LoadAliScriptTaskFromFile();
    int LoadReceiveTaskFromFile();
    int LoadExporterTaskFromFile();
    int LoadShennongExporterTaskFromFile();
    int LoadHttpReceiveTaskFromFile();
    int LoadBaseMetricFromFile();
    int ParseHttpReceiveTaskInfo(const std::string &json,bool writeLocalFile=false);
    int ParseScriptTaskInfo(const std::string &jsonContent,bool writeLocalFile=false);
    int ParseAliScriptTaskInfo(const std::string &jsonContent,bool writeLocalFile=false);
    static int ParseModuleTaskInfo(
            const std::string &fileContent,
            const std::function<void(const std::shared_ptr<std::map<std::string, ModuleItem>> &)> &fnSave);
    int ParseBaseMetricInfo(const std::string &json,bool  writeLocalFile = false);
    int ParseCustomTaskInfo(const std::string &json,std::unordered_map<std::string, std::vector<std::string>> &contentMap);
    static int ParseCmsDetectTaskInfo(const std::string &json);
    static int ParseCmsProcessTaskInfo(const std::string &json);
    static int ParseCmsTopNTaskInfo(const std::string &json);
public:
    static void ParseHttpInfo(const json::Object &);
    static void ParseTelnetInfo(const json::Object &);
    static void ParsePingInfo(const json::Object &);
    static void ParseProcessInfo(const json::Object &);

    // mid机制简直就是不靠谱，增加成员极有可能会漏掉
    // static std::string GetModuleItemMid(const ModuleItem & item);
    // static std::string CalcMid(const ScriptItem & item);
    // static std::string GetExporterItemMid(const ExporterItem & item);

private:
    std::chrono::seconds mInterval{0};
    std::string mModuleTaskFile;
    std::string mModuleTaskMd5;
    std::string mReceiveTaskFile;
    std::string mReceiveTaskMd5;
    std::string mScriptTaskFile;
    std::string mScriptTaskMd5;
    std::string mAliScriptTaskFile;
    std::string mAliScriptTaskMd5;
    std::string mExporterTaskFile;
    std::string mShennongExporterTaskFile;
    std::string mExporterTaskMd5;
    std::string mShennongExporterTaskMd5;
    std::string mHttpReceiveTaskFile;
    std::string mHttpReceiveTaskMd5;
    std::string mBaseMetricFile;
    std::string mBaseMetricMd5;

    std::string mUnifiedConfigMd5;

    std::string mLoggerTaskMd5;
    std::string mLoggerTaskFile;
    std::string mLokiTaskMd5;
    std::string mLokiTaskFile;
    std::string mCmsDetectTaskFile;
    std::string mCmsDetectTaskMd5;
    std::string mCmsProcessTaskFile;
    std::string mCmsProcessTaskMd5;
    std::string mCmsTopNTaskFile;
    std::string mCmsTopNTaskMd5;

};
#include "common/test_support"

typedef common::Singleton<TaskMonitor> SingletonTaskMonitor;
}
#endif