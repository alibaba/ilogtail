#include "task_monitor.h"

#include <algorithm>  // std::max

#include "common/Config.h"
#include "common/Logger.h"
#include "common/Md5.h"
#include "common/FileUtils.h"
#include "common/SystemUtil.h"
#include "common/ResourceConsumptionRecord.h"
#include "common/Chrono.h"

// windows特别扯蛋，定义了两个这么重要的宏。
#ifdef max
#   undef max
#   undef min
#endif

using namespace std;
using namespace common;

static const char *const KEY_NAME = "name";
static const char *const KEY_CRON = "cron";
static const char *const keyCmd = "cmd";
static const char *const keyVersion = "version";
static const char *const keyInterval = "interval";
static const char *const keyTimeout = "timeout";
static const char *const keyUser = "user";
static const char *const keyDuration = "duration";
static const char *const keyFirstSche = "firstSche";
static const char *const keyReportStatus = "reportStatus";
static const char *const keyOutput = "output";
static const char *const keyType = "type";
static const char *const keyConf = "conf";
static const char *const keyScriptItem = "scriptItem";

static constexpr const int MIN_DETECT_INTERVAL = 5;
static constexpr const int MAX_DETECT_HTTP_TIMEOUT = 10;
static constexpr const int MIN_DETECT_HTTP_TIMEOUT = 0;

// 校正interval
static std::chrono::seconds correctDetectInterval(int64_t interval) {
    interval = std::max<int64_t>((interval <= 0 ? 60 : interval), MIN_DETECT_INTERVAL);
    return std::chrono::seconds{interval};
}

namespace argus
{
    void parseLabels(const json::Array &arr, std::map<std::string, std::string> &labels) {
        for (size_t k = 0; k < arr.size(); k++) {
            json::Object metricLabelItem = arr[k].asObject();
            if (!metricLabelItem.isNull()) {
                string labelName = metricLabelItem.getString("label_name");
                string labelValue = metricLabelItem.getString("label_value");
                if (!labelName.empty() && !labelValue.empty()) {
                    labels[labelName] = labelValue;
                }
            }
        }
    }

    size_t parseOutput(const json::Array &arr, vector<pair<string, string>> &output, const std::string &defType) {
        if (arr.isNull()) {
            if (!defType.empty()) {
                LogInfo("will use default output: {}", defType);
                output.emplace_back(defType, "");
            }
        } else {
            arr.forEach([&](size_t, const json::Object &obj) {
                string type = obj.getString(keyType);
                if (!type.empty()) {
                    string conf;
                    json::Value confValue = obj.getValue(keyConf);
                    if (!confValue.isNull()) {
                        conf = confValue.toStyledString();
                    }
                    output.emplace_back(type, conf);
                }
            });
        }
        return output.size();
    }

/*
const static string EMPTY_JSON_OBJECT="{\n}";
const static string EMPTY_JSON_OBJECT_MD5=MD5String(EMPTY_JSON_OBJECT.c_str(),EMPTY_JSON_OBJECT.size());

const static string EMPTY_JSON_ARRAY="[\n]";
const static string EMPTY_JSON_ARRAY_MD5=MD5String(EMPTY_JSON_ARRAY.c_str(),EMPTY_JSON_ARRAY.size());

const static string EMPTY_MODULE_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_MODULE_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_SCRIPT_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_SCRIPT_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_ALISCRIPT_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_ALISCRIPT_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_RECEIVE_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_RECEIVE_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_EXPORTER_TASK=EMPTY_JSON_ARRAY;
const static string EMPTY_EXPORTER_TASK_MD5=EMPTY_JSON_ARRAY_MD5;

const static string EMPTY_HTTP_RECEIVE_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_HTTP_RECEIVE_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_BASE_METRIC_TASK=EMPTY_JSON_ARRAY;
const static string EMPTY_BASE_METRIC_TASK_MD5=EMPTY_JSON_ARRAY_MD5;

const static string EMPTY_LOKI_TASK=EMPTY_JSON_ARRAY;
const static string EMPTY_LOKI_TASK_MD5=EMPTY_JSON_ARRAY_MD5;

const static string EMPTY_LOGGER_TASK=EMPTY_JSON_ARRAY;
const static string EMPTY_LOGGER_TASK_MD5=EMPTY_JSON_ARRAY_MD5;

const static string EMPTY_CMS_DETECT_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_CMS_DETECT_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_CMS_PROCESS_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_CMS_PROCESS_TASK_MD5=EMPTY_JSON_OBJECT_MD5;

const static string EMPTY_CMS_TOPN_TASK=EMPTY_JSON_OBJECT;
const static string EMPTY_CMS_TOPN_TASK_MD5=EMPTY_JSON_OBJECT_MD5;
*/
TaskMonitor::TaskMonitor()
{
    Config *cfg = SingletonConfig::Instance();
    mInterval = std::chrono::seconds{cfg->GetValue("agent.task.monitor.interval", 10)};
    cfg->getModuleTaskFile(mModuleTaskFile);
    mModuleTaskMd5 = "";
    cfg->getScriptTaskFile(mScriptTaskFile);
    mScriptTaskMd5 = "";
    cfg->getReceiveTaskFile(mReceiveTaskFile);
    mReceiveTaskMd5 = "";
    const auto &baseDir = cfg->getBaseDir();
    mAliScriptTaskFile = (baseDir / "local_data" / "conf" / "aliScriptTask.json").string();
    mAliScriptTaskFile = cfg->GetValue("agent.aliscript.task.file", mAliScriptTaskFile);
    mAliScriptTaskMd5 = "";
    mExporterTaskFile = (baseDir / "local_data" / "conf" / "exporterTask.json").string();
    mExporterTaskFile = cfg->GetValue("agent.exporter.task.file", mExporterTaskFile);
    mShennongExporterTaskFile = (baseDir / "local_data" / "conf" / "shennongExporterTask.json").string();
    mShennongExporterTaskFile = cfg->GetValue("agent.shennong.exporter.task.file", mShennongExporterTaskFile);
    mExporterTaskMd5 = "";
    mShennongExporterTaskMd5 = "";
    mHttpReceiveTaskFile = (baseDir / "local_data" / "conf" / "httpReceiveTask.json").string();
    mHttpReceiveTaskFile = cfg->GetValue("agent.exporter.http.receive.file", mHttpReceiveTaskFile);
    mHttpReceiveTaskMd5 = "";
    mLoggerTaskFile = (baseDir / "local_data" / "conf" / "loggerTask.json").string();
    mLoggerTaskFile = cfg->GetValue("agent.logger.task.file", mLoggerTaskFile);
    mLoggerTaskMd5 = "";
    mLokiTaskFile = (baseDir / "local_data" / "conf" / "lokiTask.json").string();
    mLokiTaskFile = cfg->GetValue("agent.loki.task.file", mLokiTaskFile);
    mLokiTaskFile = "";
    mBaseMetricFile = (baseDir / "local_data" / "conf" / "baseMetric.json").string();
    mBaseMetricMd5 = "";
    mCmsDetectTaskFile = (baseDir / "local_data" / "conf" / "detectTask.json").string();
    mCmsDetectTaskMd5 = "";
    mCmsProcessTaskFile = (baseDir / "local_data" / "conf" / "processTask.json").string();
    mCmsProcessTaskMd5 = "";
    mCmsTopNTaskFile = (baseDir / "local_data" / "conf" / "topNTask.json").string();
    mCmsTopNTaskMd5 = "";
    //Load ARGUS_EXPORT_CONFIG;
    string exportJson = SystemUtil::GetEnv("ARGUS_EXPORT_CONFIG");
    if (!exportJson.empty()) {
        ParseExporterTaskInfo(exportJson, true);
    }
}
TaskMonitor::~TaskMonitor()
{
    endThread();
    join();
}
void TaskMonitor::doRun()
{
    const Config *cfg = SingletonConfig::Instance();
    const bool enableModuleTask = cfg->GetValue("agent.enable.baseMetric.collect", true);
    const bool enableExporterTask = cfg->GetValue("agent.enable.exporter.collect", true);
    const bool enableShennongExporterTask = cfg->GetValue("agent.enable.shennong.exporter.collect", true);
    const bool enableReceiveTask = cfg->GetValue("agent.enable.receive.collect", false);
    //baseMetric配置默认容许从本地读取;
#ifndef ENABLE_CLOUD_MONITOR
    const bool enableScriptTask = cfg->GetValue("agent.enable.script.collect", true);
    const bool enableAliScriptTask = cfg->GetValue("agent.enable.script.collect", true);
    const bool enableHttpReceiveTask = cfg->GetValue("agent.enable.http.listen", true);
    const bool enableBaseMetric = cfg->GetValue("agent.enable.base.metric", true);
#endif

    for (; !isThreadEnd(); sleepFor(mInterval)) {
        CpuProfile("TaskMonitor::doRun");

        if (enableModuleTask) {
            LoadModuleTaskFromFile();
        }
        if (enableReceiveTask) {
            LoadReceiveTaskFromFile();
        }
        if (enableExporterTask) {
            LoadExporterTaskFromFile();
        }
        if (enableShennongExporterTask) {
            LoadShennongExporterTaskFromFile();
        }
#ifndef ENABLE_CLOUD_MONITOR
        if (enableScriptTask) {
            LoadScriptTaskFromFile();
        }
        if (enableAliScriptTask) {
            LoadAliScriptTaskFromFile();
        }
        if (enableHttpReceiveTask) {
            LoadHttpReceiveTaskFromFile();
        }
        if (enableBaseMetric) {
            LoadBaseMetricFromFile();
        }
#endif
    }
}

int TaskMonitor::ParseModuleTaskInfo(
        const string &fileContent,
        const std::function<void(const std::shared_ptr<std::map<std::string, ModuleItem>> &)> &fnSave) {
    const string keyModuleItem = "moduleItem";
    // const string keyModule = "module";
    // const string keyArgs = "args";
    //check content
    // JsonValue jsonValue;
    // if (!jsonValue.parse(fileContent))
    // {
    //     LogError("module task not json:{}", fileContent.c_str());
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogError("module task not object:{}", fileContent.c_str());
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(fileContent, error);
    if (value.isNull()) {
        LogError("module task json is invalid: {}, json: {}", error, fileContent);
        return -1;
    }

    auto moduleItemMap = std::make_shared<map<string, ModuleItem>>();
    //read moduleItem
    json::Array moduleItemValue = value.getArray(keyModuleItem);
    if (moduleItemValue.isNull()) {
        LogInfo("no key ({}) in taskFile", keyModuleItem);
    } else {
        moduleItemValue.forEach([&](size_t, const json::Object &moduleItem){
            if (moduleItem.getBool("disabled", false)) {
                return; // 允许显式禁用采集项
            }
            ModuleItem mi;
            //read name
            mi.name = moduleItem.getString(KEY_NAME);
            if (mi.name.empty()) {
                LogWarn( "parse name error,skip this moduleItem");
                return;
            }

            //module，在新的比选，老的不存在，需要考虑兼容
            mi.module = moduleItem.getString("module");
            mi.isSoType = mi.module.empty();
            if (mi.module.empty()) {
                mi.module = mi.name;
            }

            //read verison
            mi.moduleVersion = moduleItem.getString(keyVersion);
            // if (mi.moduleVersion.empty()) {
            //     LogInfo("parse version error of this moduleItem");
            // }

            //read cron
            std::string cron = moduleItem.getString(KEY_CRON, QUARTZ_ALL);
            if (!cron.empty() && cron != QUARTZ_ALL && !mi.timePeriod.parse(cron.c_str())) {
                LogWarn("parse cron error,cron={}", cron);
                return;
            }
            mi.scheduleExpr = cron;

            //read intv
            int interval = moduleItem.getNumber<int>(keyInterval, -1);
            if (interval <= 0) {
                LogWarn("parse interval error,skip this moduleItem");
                return;
            }
            //单位从秒转换成ms
            mi.scheduleInterval = std::chrono::milliseconds(interval * 1000);

            //args可选的参数
            mi.moduleArgs = moduleItem.getString("args");

            //read output
            if (parseOutput(moduleItem.getArray(keyOutput), mi.outputVector, "") == 0) {
                LogWarn("outputItem is 0,name={}", mi.name);
                return;
            }
            mi.mid = mi.name + "_" + MD5String(moduleItem.toStyledString()); // GetModuleItemMid(mi);
            moduleItemMap->emplace(mi.mid, mi);
        });
    }
    fnSave(moduleItemMap);

    return static_cast<int>(moduleItemMap->size());
}

int TaskMonitor::LoadModuleTaskFromFile()
{
    string jsonContent;
    if (FileUtils::ReadFileContent(mModuleTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mModuleTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo( "module Task md5 is not the same,will reload module task");
            mModuleTaskMd5 = md5;
            return ParseModuleTaskInfo(jsonContent, [](const std::shared_ptr<std::map<std::string, ModuleItem>> &d){
                SingletonTaskManager::Instance()->SetModuleItems(d);
            });
        }
    }
    return -1;
}

static const auto &mapFormat = *new std::map<std::string, ScriptResultFormat>{
        {"prometheus",     PROMETHEUS_FORMAT},
        {"alimonitorJson", ALIMONITOR_JSON_FORMAT},
        {"alimonitorText", ALIMONITOR_TEXT_FORMAT},
        {"rawData",        RAW_DATA_FORMAT},
};

ScriptResultFormat getResultFormatEnum(const std::string &f) {
    auto it = mapFormat.find(f);
    return it != mapFormat.end()? it->second: RAW_FORMAT;
}
ScriptResultFormat getResultFormatEnum(const std::string &f, ScriptResultFormat matchedFormat) {
    return mapFormat.find(f) != mapFormat.end()? matchedFormat: RAW_FORMAT;
}

int TaskMonitor::ParseScriptTaskInfo(const string &fileContent, bool writeLocalFile) {
    // JsonValue jsonValue;
    // if (!jsonValue.parse(fileContent))
    // {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(fileContent, error);
    if (value.isNull()){
        LogError("task file format error: {}, please check", error);
        return -1;
    }

    auto scriptItemMap = std::make_shared<map<string, ScriptItem>>();
    //read scriptItem
    json::Array scriptItemValue = value.getArray(keyScriptItem);
    if (scriptItemValue.isNull()) {
        LogInfo("no script task({}) in scriptTaskFile", keyScriptItem);
        return -2;
    } else {
        scriptItemValue.forEach([&](size_t, const json::Object &scriptItem){
            ScriptItem mi;
            //read name
            mi.name = scriptItem.getString(KEY_NAME);
            if (mi.name.empty()) {
                LogWarn("parse name error,skip this scriptItem");
                return;
            }

            //read verison
            const string version = scriptItem.getString(keyVersion);
            if (version.empty()) {
                LogWarn("parse version error,skip this scriptItem({})", mi.name);
                return;
            }
            //read cron
            mi.scheduleExpr = scriptItem.getString(KEY_CRON, "[* * * * * ?]");
            if (!mi.timePeriod.parse(mi.scheduleExpr.c_str())) {
                LogWarn("parse cron error,skip this scriptItem({}), cron={}", mi.name, mi.scheduleExpr);
                return;
            }

            //read cmd
            mi.collectUrl = scriptItem.getString(keyCmd);
            if (mi.collectUrl.empty()) {
                LogWarn("parse cmd error,skip this scriptItem({})", mi.name);
                return;
            }
            //read interval
            mi.scheduleIntv = std::chrono::seconds(scriptItem.getNumber<int>(keyInterval));
            if (mi.scheduleIntv.count() <= 0) {
                LogWarn("interval is {} of scriptItem({}), skip", mi.scheduleIntv, mi.name);
                return;
            }

            //read timeout
            mi.timeOut = std::chrono::seconds{scriptItem.getNumber<int>(keyTimeout)};
            if (mi.timeOut.count() <= 0) {
                LogInfo("timeout({}) invalid of scriptItem({}), skip", mi.timeOut, mi.name);
                return;
            } else if (mi.timeOut > mi.scheduleIntv) {
                LogInfo("scriptItem({}) timeout({}) > scheduleInterval({}), use: {}",
                        mi.name, mi.timeOut, mi.scheduleIntv, mi.scheduleIntv);
                mi.timeOut = mi.scheduleIntv;
            }
            //read user
            mi.scriptUser = scriptItem.getString(keyUser, "nobody");
            //read duration,小于等于0等于没有设置
            mi.duration = std::chrono::seconds{scriptItem.getNumber(keyDuration, 0)};
            //首次调度时间
            mi.firstSche = std::chrono::FromSeconds(scriptItem.getNumber(keyFirstSche, 0));

            mi.reportStatus = scriptItem.getNumber(keyReportStatus, 0);
            //read resultFormat;
            mi.resultFormat = getResultFormatEnum(scriptItem.getString("resultFormat"));

            parseAddLabels("ScriptTask", scriptItem.getArray("add_labels"), mi.labelAddInfos);
            //read output;
            if (parseOutput(scriptItem.getArray(keyOutput), mi.outputVector, "") == 0) {
                LogWarn("parse output error, outputItem is 0, skip. scriptItem: {}", scriptItem.toStyledString());
                return;
            }
            mi.mid = mi.name + "_" + version + "_" + MD5String(scriptItem.toStyledString()); // CalcMid(mi);
            (*scriptItemMap)[mi.mid] = mi;
        });
        // for (size_t i = 0; i < scriptItemValue.size(); i++)
        // {
        //     ScriptItem mi;
        //     Json::Value scriptItem = scriptItemValue.get(i, Json::Value::null);
        //     if (scriptItem.isNull() || !scriptItem.isObject())
        //     {
        //         continue;
        //     }
        //     //read name
        //     Json::Value nameValue = scriptItem.get(KEY_NAME, Json::Value::null);
        //     if (nameValue.isNull() || !nameValue.isString())
        //     {
        //         LogWarn("parse name error,skip this scriptItem");
        //         continue;
        //     }
        //     string name = nameValue.asString();
        //     mi.name = name;
        //     //read verison
        //     Json::Value versionValue = scriptItem.get(keyVersion, Json::Value::null);
        //     if (versionValue.isNull() || !versionValue.isString())
        //     {
        //         LogWarn("parse version error,skip this scriptItem");
        //         continue;
        //     }
        //     string version = versionValue.asString();
        //     //read cron
        //     string cron;
        //     Json::Value cronValue = scriptItem.get(KEY_CRON, Json::Value::null);
        //     if (cronValue.isNull() || !cronValue.isString())
        //     {
        //         cron = "[* * * * * ?]";
        //     }
        //     else
        //     {
        //         cron = cronValue.asString();
        //     }
        //     if (!mi.timePeriod.parse(cron.c_str()))
        //     {
        //         LogWarn("parse cron error,skip this scriptItem,cron={}", cron.c_str());
        //         continue;
        //     }
        //     mi.scheduleExpr = cron;
        //     //read cmd
        //     Json::Value cmdValue = scriptItem.get(keyCmd, Json::Value::null);
        //     if (cmdValue.isNull() || !cmdValue.isString())
        //     {
        //         LogWarn("parse cmd error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.collectUrl = cmdValue.asString();
        //     //read interval
        //     Json::Value intervalValue = scriptItem.get(keyInterval, Json::Value::null);
        //     if (intervalValue.isNull() || !intervalValue.isInt())
        //     {
        //         LogWarn("parse interval error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.scheduleIntv = intervalValue.asInt();
        //     if (mi.scheduleIntv == 0)
        //     {
        //         LogWarn("interval is 0,skip");
        //         continue;
        //     }
        //     //read timeout
        //     Json::Value timeoutValue = scriptItem.get(keyTimeout, Json::Value::null);
        //     if (timeoutValue.isNull() || !timeoutValue.isInt())
        //     {
        //         LogWarn("parse timeout error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.timeOut = timeoutValue.asInt();
        //     if (mi.timeOut > mi.scheduleIntv)
        //     {
        //         LogInfo("timeout({}u)>scheduleIntv({}u),skip", mi.timeOut, mi.scheduleIntv);
        //         mi.timeOut = mi.scheduleIntv;
        //     }
        //     //read user
        //     Json::Value userValue = scriptItem.get(keyUser, Json::Value::null);
        //     if (userValue.isNull() || !userValue.isString())
        //     {
        //         mi.scriptUser = "nobody";
        //     }
        //     else
        //     {
        //         mi.scriptUser = userValue.asString();
        //     }
        //     //read duration,小于等于0等于没有设置
        //     int duration = 0;
        //     Json::Value durationValue = scriptItem.get(keyDuration, Json::Value::null);
        //     if (!durationValue.isNull() && durationValue.isInt())
        //     {
        //         duration = durationValue.asInt();
        //     }
        //     mi.duration = duration;
        //     //首次调度时间
        //     int firstSche = 0;
        //     Json::Value first_scheValue = scriptItem.get(keyFirstSche, Json::Value::null);
        //     if (!first_scheValue.isNull() && first_scheValue.isInt())
        //     {
        //         firstSche = first_scheValue.asInt();
        //     }
        //     mi.firstSche = firstSche;
        //     int reportStatus = 0;
        //     Json::Value report_statusValue = scriptItem.get(keyReportStatus, Json::Value::null);
        //     if (!report_statusValue.isNull() && report_statusValue.isInt())
        //     {
        //         reportStatus = report_statusValue.asInt();
        //     }
        //     mi.reportStatus = reportStatus;
        //     //read resultFormat;
        //     string resultFormat;
        //     Json::Value resultFormatValue = scriptItem.get("resultFormat", Json::Value::null);
        //     if (!resultFormatValue.isNull() && resultFormatValue.isString())
        //     {
        //         resultFormat = resultFormatValue.asString();
        //     }
        //     if (resultFormat == "prometheus")
        //     {
        //         mi.resultFormat = PROMETHEUS_FORMAT;
        //     }
        //     else if (resultFormat == "alimonitorJson")
        //     {
        //         mi.resultFormat = ALIMONITOR_JSON_FORMAT;
        //     }
        //     else if (resultFormat == "alimonitorText")
        //     {
        //         mi.resultFormat = ALIMONITOR_TEXT_FORMAT;
        //     }
        //     else if (resultFormat == "rawData")
        //     {
        //         mi.resultFormat = RAW_DATA_FORMAT;
        //     }
        //     else
        //     {
        //         mi.resultFormat = RAW_FORMAT;
        //     }
        //     //read add_labels
        //     Json::Value addLablesValue = scriptItem.get("add_labels", Json::Value::null);
        //     if (!addLablesValue.isNull() && addLablesValue.isArray())
        //     {
        //         for (size_t j = 0; j < addLablesValue.size(); j++)
        //         {
        //             Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
        //             if (addLableItem.isNull() || !addLableItem.isObject())
        //             {
        //                 continue;
        //             }
        //             LabelAddInfo lableAddInfo;
        //             CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
        //             CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
        //             CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
        //             if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
        //             {
        //                 LogWarn("skip invalid add_labels item,type={},name={}",
        //                          lableAddInfo.type,
        //                          lableAddInfo.name.c_str());
        //                 continue;
        //             }
        //             mi.labelAddInfos.push_back(lableAddInfo);
        //         }
        //     }
        //     //read output;
        //     Json::Value outputValue = scriptItem.get(keyOutput, Json::Value::null);
        //     if (outputValue.isNull() || !outputValue.isArray())
        //     {
        //         LogWarn("parse output error,skip this scriptItem:{}",
        //                  scriptItem.toStyledString().c_str());
        //         continue;
        //     }
        //     for (size_t j = 0; j < outputValue.size(); j++)
        //     {
        //         Json::Value outputItem = outputValue.get(j, Json::Value::null);
        //         if (!outputItem.isObject() || outputItem.isNull())
        //         {
        //             continue;
        //         }
        //         Json::Value typeValue = outputItem.get(keyType, Json::Value::null);
        //         if (typeValue.isNull() || !typeValue.isString())
        //         {
        //             LogWarn("parse output-type error,skip this outputItem:{}",
        //                      scriptItem.toStyledString().c_str());
        //             continue;
        //         }
        //         string type = typeValue.asString();
        //         string conf;
        //         Json::Value confValue = outputItem.get(keyConf, Json::Value::null);
        //         if (confValue.isNull())
        //         {
        //             LogInfo( "output-conf is null");
        //         }
        //         else
        //         {
        //             conf = confValue.toStyledString();
        //         }
        //         mi.outputVector.emplace_back(type, conf);
        //     }
        //     if (mi.outputVector.empty())
        //     {
        //         LogInfo("outputItem is 0,name={}", name.c_str());
        //         continue;
        //     }
        //     mi.mid = name + "_" + version + "_" + GetScriptItemMid(mi);
        //     (*scriptItemMap)[mi.mid] = mi;
        // }
    }
    SingletonTaskManager::Instance()->SetScriptItems(scriptItemMap);
    if (writeLocalFile)
    {
        mScriptTaskMd5 = MD5String(fileContent.c_str(), fileContent.size());
        FileUtils::WriteFileContent(mScriptTaskFile, fileContent);
    }
    return static_cast<int>(scriptItemMap->size());
}

//与ParseScriptTaskInfo解耦，以方便以后两者出现更多差异后进行扩展。
int TaskMonitor::ParseAliScriptTaskInfo(const string &fileContent, bool writeLocalFile)
{
    // JsonValue jsonValue;
    // if (!jsonValue.parse(fileContent)) {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject()) {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(fileContent, error);
    if (value.isNull()) {
        LogError("AliScriptTask file format error: {}", error);
        return -1;
    }

    auto AliScriptItemMap = std::make_shared<map<string, ScriptItem>>();
    //read scriptItem
    json::Array scriptItemValue = value.getArray(keyScriptItem);
    if (scriptItemValue.isNull()) {
        LogInfo("no script task({}) in scriptTaskFile", keyScriptItem);
        return -2;
    } else {
        scriptItemValue.forEach([&](size_t index, const json::Object &scriptItem) {
            ScriptItem mi;
            //read name
            if (scriptItem.get(KEY_NAME, mi.name) == 0) {
                LogWarn("scriptItem[{}] has no name, skipped", index);
                return;
            }

            //read protocolVersion
            mi.protocolVersion = scriptItem.getNumber<int>("protocolVersion", 1);
            //read verison
            const std::string version = scriptItem.getString(keyVersion);
            if (version.empty()) {
                LogWarn("parse version error,skip this scriptItem[{}]", mi.name);
                return;
            }

            //read cron
            mi.scheduleExpr = scriptItem.getString(KEY_CRON, "[* * * * * ?]");
            if (!mi.timePeriod.parse(mi.scheduleExpr.c_str())) {
                LogWarn("parse cron error,skip this scriptItem[{}], cron={}", mi.name, mi.scheduleExpr);
                return;
            }

            //read cmd
            if (scriptItem.get(keyCmd, mi.collectUrl) == 0) {
                LogWarn("parse cmd error,skip this scriptItem[{}]", mi.name);
                return;
            }

            //read interval
            mi.scheduleIntv = std::chrono::seconds{scriptItem.getNumber<int>(keyInterval)};
            if (mi.scheduleIntv <= std::chrono::seconds{0}) {
                LogWarn("interval is 0 of scriptItem[{}],skip", mi.name);
                return;
            }

            //read timeout
            mi.timeOut = std::chrono::seconds{scriptItem.getNumber<int64_t>(keyTimeout, -1)};
            if (mi.timeOut <= std::chrono::seconds{0}) {
                LogWarn("parse timeout error,skip this scriptItem[{}]", mi.name);
                return;
            } else if (mi.timeOut > mi.scheduleIntv) {
                LogInfo("scriptItem[{}] timeout({})>scheduleIntv({}),skip", mi.name, mi.timeOut, mi.scheduleIntv);
                mi.timeOut = mi.scheduleIntv;
            }

            //read user
            mi.scriptUser = scriptItem.getString(keyUser, "nobody");
            //read duration,小于等于0等于没有设置
            mi.duration = std::chrono::seconds{scriptItem.getNumber<int>(keyDuration, 0)};
            //首次调度时间
            mi.firstSche = std::chrono::FromSeconds(scriptItem.getNumber(keyFirstSche, int64_t(0)));
            //
            mi.reportStatus = scriptItem.getNumber<int>(keyReportStatus, 0);
            //read resultFormat;
            if (mi.protocolVersion == 1) {
                mi.resultFormat = getResultFormatEnum(scriptItem.getString("resultFormat"));
            } else {
                mi.resultFormat = getResultFormatEnum(scriptItem.getString("resultFormat"), RAW_DATA_FORMAT);
            }
            //read add_labels
            parseAddLabels("AliScriptTask", scriptItem.getArray("add_labels"), mi.labelAddInfos);
            //read output;
            parseOutput(scriptItem.getArray(keyOutput), mi.outputVector, "");
            if (mi.outputVector.empty()) {
                LogInfo("outputItem is 0,name={}", mi.name);
                return;
            }

            //read metrics
            scriptItem.getArray("metrics").forEach([&](size_t, const json::Object &metricFilterItem){
                MetricFilterInfo filterInfo;
                if (metricFilterItem.get("name", filterInfo.name) > 0) {
                    parseLabels(metricFilterItem.getArray("labels"), filterInfo.tagMap);
                    filterInfo.metricName = metricFilterItem.getString("metric_name");

                    mi.metricFilterInfos.push_back(filterInfo);
                }
            });

            //read metricMeta
            scriptItem.getArray("metricMeta").forEach([&](size_t, const json::Object &metricMetaItem){
                MetricMeta metricMeta;
                metricMeta.name = metricMetaItem.getString("name");
                if (metricMeta.name.empty()) {
                    LogWarn("skip invalid metricMeta item duto without name, script name={}", mi.name);
                    return;
                }

                metricMetaItem.get("isMetric", metricMeta.isMetric);
                string type = metricMetaItem.getString("type");
                if (type == "number") {
                    metricMeta.type = 1;
                } else if (type == "string") {
                    metricMeta.type = 2;
                }

                mi.metricMetas.push_back(metricMeta);
            });
            mi.isAliScript = true;
            //script和aliscript即使配置一样，也会存在两个。
            mi.mid = mi.name + "_" + version + "_AliScript_" + MD5String(scriptItem.toStyledString()); // CalcMid(mi);
            (*AliScriptItemMap)[mi.mid] = mi;
        });
        // for (size_t i = 0; i < scriptItemValue.size(); i++)
        // {
        //     ScriptItem mi;
        //     Json::Value scriptItem = scriptItemValue.get(i, Json::Value::null);
        //     if (scriptItem.isNull() || !scriptItem.isObject())
        //     {
        //         continue;
        //     }
        //     //read name
        //     Json::Value nameValue = scriptItem.get(KEY_NAME, Json::Value::null);
        //     if (nameValue.isNull() || !nameValue.isString())
        //     {
        //         LogWarn("parse name error,skip this scriptItem");
        //         continue;
        //     }
        //     string name = nameValue.asString();
        //     mi.name = name;
        //     //read protocolVersion
        //     Json::Value protocolVersionValue = scriptItem.get("protocolVersion", Json::Value::null);
        //     int protocolVersion = 1;
        //     if (protocolVersionValue.isNull() || !protocolVersionValue.isInt())
        //     {
        //         LogInfo( "parse protocolVersion failed, default protocolVersion is 1");
        //     }else
        //     {
        //         protocolVersion = protocolVersionValue.asInt();
        //     }
        //     mi.protocolVersion = protocolVersion;
        //     //read verison
        //     Json::Value versionValue = scriptItem.get(keyVersion, Json::Value::null);
        //     if (versionValue.isNull() || !versionValue.isString())
        //     {
        //         LogWarn("parse version error,skip this scriptItem");
        //         continue;
        //     }
        //     string version = versionValue.asString();
        //     //read cron
        //     string cron;
        //     Json::Value cronValue = scriptItem.get(KEY_CRON, Json::Value::null);
        //     if (cronValue.isNull() || !cronValue.isString())
        //     {
        //         cron = "[* * * * * ?]";
        //     }
        //     else
        //     {
        //         cron = cronValue.asString();
        //     }
        //     if (!mi.timePeriod.parse(cron.c_str()))
        //     {
        //         LogWarn("parse cron error,skip this scriptItem,cron={}", cron.c_str());
        //         continue;
        //     }
        //     mi.scheduleExpr = cron;
        //     //read cmd
        //     Json::Value cmdValue = scriptItem.get(keyCmd, Json::Value::null);
        //     if (cmdValue.isNull() || !cmdValue.isString())
        //     {
        //         LogWarn("parse cmd error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.collectUrl = cmdValue.asString();
        //     //read interval
        //     Json::Value intervalValue = scriptItem.get(keyInterval, Json::Value::null);
        //     if (intervalValue.isNull() || !intervalValue.isInt())
        //     {
        //         LogWarn("parse interval error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.scheduleIntv = intervalValue.asInt();
        //     if (mi.scheduleIntv == 0)
        //     {
        //         LogWarn("interval is 0,skip");
        //         continue;
        //     }
        //     //read timeout
        //     Json::Value timeoutValue = scriptItem.get(keyTimeout, Json::Value::null);
        //     if (timeoutValue.isNull() || !timeoutValue.isInt())
        //     {
        //         LogWarn("parse timeout error,skip this scriptItem");
        //         continue;
        //     }
        //     mi.timeOut = timeoutValue.asInt();
        //     if (mi.timeOut > mi.scheduleIntv)
        //     {
        //         LogInfo("timeout({}u)>scheduleIntv({}u),skip", mi.timeOut, mi.scheduleIntv);
        //         mi.timeOut = mi.scheduleIntv;
        //     }
        //     //read user
        //     Json::Value userValue = scriptItem.get(keyUser, Json::Value::null);
        //     if (userValue.isNull() || !userValue.isString())
        //     {
        //         mi.scriptUser = "nobody";
        //     }
        //     else
        //     {
        //         mi.scriptUser = userValue.asString();
        //     }
        //     //read duration,小于等于0等于没有设置
        //     int duration = 0;
        //     Json::Value durationValue = scriptItem.get(keyDuration, Json::Value::null);
        //     if (!durationValue.isNull() && durationValue.isInt())
        //     {
        //         duration = durationValue.asInt();
        //     }
        //     mi.duration = duration;
        //     //首次调度时间
        //     int firstSche = 0;
        //     Json::Value first_scheValue = scriptItem.get(keyFirstSche, Json::Value::null);
        //     if (!first_scheValue.isNull() && first_scheValue.isInt())
        //     {
        //         firstSche = first_scheValue.asInt();
        //     }
        //     mi.firstSche = firstSche;
        //     int reportStatus = 0;
        //     Json::Value report_statusValue = scriptItem.get(keyReportStatus, Json::Value::null);
        //     if (!report_statusValue.isNull() && report_statusValue.isInt())
        //     {
        //         reportStatus = report_statusValue.asInt();
        //     }
        //     mi.reportStatus = reportStatus;
        //     //read resultFormat;
        //     string resultFormat;
        //     Json::Value resultFormatValue = scriptItem.get("resultFormat", Json::Value::null);
        //     if (!resultFormatValue.isNull() && resultFormatValue.isString())
        //     {
        //         resultFormat = resultFormatValue.asString();
        //     }
        //     if (resultFormat == "prometheus")
        //     {
        //         if(mi.protocolVersion==1){
        //             mi.resultFormat = PROMETHEUS_FORMAT;
        //         }else{
        //             mi.resultFormat = RAW_DATA_FORMAT;
        //         }
        //     }
        //     else if (resultFormat == "alimonitorJson")
        //     {
        //         if(mi.protocolVersion==1)
        //         {
        //             mi.resultFormat = ALIMONITOR_JSON_FORMAT;
        //         }else
        //         {
        //             mi.resultFormat = RAW_DATA_FORMAT;
        //         }
        //     }
        //     else if (resultFormat == "alimonitorText")
        //     {
        //         if(mi.protocolVersion==1)
        //         {
        //             mi.resultFormat = ALIMONITOR_TEXT_FORMAT;
        //         }else
        //         {
        //             mi.resultFormat = RAW_DATA_FORMAT;
        //         }
        //     }
        //     else if (resultFormat == "rawData")
        //     {
        //         mi.resultFormat = RAW_DATA_FORMAT;
        //     }
        //     else
        //     {
        //         mi.resultFormat = RAW_FORMAT;
        //     }
        //     //read add_labels
        //     Json::Value addLablesValue = scriptItem.get("add_labels", Json::Value::null);
        //     if (!addLablesValue.isNull() && addLablesValue.isArray())
        //     {
        //         for (size_t j = 0; j < addLablesValue.size(); j++)
        //         {
        //             Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
        //             if (addLableItem.isNull() || !addLableItem.isObject())
        //             {
        //                 continue;
        //             }
        //             LabelAddInfo lableAddInfo;
        //             CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
        //             CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
        //             CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
        //             if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
        //             {
        //                 LogWarn("skip invalid add_labels item,type={},name={}",
        //                          lableAddInfo.type,
        //                          lableAddInfo.name.c_str());
        //                 continue;
        //             }
        //             mi.labelAddInfos.push_back(lableAddInfo);
        //         }
        //     }
        //     //read output;
        //     Json::Value outputValue = scriptItem.get(keyOutput, Json::Value::null);
        //     if (outputValue.isNull() || !outputValue.isArray())
        //     {
        //         LogWarn("parse output error,skip this scriptItem:{}",
        //                  scriptItem.toStyledString().c_str());
        //         continue;
        //     }
        //     for (size_t j = 0; j < outputValue.size(); j++)
        //     {
        //         Json::Value outputItem = outputValue.get(j, Json::Value::null);
        //         if (!outputItem.isObject() || outputItem.isNull())
        //         {
        //             continue;
        //         }
        //         Json::Value typeValue = outputItem.get(keyType, Json::Value::null);
        //         if (typeValue.isNull() || !typeValue.isString())
        //         {
        //             LogWarn("parse output-type error,skip this outputItem:{}",
        //                      scriptItem.toStyledString().c_str());
        //             continue;
        //         }
        //         string type = typeValue.asString();
        //         string conf;
        //         Json::Value confValue = outputItem.get(keyConf, Json::Value::null);
        //         if (confValue.isNull())
        //         {
        //             LogInfo( "output-conf is null");
        //         }
        //         else
        //         {
        //             conf = confValue.toStyledString();
        //         }
        //         mi.outputVector.emplace_back(type, conf);
        //     }
        //     if (mi.outputVector.empty())
        //     {
        //         LogInfo("outputItem is 0,name={}", name.c_str());
        //         continue;
        //     }
        //     //read metrics
        //     Json::Value metricsValue = scriptItem.get("metrics", Json::Value::null);
        //     if (!metricsValue.isNull() && metricsValue.isArray())
        //     {
        //         for (size_t j = 0; j < metricsValue.size(); j++)
        //         {
        //             Json::Value metricFilterItem = metricsValue.get(j, Json::Value::null);
        //             if (metricFilterItem.isNull() || !metricFilterItem.isObject())
        //             {
        //                 continue;
        //             }
        //             MetricFilterInfo filterInfo;
        //             CommonJson::GetJsonStringValue(metricFilterItem, "name", filterInfo.name);
        //             if (filterInfo.name.empty())
        //             {
        //                 continue;
        //             }
        //             Json::Value metricLablesValue = metricFilterItem.get("labels", Json::Value::null);
        //             if (!metricLablesValue.isNull() && metricLablesValue.isArray())
        //             {
        //                 for (size_t k = 0; k < metricLablesValue.size(); k++)
        //                 {
        //                     Json::Value metricLabelItem = metricLablesValue.get(k, Json::Value::null);
        //                     if (metricLabelItem.isNull() || !metricLabelItem.isObject())
        //                     {
        //                         continue;
        //                     }
        //                     string labelName, labelValue;
        //                     CommonJson::GetJsonStringValue(metricLabelItem, "label_name", labelName);
        //                     CommonJson::GetJsonStringValue(metricLabelItem, "label_value", labelValue);
        //                     if (labelName.empty() || labelValue.empty())
        //                     {
        //                         continue;
        //                     }
        //                     filterInfo.tagMap[labelName] = labelValue;
        //                 }
        //             }
        //             Json::Value metricNameValue = metricFilterItem.get("metric_name", Json::Value::null);
        //             if(!metricNameValue.isNull() && metricNameValue.isString()){
        //                 filterInfo.metricName=metricNameValue.asString();
        //             }
        //             mi.metricFilterInfos.push_back(filterInfo);
        //         }
        //     }
        //     //read metricMeta
        //     Json::Value metricMetasValue = scriptItem.get("metricMeta", Json::Value::null);
        //     if (!metricMetasValue.isNull() && metricMetasValue.isArray())
        //     {
        //         for (size_t j = 0; j < metricMetasValue.size(); j++)
        //         {
        //             Json::Value metricMetaItem = metricMetasValue.get(j, Json::Value::null);
        //             if (metricMetaItem.isNull() || !metricMetaItem.isObject())
        //             {
        //                 continue;
        //             }
        //             MetricMeta metricMeta;
        //             CommonJson::GetJsonStringValue(metricMetaItem, "name", metricMeta.name);
        //             if (metricMeta.name.empty())
        //             {
        //                 LogWarn("skip invalid metricMeta item, script name={}", name.c_str());
        //                 continue;
        //             }
        //             CommonJson::GetJsonBoolValue(metricMetaItem, "isMetric", metricMeta.isMetric);
        //             string type;
        //             CommonJson::GetJsonStringValue(metricMetaItem, "type", type);
        //             if (type == "number")
        //             {
        //                 metricMeta.type = 1;
        //             }
        //             else if (type == "string")
        //             {
        //                 metricMeta.type = 2;
        //             }
        //             if (metricMeta.name.empty())
        //             {
        //                 continue;
        //             }
        //             mi.metricMetas.push_back(metricMeta);
        //         }
        //     }
        //     mi.isAliScript = true;
        //     //script和aliscript即使配置一样，也会存在两个。
        //     mi.mid = name + "_" + version + "_AliScript_" + GetScriptItemMid(mi);
        //     (*AliScriptItemMap)[mi.mid] = mi;
        // }
    }
    TaskManager *pTask = SingletonTaskManager::Instance();
    pTask->SetAliScriptItems(AliScriptItemMap);
    if (writeLocalFile)
    {
        mAliScriptTaskMd5 = MD5String(fileContent.c_str(), fileContent.size());
        FileUtils::WriteFileContent(mAliScriptTaskFile, fileContent);
    }
    return static_cast<int>(AliScriptItemMap->size());
}
int TaskMonitor::LoadScriptTaskFromFile()
{
    string jsonContent;
    if (FileUtils::ReadFileContent(mScriptTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mScriptTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo( "script Task md5 is not the same,will reload script task");
            mScriptTaskMd5 = md5;
            return ParseScriptTaskInfo(jsonContent);
        }
    }
    return -1;
}
int TaskMonitor::LoadAliScriptTaskFromFile()
{
    string jsonContent;
    if (FileUtils::ReadFileContent(mAliScriptTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mAliScriptTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo( "aliscript Task md5 is not the same,will reload aliscript task");
            mAliScriptTaskMd5 = md5;
            return ParseAliScriptTaskInfo(jsonContent);
        }
    }
    return -1;
}
int TaskMonitor::LoadReceiveTaskFromFile()
{
    const string keyReceiveItem = "receiveItem";

    if (!fs::exists(mReceiveTaskFile)) {
        return -1;
    }

    std::string fileContent = ReadFileContent(mReceiveTaskFile);
    // apr_pool_t *mp;
    // apr_file_t *file;
    // apr_pool_create(&mp, NULL);
    // apr_status_t rv = apr_file_open(&file, mReceiveTaskFile.c_str(), APR_READ, APR_UREAD, mp);
    // if (rv != APR_SUCCESS)
    // {
    //     LogError("open file ({}) error : {}", mReceiveTaskFile.c_str(), getErrorString(rv).c_str());
    //     apr_pool_destroy(mp);
    //     return -1;
    // }
    //
    // //read content
    // string fileContent;
    // while (apr_file_eof(file) != APR_EOF)
    // {
    //     char buf[4097] = {0};
    //     size_t len = 4096;
    //     apr_file_read(file, (void *) buf, &len);
    //     buf[len] = 0;
    //     fileContent.append(buf, len);
    // }
    // apr_file_close(file);
    // apr_pool_destroy(mp);
    string md5 = MD5String(fileContent.c_str(), fileContent.size());
    if (md5 == mReceiveTaskMd5) {
        return 0;
    } else {
        LogInfo("receive Task md5 is not the same,will reload receive task");
        mReceiveTaskMd5 = md5;
    }

    // //check content
    // JsonValue jsonValue;
    // if (!jsonValue.parse(fileContent))
    // {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogError("task file format error, please check!");
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(fileContent, error);
    if (value.isNull()) {
        LogError("task file format error: {}, please check", error);
        return -1;
    }

    auto ItemMap = std::make_shared<map<int, ReceiveItem>>();
    //read receiveItem
    json::Array receiveItems = value.getArray(keyReceiveItem);
    // Json::Value receiveItemValue = value.get(keyReceiveItem, Json::Value::null);
    // if (receiveItemValue.isNull() || !receiveItemValue.isArray())
    if (receiveItems.isNull()) {
        LogInfo("no key({}) in taskFile", keyReceiveItem);
        return -2;
    } else {
        receiveItems.forEach([&](size_t, const json::Object &receiveItem){
            ReceiveItem mi;
            //read name
            mi.name = receiveItem.getString(KEY_NAME);
            if (mi.name.empty()) {
                LogWarn("parse name error,skip this receiveItem");
                return ;
            }

            //read type
            json::Value typeValue = receiveItem.getValue(keyType);
            if (!typeValue.isNumber()) {
                LogWarn("parse type error,skip this moduleItem");
                return;
            }
            mi.type = typeValue.asNumber<int>();

            if (parseOutput(receiveItem.getArray(keyOutput), mi.outputVector, "") == 0) {
                LogWarn("outputItem is 0,skip this outputItem");
                return;
            }
            (*ItemMap)[mi.type] = mi;
        });
        // for (size_t i = 0; i < receiveItems.size(); i++)
        // {
        //     Json::Value receiveItem = receiveItemValue.get(i, Json::Value::null);
        //     if (receiveItem.isNull() || !receiveItem.isObject())
        //     {
        //         continue;
        //     }
        //
        //     ReceiveItem mi;
        //     //read name
        //     Json::Value nameValue = receiveItem.get(keyName, Json::Value::null);
        //     if (nameValue.isNull() || !nameValue.isString())
        //     {
        //         LogWarn("parse name error,skip this receiveItem");
        //         continue;
        //     }
        //     mi.name = nameValue.asString();
        //
        //     //read type
        //     {
        //         Json::Value typeValue = receiveItem.get(keyType, Json::Value::null);
        //         if (typeValue.isNull() || !typeValue.isInt()) {
        //             LogWarn("parse type error,skip this moduleItem");
        //             continue;
        //         }
        //         mi.type = typeValue.asInt();
        //     }
        //     //read output
        //     Json::Value outputValue = receiveItem.get(keyOutput, Json::Value::null);
        //     if (outputValue.isNull() || !outputValue.isArray())
        //     {
        //         LogWarn("parse output error,skip this moduleItem");
        //         continue;
        //     }
        //     for (size_t j = 0; j < outputValue.size(); j++)
        //     {
        //         Json::Value outputItem = outputValue.get(j, Json::Value::null);
        //         if (!outputItem.isObject() || outputItem.isNull())
        //         {
        //             continue;
        //         }
        //         Json::Value typeValue = outputItem.get(keyType, Json::Value::null);
        //         if (typeValue.isNull() || !typeValue.isString())
        //         {
        //             LogWarn("parse output-type error,skip this outputItem");
        //             continue;
        //         }
        //         string type = typeValue.asString();
        //         Json::Value confValue = outputItem.get(keyConf, Json::Value::null);
        //         string conf;
        //         if (confValue.isNull())
        //         {
        //             LogWarn("output-conf is null");
        //         }
        //         else
        //         {
        //             conf = confValue.toStyledString();
        //         }
        //         mi.outputVector.emplace_back(type, conf);
        //     }
        //     if (mi.outputVector.empty())
        //     {
        //         LogWarn("outputItem is 0,skip this outputItem");
        //         continue;
        //     }
        //     (*ItemMap)[mi.type] = mi;
        // }
    }
    TaskManager *pTask = SingletonTaskManager::Instance();
    pTask->SetReceiveItems(ItemMap);
    return static_cast<int>(ItemMap->size());
}

int TaskMonitor::LoadExporterTaskFromFile()
{
    string jsonContent;
    if (FileUtils::ReadFileContent(mExporterTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mExporterTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo("exporter Task md5 is not the same,will reload exporter task");
            mExporterTaskMd5 = md5;
            return ParseExporterTaskInfo(jsonContent);
        }
    }
    return -1;
}
int TaskMonitor::LoadShennongExporterTaskFromFile(){
    string jsonContent;
    if (FileUtils::ReadFileContent(mShennongExporterTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mShennongExporterTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo("shennong exporter Task md5 is not the same,will reload shennong exporter task");
            mShennongExporterTaskMd5 = md5;
            return ParseShennongExporterTaskInfo(jsonContent);
        }
    }
    return -1;
}
int TaskMonitor::ParseExporterTaskInfo(const string &json, std::vector<ExporterItem> &items)
{
    // //check content
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("exporter info not json format:{}", json.c_str());
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isArray())
    // {
    //     LogWarn("not json array!");
    //     return -1;
    // }
    std::string error;
    json::Array value = json::parseArray(json, error);
    if (value.isNull()) {
        LogWarn("exporter info is invalid: {}, json: {}", error, json);
        return -1;
    }

    for (size_t i = 0; i < value.size(); i++) {
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isObject())
        // {
        //     continue;
        // }
        json::Object item = value[i].asObject();
        if (item.isNull()) {
            continue;
        }

        ExporterItem exporterItem;
        item.get("target", exporterItem.target);
        item.get("name", exporterItem.name);
        item.get("metrics_path", exporterItem.metricsPath);
        item.getArray("headers").toStringArray(exporterItem.headers);
        if (exporterItem.metricsPath.empty())
        {
            exporterItem.metricsPath = "/metrics";
        }
        if (exporterItem.name.empty())
        {
            exporterItem.name = exporterItem.target + exporterItem.metricsPath;
        }
        exporterItem.interval = std::chrono::seconds{item.getNumber("interval", exporterItem.interval.count())};
        exporterItem.timeout = std::chrono::seconds{item.getNumber("timeout", exporterItem.timeout.count())};
        item.get("add_status", exporterItem.addStatus);
        item.get("type", exporterItem.type);
        if (exporterItem.interval.count() <= 0)
        {
            LogWarn("invalid interval!");
            continue;
        }
        if (exporterItem.timeout.count() <= 0)
        {
            exporterItem.timeout = exporterItem.interval;
        }
        // Json::Value metricsValue = item.get("metrics", Json::Value::null);
        // if (!metricsValue.isNull() && metricsValue.isArray())
        item.getArray("metrics").forEach([&](size_t, const json::Object &metricFilterItem){
                MetricFilterInfo filterInfo;
                metricFilterItem.get("name", filterInfo.name);
                if (filterInfo.name.empty()) {
                    return;
                }
                parseLabels(metricFilterItem.getArray("labels"), filterInfo.tagMap);
                metricFilterItem.get("metric_name", filterInfo.metricName);

                exporterItem.metricFilterInfos.push_back(filterInfo);
        });
        // {
        //     for (size_t j = 0; j < metricsValue.size(); j++)
        //     {
        //         Json::Value metricFilterItem = metricsValue.get(j, Json::Value::null);
        //         if (metricFilterItem.isNull() || !metricFilterItem.isObject())
        //         {
        //             continue;
        //         }
        //         MetricFilterInfo filterInfo;
        //         CommonJson::GetJsonStringValue(metricFilterItem, "name", filterInfo.name);
        //         if (filterInfo.name.empty())
        //         {
        //             continue;
        //         }
        //         Json::Value metricLablesValue = metricFilterItem.get("labels", Json::Value::null);
        //         if (!metricLablesValue.isNull() && metricLablesValue.isArray())
        //         {
        //             for (size_t k = 0; k < metricLablesValue.size(); k++)
        //             {
        //                 Json::Value metricLabelItem = metricLablesValue.get(k, Json::Value::null);
        //                 if (metricLabelItem.isNull() || !metricLabelItem.isObject())
        //                 {
        //                     continue;
        //                 }
        //                 string labelName, labelValue;
        //                 CommonJson::GetJsonStringValue(metricLabelItem, "label_name", labelName);
        //                 CommonJson::GetJsonStringValue(metricLabelItem, "label_value", labelValue);
        //                 if (labelName.empty() || labelValue.empty())
        //                 {
        //                     continue;
        //                 }
        //                 filterInfo.tagMap[labelName] = labelValue;
        //             }
        //         }
        //         Json::Value metricNameValue = metricFilterItem.get("metric_name", Json::Value::null);
        //         if (!metricNameValue.isNull() && metricNameValue.isString())
        //         {
        //             filterInfo.metricName = metricNameValue.asString();
        //         }
        //         exporterItem.metricFilterInfos.push_back(filterInfo);
        //     }
        // }
        parse("ExporterTask", item.getArray("add_labels"), exporterItem.labelAddInfos);
        // Json::Value addLablesValue = item.get("add_labels", Json::Value::null);
        // if (!addLablesValue.isNull() && addLablesValue.isArray())
        // {
        //     for (size_t j = 0; j < addLablesValue.size(); j++)
        //     {
        //         Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
        //         if (addLableItem.isNull() || !addLableItem.isObject())
        //         {
        //             continue;
        //         }
        //         LabelAddInfo lableAddInfo;
        //         CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
        //         CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
        //         CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
        //         if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
        //         {
        //             LogWarn("skip invalid add_labels item,type={},name={}",
        //                      lableAddInfo.type,
        //                      lableAddInfo.name);
        //             continue;
        //         }
        //         exporterItem.labelAddInfos.push_back(lableAddInfo);
        //     }
        // }
        //read output
        if (parseOutput(item.getArray("output"), exporterItem.outputVector, "alimonitor") == 0) {
            LogWarn("outputItem is 0,skip this exporterTask Item");
            continue;
        }
        // Json::Value outputValue = item.get("output", Json::Value::null);
        // if (outputValue.isNull() || !outputValue.isArray())
        // {
        //     LogInfo("will use the default output channel-alimonitor");
        //     exporterItem.outputVector.emplace_back("alimonitor", "");
        // }
        // else
        // {
        //     for (size_t j = 0; j < outputValue.size(); j++)
        //     {
        //         Json::Value outputItem = outputValue.get(j, Json::Value::null);
        //         if (!outputItem.isObject() || outputItem.isNull())
        //         {
        //             continue;
        //         }
        //         string type;
        //         string conf;
        //         CommonJson::GetJsonStringValue(outputItem, "type", type);
        //         if (type.empty())
        //         {
        //             continue;
        //         }
        //         Json::Value confValue = outputItem.get("conf", Json::Value::null);
        //         if (confValue.isNull())
        //         {
        //             LogInfo("output-conf is null");
        //         }
        //         else
        //         {
        //             conf = confValue.toStyledString();
        //         }
        //         exporterItem.outputVector.emplace_back(type, conf);
        //     }
        //     if (exporterItem.outputVector.empty())
        //     {
        //         LogWarn("outputItem is 0,skip this exporterTask Item");
        //         continue;
        //     }
        // }
        //唯一标识该item;
        exporterItem.mid = exporterItem.name + "_" + MD5String(item.toStyledString()); // GetExporterItemMid(exporterItem);
        items.push_back(exporterItem);
    }
    return static_cast<int>(items.size());
}
int TaskMonitor::ParseExporterTaskInfo(const string &json, bool writeLocalFile)
{
    auto items = std::make_shared<vector<ExporterItem>>();
    if (ParseExporterTaskInfo(json, *items) < 0){
        LogError( "parse exporter task ERROR!");
        return -1;
    }
    // TaskManager *pTask = SingletonTaskManager::Instance();
    SingletonTaskManager::Instance()->SetExporterItems(items);
    if (writeLocalFile)
    {
        mExporterTaskMd5 = MD5String(json.c_str(), json.size());
        FileUtils::WriteFileContent(mExporterTaskFile, json);
    }
    return static_cast<int>(items->size());
}
int TaskMonitor::ParseShennongExporterTaskInfo(const string &json, bool writeLocalFile)
{
    // vector<ExporterItem> items;
    auto items = std::make_shared<vector<ExporterItem>>();
    if (ParseExporterTaskInfo(json, *items) < 0){
        LogError( "parse exporter task ERROR!");
        return -1;
    }
    // TaskManager *pTask = SingletonTaskManager::Instance();
    SingletonTaskManager::Instance()->SetShennongExporterItems(items);
    if (writeLocalFile)
    {
        mShennongExporterTaskMd5 = MD5String(json.c_str(), json.size());
        FileUtils::WriteFileContent(mShennongExporterTaskFile, json);
    }
    return static_cast<int>(items->size());
}

int TaskMonitor::ParseHttpReceiveTaskInfo(const string &json, bool writeLocalFile)
{
    HttpReceiveItem httpReceiveItem;

    // //check content
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("http receive task not json format!");
    //     return -1;
    // }
    // Json::Value item = jsonValue.getValue();
    // if (!item.isObject())
    // {
    //     LogWarn("http receive task not object");
    //     return -1;
    // }
    std::string error;
    json::Object item = json::parseObject(json, error);
    if (item.isNull()) {
        LogWarn("http receive task is invalid: {}, json: {}", error, json);
        return -1;
    }

    // Json::Value metricsValue = item.get("metrics", Json::Value::null);
    // if (!metricsValue.isNull() && metricsValue.isArray())
    item.getArray("metrics").forEach([&](size_t, const json::Object &metricFilterItem){
        MetricFilterInfo filterInfo;
        filterInfo.name = metricFilterItem.getString("name");
        if (!filterInfo.name.empty()) {
            parseLabels(metricFilterItem.getArray("labels"), filterInfo.tagMap);

            httpReceiveItem.metricFilterInfos.push_back(filterInfo);
        }
    });
    parse("", item.getArray("add_labels"), httpReceiveItem.labelAddInfos);
    // Json::Value addLablesValue = item.get("add_labels", Json::Value::null);
    // if (!addLablesValue.isNull() && addLablesValue.isArray())
    // {
    //     for (size_t j = 0; j < addLablesValue.size(); j++)
    //     {
    //         Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
    //         if (addLableItem.isNull() || !addLableItem.isObject())
    //         {
    //             continue;
    //         }
    //         LabelAddInfo lableAddInfo;
    //         CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
    //         CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
    //         CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
    //         if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
    //         {
    //             LogWarn("skip invalid add_labels item,type={},name={}",
    //                      lableAddInfo.type,
    //                      lableAddInfo.name.c_str());
    //             continue;
    //         }
    //         httpReceiveItem.labelAddInfos.push_back(lableAddInfo);
    //     }
    // }
    //read output
    // Json::Value outputValue = item.get("output", Json::Value::null);
    // if (outputValue.isNull() || !outputValue.isArray())
    if (parseOutput(item.getArray("output"), httpReceiveItem.outputVector, "alimonitor") == 0) {
        LogWarn("outputItem is 0,skip this http receive Item");
        return -1;
    }
    // if (outputValue.isNull()) {
    //     LogInfo("will use the default output channel-alimonitor");
    //     httpReceiveItem.outputVector.emplace_back("alimonitor", "");
    // } else {
    //     for (size_t j = 0; j < outputValue.size(); j++)
    //     {
    //         // Json::Value outputItem = outputValue.get(j, Json::Value::null);
    //         // if (!outputItem.isObject() || outputItem.isNull())
    //         // {
    //         //     continue;
    //         // }
    //         json::Object outputItem = outputValue[j].asObject();
    //         if (outputItem.isNull()) {
    //             continue;
    //         }
    //
    //         string type = outputItem.getString("type");
    //         if (type.empty()) {
    //             continue;
    //         }
    //         // CommonJson::GetJsonStringValue(outputItem, "type", type);
    //         // if (type.empty())
    //         // {
    //         //     continue;
    //         // }
    //         // Json::Value confValue = outputItem.get("conf", Json::Value::null);
    //         json::Value confValue = outputItem.getValue("conf");
    //         string conf;
    //         if (confValue.isNull()) {
    //             LogInfo("output-conf is null");
    //         } else {
    //             conf = confValue.toStyledString();
    //         }
    //         httpReceiveItem.outputVector.emplace_back(type, conf);
    //     }
    //     if (httpReceiveItem.outputVector.empty())
    //     {
    //         LogWarn("outputItem is 0,skip this http receive Item");
    //         return -1;
    //     }
    // }
    TaskManager *pTask = SingletonTaskManager::Instance();
    pTask->SetHttpReceiveItem(MakeCopyShared(httpReceiveItem));
    if (writeLocalFile)
    {
        mHttpReceiveTaskMd5 = MD5String(json.c_str(), json.size());
        FileUtils::WriteFileContent(mHttpReceiveTaskFile, json);
    }
    return 1;
}
int TaskMonitor::LoadHttpReceiveTaskFromFile()
{
    string jsonContent;
    if (FileUtils::ReadFileContent(mHttpReceiveTaskFile, jsonContent) > 0)
    {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mHttpReceiveTaskMd5)
        {
            return 0;
        }
        else
        {
            LogInfo("http receive Task md5 is not the same,will http receive task");
            mHttpReceiveTaskMd5 = md5;
            return ParseHttpReceiveTaskInfo(jsonContent);
        }
    }
    return -1;
}

int TaskMonitor::LoadBaseMetricFromFile() {
    string jsonContent;
    if (FileUtils::ReadFileContent(mBaseMetricFile, jsonContent) > 0) {
        string md5 = MD5String(jsonContent.c_str(), jsonContent.size());
        if (md5 == mBaseMetricMd5) {
            return 0;
        } else {
            LogInfo("baseMetric is not the same,will update baseMetric");
            mBaseMetricMd5 = md5;
            return ParseBaseMetricInfo(jsonContent, false);
        }
    }
    return -1;
}

int TaskMonitor::ParseBaseMetricInfo(const string &json, bool writeLocalFile)
{
    // //check content
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("not json format!");
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isArray())
    // {
    //     LogWarn("not array base metric json!");
    //     return -1;
    // }
    std::string error;
    json::Array value = json::parseArray(json, error);
    if (value.isNull()) {
        LogWarn("not a valid baseMetric json: {}, json: {}", error, json);
        return -1;
    }

    auto aliModuleItemMap = std::make_shared<map<string, AliModuleItem>>();
    for (size_t i = 0; i < value.size(); i++) {
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isObject())
        // {
        //     continue;
        // }
        json::Object item = value[i].asObject();
        if (item.isNull()) {
            continue;
        }
        string md5 = item.toStyledString();
        md5 = MD5String(md5.c_str(), md5.size());

        AliModuleItem aliItem;
        if (!aliItem.parseJson(item)) {
            continue;
        }
        // // CommonJson::GetJsonIntValue(item, "interval", aliItem.interval);
        // aliItem.interval = std::chrono::seconds{item.getNumber("interval", ToSeconds(aliItem.interval))};
        // // CommonJson::GetJsonStringValue(item, "module_name", aliItem.moduleName);
        // aliItem.moduleName = item.getString("module_name");
        // // CommonJson::GetJsonStringValue(item, "label_name", aliItem.tagName);
        // if (IsZero(aliItem.interval) || aliItem.moduleName.empty()) {
        //     LogWarn("skip invalid baseMetricItem,moduel_name={},interval={}", aliItem.moduleName, aliItem.interval);
        //     continue;
        // }
        // // CommonJson::GetJsonStringValues(item, "metric_list", aliItem.metrics);
        // aliItem.metrics = item.getArray("metric_list").toStringArray();
        // // CommonJson::GetJsonStringValues(item, "label_values", aliItem.tagValues);
        // // aliItem.tagName = item.getString("label_name");
        // // aliItem.tagValues = item.getArray("label_values").toStringArray();
        // aliItem.whitelist = parseBaseMetricKeyValues(item.getArray("whitelist"));
        // aliItem.blacklist = parseBaseMetricKeyValues(item.getArray("blacklist"));
        // tagKeyValues filterItem = parseBaseMetricKeyValues(item); // 兼容旧格式
        // if (!filterItem.key.empty()) {
        //     aliItem.whitelist.push_back(filterItem);
        // }
        // parse("baseMetric", item.getArray("add_labels"), aliItem.labelAddInfos);
        // // Json::Value addLablesValue = item.get("add_labels", Json::Value::null);
        // // if (!addLablesValue.isNull() && addLablesValue.isArray())
        // // {
        // //     for (size_t j = 0; j < addLablesValue.size(); j++)
        // //     {
        // //         Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
        // //         if (addLableItem.isNull() || !addLableItem.isObject())
        // //         {
        // //             continue;
        // //         }
        // //         LabelAddInfo lableAddInfo;
        // //         CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
        // //         CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
        // //         CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
        // //         if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
        // //         {
        // //             LogWarn("skip invalid add_labels item,type={},name={}",
        // //                      lableAddInfo.type,
        // //                      lableAddInfo.name.c_str());
        // //             continue;
        // //         }
        // //         aliItem.labelAddInfos.push_back(lableAddInfo);
        // //     }
        // // }
        (*aliModuleItemMap)[aliItem.moduleName + md5] = aliItem;
    }
    // TaskManager *pTask = SingletonTaskManager::Instance();
    SingletonTaskManager::Instance()->SetAliModuleItems(aliModuleItemMap);
    if (writeLocalFile)
    {
        mBaseMetricMd5 = MD5String(json.c_str(), json.size());
        FileUtils::WriteFileContent(mBaseMetricFile, json);
    }
    return static_cast<int>(aliModuleItemMap->size());
}
int TaskMonitor::ParseUnifiedConfig(const string &jsonConfig, const std::string &cfgMd5)
{
    if (!cfgMd5.empty() && cfgMd5 == mUnifiedConfigMd5) {
        LogInfo("unifiedConfig, not changed");
        return 0;
    }
    // JsonValue jsonValue;
    // if (!jsonValue.parse(jsonConfig))
    // {
    //     LogWarn("unified config not json format!");
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogWarn("unified config not json object!");
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(jsonConfig, error);
    if (value.isNull()) {
        LogWarn("unified config json is invalid: {}, json: {}", error, jsonConfig);
        return -1;
    }

    string exporterTask = value.getValue("exporterTask").toStyledString();
    string httpReceiveTask;
    // string moduleTask;
    //string scriptTask;
    // string loggerTask;
    // string baseMetric;
    // string customTask;
    // string lokiTask;
    // CommonJson::GetJsonObjectStyledString(value, "exporterTask", exporterTask);
    //httpReceiveTask从服务端下发的为数组,需要进行转换
    // Json::Value httpReceiveTaskValue = value.get("httpReceiveTask", Json::Value::null);
    // if (!httpReceiveTaskValue.isNull() && httpReceiveTaskValue.isArray() && !httpReceiveTaskValue.empty())
    json::Array httpReceiveTaskArray = value.getArray("httpReceiveTask");
    if (!httpReceiveTaskArray.empty()) {
        //只需要取第1个
        // int i = 0;
        // Json::Value itemValue = httpReceiveTaskValue.get(i, Json::Value::null);
        httpReceiveTask = httpReceiveTaskArray[0].toStyledString();
    }
    //CommonJson::GetJsonObjectStyledString(value, "httpReceiveTask",httpReceiveTask);
    // CommonJson::GetJsonObjectStyledString(value, "moduleTask", moduleTask);
    string moduleTask = value.getValue("moduleTask").toStyledString();
    //CommonJson::GetJsonObjectStyledString(value, "scriptTask", scriptTask);
    // CommonJson::GetJsonObjectStyledString(value, "baseMetric", baseMetric);
    string baseMetric = value.getValue("baseMetric").toStyledString();
    // CommonJson::GetJsonObjectStyledString(value, "loggerTask", loggerTask);
    string loggerTask = value.getValue("loggerTask").toStyledString();
    // CommonJson::GetJsonObjectStyledString(value, "customTask", customTask);
    string customTask = value.getValue("customTask").toStyledString();
    // CommonJson::GetJsonObjectStyledString(value, "lokiTask", lokiTask);
    string lokiTask = value.getValue("lokiTask").toStyledString();
    unordered_map<string, vector<string>> customMap;
    if (!customTask.empty()) {
        ParseCustomTaskInfo(customTask, customMap);
    }
    auto exportTaskIt = customMap.find("exporterTask");
    if (!exporterTask.empty() || exportTaskIt != customMap.end()) {
        //进行合并
        if (exportTaskIt != customMap.end()) {
            exporterTask = json::mergeArray(exporterTask, exportTaskIt->second);
        }

        string md5 = MD5String(exporterTask);
        if (mExporterTaskMd5 != md5) {
            LogInfo("exporter Task md5 is not the same,receive new exporter task");
            ParseExporterTaskInfo(exporterTask, true);
        }
    }
    // else if(mExporterTaskMd5 != EMPTY_EXPORTER_TASK_MD5)
    // {
    //     LogInfo("exporter Task md5 is not empty, will empty the exporter Task");
    //     ParseExporterTaskInfo(EMPTY_EXPORTER_TASK, true);
    // }
    if (!httpReceiveTask.empty()) {
        string md5 = MD5String(httpReceiveTask.c_str(), httpReceiveTask.size());
        if (mHttpReceiveTaskMd5 != md5) {
            LogInfo("http Task md5 is not the same,receive new http receive task");
            ParseHttpReceiveTaskInfo(httpReceiveTask, true);
        }
    }
    // else if(mHttpReceiveTaskMd5 != EMPTY_HTTP_RECEIVE_TASK_MD5)
    // {
    //     LogInfo("http Task md5 is not empty, will empty the http Task");
    //     ParseHttpReceiveTaskInfo(EMPTY_HTTP_RECEIVE_TASK, true);
    // }
    if (!moduleTask.empty()) {
        //保持和老的定义兼容
        string moduleTaskPre = "{\n\t\"moduleItem\":";
        moduleTask = moduleTaskPre + moduleTask;
        moduleTask += "\n}";
        string md5 = MD5String(moduleTask.c_str(), moduleTask.size());
        if (mModuleTaskMd5 != md5) {
            LogInfo("module Task md5 is not the same,receive new module task");
            ParseModuleTaskInfo(moduleTask, [&](const std::shared_ptr<std::map<std::string, ModuleItem>> &d){
                SingletonTaskManager::Instance()->SetModuleItems(d);

                mModuleTaskMd5 = md5;
                FileUtils::WriteFileContent(mModuleTaskFile, moduleTask);
            });
        }
    }
    // else if(mModuleTaskMd5 != EMPTY_MODULE_TASK_MD5)
    // {
    //     LogInfo("module Task md5 is not empty, will empty the module Task");
    //     ParseModuleTaskInfo(EMPTY_MODULE_TASK, true);
    // }
    /*
    if (!scriptTask.empty())
    {
        string scriptTaskPre = "{\n\t\"scriptItem\":";
        scriptTask = scriptTaskPre + scriptTask;
        scriptTask += "\n}";
        string md5 = MD5String(scriptTask.c_str(), scriptTask.size());
        if (mScriptTaskMd5 != md5)
        {
            LogInfo("receive Task md5 is not the same,receive new receive task");
            ParseScriptTaskInfo(scriptTask, true);
        }
    }
    */
    string aliScriptTask;
    auto aliScriptTaskIt = customMap.find("scriptTask");
    if (aliScriptTaskIt != customMap.end())
    {
        //进行合并
        aliScriptTask = json::mergeArray(aliScriptTask, aliScriptTaskIt->second);
        string aliScriptTaskPre = "{\n\t\"scriptItem\":";
        aliScriptTask = aliScriptTaskPre + aliScriptTask;
        aliScriptTask += "\n}";
        string md5 = MD5String(aliScriptTask.c_str(), aliScriptTask.size());
        if (mAliScriptTaskMd5 != md5)
        {
            LogInfo("aliScript Task md5 is not the same,receive new aliScript task");
            ParseAliScriptTaskInfo(aliScriptTask, true);
        }
    }
    // else if(mAliScriptTaskMd5 != EMPTY_ALISCRIPT_TASK_MD5)
    // {
    //     LogInfo("aliScript Task md5 is not empty, will empty the aliScript Task");
    //     ParseAliScriptTaskInfo(EMPTY_ALISCRIPT_TASK, true);
    // }
    if (!baseMetric.empty())
    {
        //保持和老的定义兼容
        string md5 = MD5String(baseMetric.c_str(), baseMetric.size());
        if (mBaseMetricMd5 != md5)
        {
            LogInfo("base metric md5 is not the same,receive new base metric");
            ParseBaseMetricInfo(baseMetric, true);
        }
    }
    // else if(mBaseMetricMd5 != EMPTY_BASE_METRIC_TASK_MD5)
    // {
    //     LogInfo("base metric md5 is not empty, will empty the base metric");
    //     ParseBaseMetricInfo(EMPTY_BASE_METRIC_TASK, true);
    // }
    if (!loggerTask.empty())
    {
        string md5 = MD5String(loggerTask.c_str(), loggerTask.size());
        if (mLoggerTaskMd5 != md5)
        {
            LogInfo("logger task is not the same,receive new logger task");
            mLoggerTaskMd5 = md5;
            FileUtils::WriteFileContent(mLoggerTaskFile, loggerTask);
        }
    }
    // else if(mLoggerTaskMd5 != EMPTY_LOGGER_TASK_MD5)
    // {
    //     LogInfo("logger task md5 is not empty, will empty the logger task");
    //     mLoggerTaskMd5 = EMPTY_LOGGER_TASK_MD5;
    //     FileUtils::WriteFileContent(mLoggerTaskFile, EMPTY_LOGGER_TASK);
    // }
    if (!lokiTask.empty())
    {
        string md5 = MD5String(lokiTask.c_str(), lokiTask.size());
        if (mLokiTaskMd5 != md5)
        {
            LogInfo("loki task is not the same,receive new loki task");
            mLokiTaskMd5 = md5;
            FileUtils::WriteFileContent(mLokiTaskFile, lokiTask);
        }
    }
    // else if(mLokiTaskMd5 != EMPTY_LOKI_TASK_MD5)
    // {
    //     LogInfo("loki task md5 is not empty, will empty the loki task");
    //     mLokiTaskMd5 = EMPTY_LOKI_TASK_MD5;
    //     FileUtils::WriteFileContent(mLokiTaskFile, EMPTY_LOKI_TASK);
    // }
    if (!cfgMd5.empty()) {
        mUnifiedConfigMd5 = cfgMd5;
    }
    return 0;
}

int TaskMonitor::ParseCustomTaskInfo(const string &json, unordered_map<string, vector<string>> &contentMap) {
    std::string error;
    json::Array value = json::parseArray(json, error);
    if (value.isNull()) {
        LogWarn("the custom task is not valid json: {}, error: {}", json, error);
        return -1;
    }

    auto parseAndSave = [](const std::string &key, const std::string &content, std::string &md5sum,
                           const std::string &path, int (*parse)(const std::string &)) {
        string md5 = MD5String(content);
        if (md5sum != md5) {
            LogInfo("{} task is not the same,receive new cmsDetectTask task", key);
            md5sum = md5;
            FileUtils::WriteFileContent(path, content);
            parse(content);
        }
    };
    std::map<std::string, std::function<void(const std::string &key, const std::string &)>> mapTypeProc {
            {"cmsDetectTask", [&](const std::string &key, const std::string &content){
                parseAndSave(key, content, mCmsDetectTaskMd5, mCmsDetectTaskFile, ParseCmsDetectTaskInfo);
            }},
            {"cmsProcessTask", [&](const std::string &key, const std::string &content) {
                parseAndSave(key, content, mCmsProcessTaskMd5, mCmsProcessTaskFile, ParseCmsProcessTaskInfo);
            }},
            {"cmsTopNTask", [&](const std::string &key, const std::string &content) {
                parseAndSave(key, content, mCmsTopNTaskMd5, mCmsTopNTaskFile, ParseCmsTopNTaskInfo);
            }},
    };
    const size_t size = value.size();
    for (size_t i = 0; i < size; i++) {
        json::Object item = value[i].asObject();
        if (!item.isNull()) {
            string type = item.getString("type");
            string content = item.getObject("content").toStyledString();
            auto proc = mapTypeProc.find(type);
            if (proc != mapTypeProc.end()) {
                proc->second(proc->first, content);
            } else {
                contentMap[type].push_back(content);
            }
        }
        // if ("cmsDetectTask" == type) {
        //     // isCmsDetectTaskSet=true;
        //     string md5 = MD5String(content.c_str(), content.size());
        //     if (mCmsDetectTaskMd5 != md5) {
        //         LogInfo("cmsDetectTask task is not the same,receive new cmsDetectTask task");
        //         mCmsDetectTaskMd5 = md5;
        //         FileUtils::WriteFileContent(mCmsDetectTaskFile, content);
        //         ParseCmsDetectTaskInfo(content);
        //     }
        // } else if ("cmsProcessTask" == type) {
        //     // isCmsDetectTaskSet=true;
        //     string md5 = MD5String(content.c_str(), content.size());
        //     if (mCmsProcessTaskMd5 != md5) {
        //         LogInfo("cmsProcessTask task is not the same,receive new cmsProcessTask task");
        //         mCmsProcessTaskMd5 = md5;
        //         FileUtils::WriteFileContent(mCmsProcessTaskFile, content);
        //         ParseCmsProcessTaskInfo(content);
        //     }
        //
        // } else if ("cmsTopNTask" == type) {
        //     // isCmsDetectTaskSet=true;
        //     string md5 = MD5String(content.c_str(), content.size());
        //     if (mCmsTopNTaskMd5 != md5) {
        //         LogInfo("cmsTopNTask task is not the same,receive new cmsTopNTask task");
        //         mCmsTopNTaskMd5 = md5;
        //         FileUtils::WriteFileContent(mCmsTopNTaskFile, content);
        //         ParseCmsTopNTaskInfo(content);
        //     }
        // } else {
        //     auto it = contentMap.find(type);
        //     if (it == contentMap.end()) {
        //         vector<string> contentVector;
        //         contentVector.push_back(content);
        //         contentMap[type] = contentVector;
        //     } else {
        //         it->second.push_back(content);
        //     }
        // }
    }

    return 0;
}

int TaskMonitor::ParseCmsDetectTaskInfo(const string &json)
{
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("the cmsDetect task is not json:{}", json.c_str());
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogWarn("the cmsDetect task is not object json:{}", json.c_str());
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(json, error);
    if (value.isNull()) {
        LogWarn("the cmsDetect task is not object json: {}, error: {}", json, error);
        return -1;
    }

#ifdef ENABLE_COVERAGE
    if (auto *p = value.native()->if_contains("add_labels")) {
        std::cout << *p << std::endl;
    }
#endif
    std::vector<LabelAddInfo> labelAddInfos;
    if (parse("CmsDetect", value.getArray("add_labels"), labelAddInfos)) {
        SingletonTaskManager::Instance()->SetCmsDetectAddTags(labelAddInfos);
    }
    // json::Array labels = value.getArray("add_labels");
    // if (!labels.isNull()) {
    //     std::vector<LabelAddInfo> labelAddInfos;
    //     for (size_t j = 0; j < labels.size(); j++) {
    //         // Json::Value addLableItem = addLablesValue.get(j, Json::Value::null);
    //         // if (addLableItem.isNull() || !addLableItem.isObject())
    //         // {
    //         //     continue;
    //         // }
    //         json::Object addLabelItem = labels[j].asObject();
    //         if (addLabelItem.isNull()) {
    //             continue;
    //         }
    //         LabelAddInfo labelInfo;
    //         if (!parse(fmt::format("CmsDetect[{}]", j), addLabelItem, labelInfo)) {
    //             continue;
    //         }
    //         // // CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
    //         // labelInfo.name = addLabelItem.getString("name");
    //         // // CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
    //         // labelInfo.value = addLabelItem.getString("value");
    //         // // CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
    //         // labelInfo.type = addLabelItem.getNumber<decltype(labelInfo.type)>("type");
    //         // if (labelInfo.type < 0 || labelInfo.name.empty()) {
    //         //     LogWarn("skip invalid add_labels item,type={},name={}", labelInfo.type, labelInfo.name);
    //         //     continue;
    //         // }
    //         labelAddInfos.push_back(labelInfo);
    //     }
    //     SingletonTaskManager::Instance()->SetCmsDetectAddTags(labelAddInfos);
    // }
    ParseHttpInfo(value);
    ParsePingInfo(value);
    ParseTelnetInfo(value);
    return 0;
}
int TaskMonitor::ParseCmsProcessTaskInfo(const string &json)
{
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("the cmsDetect task is not json:{}", json.c_str());
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogWarn("the cmsDetect task is not object json:{}", json.c_str());
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(json, error);
    if (value.isNull()) {
        LogWarn("the cmsDetect task is invalid: {}, json: {}", error, json);
        return -1;
    }

    vector<LabelAddInfo> labelAddInfos;
    if (parse("CmsProcess", value.getArray("add_labels"), labelAddInfos)) {
        SingletonTaskManager::Instance()->SetCmsProcessAddTags(labelAddInfos);
    }
    // json::Array labels = value.getArray("add_labels");
    // // Json::Value addLablesValue = value.get("add_labels", Json::Value::null);
    // // if (!addLablesValue.isNull() && addLablesValue.isArray())
    // if (!labels.isNull()) {
    //     vector<LabelAddInfo> labelAddInfos;
    //     labelAddInfos.reserve(labels.size());
    //     for (size_t j = 0; j < labels.size(); j++) {
    //         // Json::Value addLableItem = labels.get(j, Json::Value::null);
    //         // if (addLableItem.isNull() || !addLableItem.isObject())
    //         // {
    //         //     continue;
    //         // }
    //         json::Object labelItem = labels[j].asObject();
    //         if (labelItem.isNull()) {
    //             continue;
    //         }
    //
    //         LabelAddInfo lableAddInfo;
    //         if (!parse(fmt::format("CmsProcess[{}]", j), labelItem, lableAddInfo)) {
    //             continue;
    //         }
    //         // // CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
    //         // lableAddInfo.name = labelItem.getString("name");
    //         // // CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
    //         // lableAddInfo.value = labelItem.getString("value");
    //         // // CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
    //         // lableAddInfo.type = labelItem.getNumber("type", lableAddInfo.type);
    //         // if (lableAddInfo.type < 0 || lableAddInfo.name.empty()) {
    //         //     LogWarn("skip invalid add_labels item,type={},name={}", lableAddInfo.type, lableAddInfo.name);
    //         //     continue;
    //         // }
    //         labelAddInfos.push_back(lableAddInfo);
    //     }
    //     SingletonTaskManager::Instance()->SetCmsProcessAddTags(labelAddInfos);
    // }
    ParseProcessInfo(value);
    return 0;
}

int TaskMonitor::ParseCmsTopNTaskInfo(const string &json)
{
    // JsonValue jsonValue;
    // if (!jsonValue.parse(json))
    // {
    //     LogWarn("the cmsTopN task is not json:{}", json.c_str());
    //     return -1;
    // }
    // Json::Value value = jsonValue.getValue();
    // if (!value.isObject())
    // {
    //     LogWarn("the cmsTopN task is not object json:{}", json.c_str());
    //     return -1;
    // }
    std::string error;
    json::Object value = json::parseObject(json, error);
    if (value.isNull()) {
        LogWarn("the cmsTopN task config is invalid: {}, json: {}", error, json);
        return -1;
    }

    TopNItem item;
    parse("CmsTopNTask", value.getArray("add_labels"), item.labelAddInfos);
    // // Json::Value addLablesValue = value.get("add_labels", Json::Value::null);
    // // if (!addLablesValue.isNull() && addLablesValue.isArray())
    // json::Array arrLabel = value.getArray("add_labels");
    // if (!arrLabel.isNull()) {
    //     vector<LabelAddInfo> labelAddInfos;
    //     for (size_t j = 0; j < arrLabel.size(); j++)
    //     {
    //         // Json::Value addLableItem = arrLabel.get(j, Json::Value::null);
    //         // if (addLableItem.isNull() || !addLableItem.isObject())
    //         // {
    //         //     continue;
    //         // }
    //         json::Object addLableItem = arrLabel[j].asObject();
    //         if (addLableItem.isNull()) {
    //             continue;
    //         }
    //
    //         LabelAddInfo lableAddInfo;
    //         if (!parse(fmt::format("CmsTopNTask[{}]", j), addLableItem, lableAddInfo)) {
    //             continue;
    //         }
    //         // CommonJson::GetJsonStringValue(addLableItem, "name", lableAddInfo.name);
    //         // CommonJson::GetJsonStringValue(addLableItem, "value", lableAddInfo.value);
    //         // CommonJson::GetJsonIntValue(addLableItem, "type", lableAddInfo.type);
    //         // if (lableAddInfo.type < 0 || lableAddInfo.name.empty())
    //         // {
    //         //     LogWarn("skip invalid add_labels item,type={},name={}",
    //         //              lableAddInfo.type,
    //         //              lableAddInfo.name.c_str());
    //         //     continue;
    //         // }
    //         labelAddInfos.push_back(lableAddInfo);
    //     }
    //     item.labelAddInfos = labelAddInfos;
    // }
    // CommonJson::GetJsonIntValue(value, "count", item.topN);
    item.topN = value.getNumber<int>("count");
    SingletonTaskManager::Instance()->SetTopNItem(item);
    return 0;
}
void TaskMonitor::ParseHttpInfo(const json::Object &jsonValue)
{
    // string key = "HTTP";
    // Json::Value value = jsonValue.get(key, Json::Value::null);
    // if (value.isNull() || !value.isArray())
    // {
    //     LogWarn("no HTTP in the response json!");
    //     return;
    // }
    json::Array value = jsonValue.getArray("HTTP");
    if (value.isNull()) {
        LogWarn("no HTTP in the response json");
        return;
    }

    auto httpItems = std::make_shared<std::unordered_map<std::string, HttpItem>>();
    for (size_t i = 0; i < value.size(); i++)
    {
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isString())
        // {
        //     continue;
        // }
        // string itemStr = item.asString();
        std::string itemStr = value[i].asString();
        std::string error;
        json::Object itemValue = json::parseObject(itemStr, error);
        if (itemValue.isNull()) {
            LogWarn("the http item is invalid: {}, content: {}", itemStr, error);
            continue;
        }
        // JsonValue jsonValueItem;
        // if (!jsonValueItem.parse(itemStr))
        // {
        //     LogWarn("the http item({}) is not json!", itemStr.c_str());
        //     continue;
        // }
        // Json::Value itemValue = jsonValueItem.getValue();
        // if (!itemValue.isObject())
        // {
        //     LogWarn("the http item({}) is not object!", itemStr.c_str());
        //     continue;
        // }
        HttpItem httpItem;
        // CommonJson::GetJsonUInt64Value(itemValue, "taskId", taskId);
        // httpItem.taskId = StringUtils::NumberToString(taskId);
        httpItem.taskId = itemValue.getString("taskId");
        httpItem.effective = itemValue.getString("effective");
        int64_t interval = itemValue.getNumber("interval", ToSeconds(httpItem.interval));
        // if (interval > 0) {
        //     // if (interval < MIN_DETECT_SCHEDULE_INTERVAL) {
        //     //     interval = MIN_DETECT_SCHEDULE_INTERVAL;
        //     //     LogWarn("The minimum http scheduling interval is {} s", MIN_DETECT_SCHEDULE_INTERVAL);
        //     // }
        //     httpItem.interval = std::max({ interval, MIN_DETECT_SCHEDULE_INTERVAL });
        // }
        httpItem.interval = correctDetectInterval(interval);
        int timeout = itemValue.getNumber<int>("timeout", 10);
        if (timeout <= MIN_DETECT_HTTP_TIMEOUT || MAX_DETECT_HTTP_TIMEOUT < timeout) {
            LogWarn("http timeout is {}s exceeded ({}, {}], Set to default 10s",
                    timeout, MIN_DETECT_HTTP_TIMEOUT, MAX_DETECT_HTTP_TIMEOUT);
            timeout = 10;
        }
        httpItem.timeout = std::chrono::seconds{timeout}; // .interval = interval;
        // CommonJson::GetJsonStringValue(itemValue, "uri", httpItem.uri);
        httpItem.uri = itemValue.getString("uri");
        // CommonJson::GetJsonStringValue(itemValue, "method", httpItem.method);
        httpItem.method = itemValue.getString("method");
        // CommonJson::GetJsonStringValue(itemValue, "requestBody", httpItem.requestBody);
        httpItem.requestBody = itemValue.getString("requestBody");
        // CommonJson::GetJsonStringValue(itemValue, "header", httpItem.header);
        httpItem.header = itemValue.getString("header");
        // CommonJson::GetJsonStringValue(itemValue, "keyword", httpItem.keyword);
        httpItem.keyword = itemValue.getString("keyword");
        // CommonJson::GetJsonBoolValue(itemValue, "negative", httpItem.negative);
        httpItem.negative = itemValue.getBool("negative", httpItem.negative);
        (*httpItems)[httpItem.taskId] = httpItem;
    }
    LogDebug("http item size : {}", value.size());
    LogDebug("http item      : {}", value.toString());
    SingletonTaskManager::Instance()->SetHttpItems(httpItems);
}

void TaskMonitor::ParsePingInfo(const json::Object &jsonValue)
{
    // string key = "PING";
    // Json::Value value = jsonValue.get(key, Json::Value::null);
    // if (value.isNull() || !value.isArray())
    // {
    //     LogWarn("no PING in the response json!");
    //     return;
    // }
    json::Array value = jsonValue.getArray("PING");
    if (value.isNull()) {
        LogWarn("no PING in the response json");
        return ;
    }

    auto pingItems = std::make_shared<std::unordered_map<std::string, PingItem>>();
    for (size_t i = 0; i < value.size(); i++)
    {
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isString())
        // {
        //     continue;
        // }
        // string itemStr = item.asString();
        // JsonValue jsonValueItem;
        // if (!jsonValueItem.parse(itemStr))
        // {
        //     LogWarn("the ping item({}) is not json!", itemStr.c_str());
        //     continue;
        // }
        // Json::Value itemValue = jsonValueItem.getValue();
        // if (!itemValue.isObject())
        // {
        //     LogWarn("the ping item({}) is not object!", itemStr.c_str());
        //     continue;
        // }
        std::string error;
        json::Object itemValue = json::parseObject(value[i].asString(), error);
        if (itemValue.isNull()) {
            LogWarn("the ping item is invalid: {}, item: {}", error, value[i].asString());
            continue;
        }

        PingItem pingItem;
        // uint64_t taskId;
        // CommonJson::GetJsonUInt64Value(itemValue, "taskId", taskId);
        // pingItem.taskId = StringUtils::NumberToString((uint64_t) taskId);
        pingItem.taskId = itemValue.getString("taskId");
        pingItem.effective = itemValue.getString("effective");
        int interval = itemValue.getNumber("interval", 60);
        // if (interval > 0) {
        pingItem.interval = correctDetectInterval(interval);
        // }
        // CommonJson::GetJsonStringValue(itemValue, "host", pingItem.host);
        pingItem.host = itemValue.getString("host");
        (*pingItems)[pingItem.taskId] = pingItem;
    }
    LogDebug("ping item size : {}", value.size());
    LogDebug("ping item      : {}", value.toString());
    SingletonTaskManager::Instance()->SetPingItems(pingItems);
}
void TaskMonitor::ParseTelnetInfo(const json::Object &jsonValue)
{
    // string key = "TELNET";
    // Json::Value value = jsonValue.get(key, Json::Value::null);
    // if (value.isNull() || !value.isArray())
    // {
    //     LogWarn("no TELNET in the response json!");
    //     return;
    // }
    json::Array value = jsonValue.getArray("TELNET");
    if (value.isNull()) {
        LogWarn("no TELNET in the response json");
        return;
    }
    auto telnetItems = std::make_shared<std::unordered_map<std::string, TelnetItem>>();
    for (size_t i = 0; i < value.size(); i++) {
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isString())
        // {
        //     continue;
        // }
        // string itemStr = item.asString();
        // JsonValue jsonValueItem;
        // if (!jsonValueItem.parse(itemStr))
        // {
        //     LogWarn("the telnet item({}) is not json!", itemStr.c_str());
        //     continue;
        // }
        // Json::Value itemValue = jsonValueItem.getValue();
        // if (!itemValue.isObject())
        // {
        //     LogWarn("the telnet item({}) is not object!", itemStr.c_str());
        //     continue;
        // }
        std::string error;
        json::Object itemValue = json::parseObject(value[i].asString(), error);
        if (itemValue.isNull()) {
            LogWarn("the telnet item is invalid: {}, item: {}", error, value[i].asString());
            continue;
        }

        TelnetItem telnetItem;
        // uint64_t taskId;
        // CommonJson::GetJsonUInt64Value(itemValue, "taskId", taskId);
        // telnetItem.taskId = StringUtils::NumberToString((uint64_t) taskId);
        telnetItem.taskId = itemValue.getString("taskId");
        telnetItem.effective = itemValue.getString("effective");
        int interval = itemValue.getNumber("interval", 60);
        // if (interval > 0) { // CommonJson::GetJsonIntValue(itemValue, "interval", interval))
        //     // if (interval < MIN_DETECT_SCHEDULE_INTERVAL)
        //     // {
        //     //     interval = MIN_DETECT_SCHEDULE_INTERVAL;
        //     //     LogWarn("The minimum telnet scheduling interval is {} s !",
        //     //              MIN_DETECT_SCHEDULE_INTERVAL);
        //     // }
        //     telnetItem.interval = std::max(interval, MIN_DETECT_SCHEDULE_INTERVAL);
        // }
        telnetItem.interval = correctDetectInterval(interval);
        // CommonJson::GetJsonStringValue(itemValue, "uri", telnetItem.uri);
        telnetItem.uri = itemValue.getString("uri");
        // CommonJson::GetJsonStringValue(itemValue, "requestBody", telnetItem.requestBody);
        telnetItem.requestBody = itemValue.getString("requestBody");
        // CommonJson::GetJsonStringValue(itemValue, "keyword", telnetItem.keyword);
        telnetItem.keyword = itemValue.getString("keyword");
        // CommonJson::GetJsonBoolValue(itemValue, "negative", telnetItem.negative);
        telnetItem.negative = itemValue.getBool("negative", telnetItem.negative);
        (*telnetItems)[telnetItem.taskId] = telnetItem;
    }
    LogDebug("telnet item size : {}", value.size());
    LogDebug("telnet item      : {}", value.toString());
    SingletonTaskManager::Instance()->SetTelnetItems(telnetItems);
}

std::vector<TagItem> parseTags(const json::Array &root) {
    std::vector<TagItem> tags;
    if (!root.empty()) {
        tags.reserve(root.size());
        root.forEach([&](size_t idx, const json::Object &item){
            TagItem tag(item.getString("k"), item.getString("v"));
            if (!tag.key.empty()) {
                tags.push_back(tag);
            }
        });
    }
    RETURN_RVALUE(tags);
}

// {"collectConfig":{"processNames":["java"],"processConfigs":[{"id":278567,"instanceId":"i-bp1ix2sgagse5adct6au","name":"java","command":"number"}],"httpConfigs":[]}}
void TaskMonitor::ParseProcessInfo(const json::Object &jsonValue)
{
    json::Object collectConfigValue = jsonValue.getObject("collectConfig");
    // string key = "collectConfig";
    // Json::Value collectConfigValue = jsonValue.get(key, Json::Value::null);
    // if (collectConfigValue.isNull() || !collectConfigValue.isObject())
    // {
    //     LogWarn("no collectConfig in the response json!");
    //     return;
    // }
    if (collectConfigValue.isNull()) {
        LogWarn("no collectConfig in the response json!");
        return;
    }
    // key = "processConfigs";
    // Json::Value value = collectConfigValue.get(key, Json::Value::null);
    // if (value.isNull() || !value.isArray())
    // {
    //     LogWarn("no processConfigs in the response json!");
    //     return;
    // }
    json::Array value = collectConfigValue.getArray("processConfigs");
    if (value.isNull()) {
        LogWarn("no processConfigs in the response json!");
        return;
    }

    const size_t count = value.size();
    auto processCollectItems = std::make_shared<vector<ProcessCollectItem>>();
    processCollectItems->reserve(count);
    for (size_t i = 0; i < count; i++)
    {
        json::Object item = value[i].asObject();
        if (item.isNull()) {
            continue;
        }
        // Json::Value item = value.get(i, Json::Value::null);
        // if (item.isNull() || !item.isObject())
        // {
        //     continue;
        // }
        ProcessCollectItem processCollectItem;
        // CommonJson::GetJsonStringValue(item, "name", processCollectItem.name);
        // CommonJson::GetJsonStringValue(item, "command", processCollectItem.command);
        // processCollectItem.command = item.getString("command");
        if (item.getString("command") != "number") {
            continue;
        }
        processCollectItem.name = item.getString("name");
        processCollectItem.processName = item.getString("processName");
        processCollectItem.processUser = item.getString("processUser");
        processCollectItem.tags = parseTags(item.getArray("tags"));

        if (!processCollectItem.isEmpty()) {
            processCollectItems->push_back(processCollectItem);
        }
    }
    SingletonTaskManager::Instance()->SetProcessCollectItems(processCollectItems);
}

/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// mid 相关
// template<typename T>
// std::vector<const T *> copyPtr(const std::vector<T> &columns) {
//     std::vector<const T *> vec;
//     vec.reserve(columns.size());
//     for (const auto &it: columns) {
//         vec.push_back(&it);
//     }
//     RETURN_RVALUE(vec);
// }
//
// template<typename T>
// void sortPtr(std::vector<const T *> &columns) {
//     std::sort(columns.begin(), columns.end(), [](const T *a, const T *b) {return *a < *b; });
// }
//
// typedef std::map<std::string, std::string>::value_type StringPair;
// template<> void sortPtr(std::vector<const StringPair *> &columns) {
//     std::sort(columns.begin(), columns.end(), [](const StringPair *a, const StringPair *b) {
//         return a->first > b->first;
//     });
// }
//
// template<typename T>
// std::vector<const T *> copySortedPtr(const std::vector<T> &columns) {
//     std::vector<const T *> sorted = copyPtr(columns);
//     sortPtr(sorted);
//     RETURN_RVALUE(sorted);
// }
//
// std::vector<const StringPair *> copySortedPtr(const std::map<std::string, std::string> &columns) {
//     std::vector<const StringPair *> sorted;
//     sorted.reserve(columns.size());
//     for (const auto &it: columns) {
//         sorted.push_back(&it);
//     }
//     sortPtr(sorted);
//     RETURN_RVALUE(sorted);
// }
//
// struct MidAssembler {
//     static std::string assemble(const std::vector<std::pair<std::string, std::string>> &vec) {
//         size_t size = 0;
//         for(const auto &pair: vec) {
//             size += pair.first.size() + pair.second.size();
//         }
//
//         std::string total;
//         total.reserve(size);
//         for (const auto *pair: copySortedPtr(vec)) {
//             total += pair->first;
//             total += pair->second;
//         }
//         return total;
//     }
//     static std::string assemble(const std::vector<LabelAddInfo> &labels) {
//         std::string total;
//         for (const auto &itLabelAddInfo: copySortedPtr(labels)) {
//             total += itLabelAddInfo->name;
//             total += itLabelAddInfo->value;
//             total += StringUtils::NumberToString(itLabelAddInfo->type);
//         }
//         return total;
//     }
//
//     static std::string assemble(const std::vector<MetricFilterInfo> &vec) {
//         std::string total;
//         for (const auto *itMetricFilterInfo: copySortedPtr(vec)) {
//             total += itMetricFilterInfo->name;
//             total += itMetricFilterInfo->metricName;
//             for (const auto *itTag: copySortedPtr(itMetricFilterInfo->tagMap)) {
//                 total += itTag->first;
//                 total += itTag->second;
//             }
//         }
//         return total;
//     }
// };
//
// string TaskMonitor::GetModuleItemMid(const ModuleItem & item)
// {
//     string total = item.name;
//     total += item.scheduleExpr;
//     total += item.collectUrl;
//     total += StringUtils::NumberToString(item.scheduleInterval.count());
//     total += item.moduleArgs;
//     total += item.moduleVersion;
//     total += item.module;
//     total += "isSoType"+StringUtils::NumberToString(item.isSoType);
//     // vector<pair<string,string>> outputVector=item.outputVector;
//     // sort(outputVector.begin(),outputVector.end(),[](const pair<string,string> &x, const pair<string,string> &y){return x.first>y.first;});
//     // for(const auto& itOutput: outputVector){
//     //     total += itOutput.first;
//     //     total += itOutput.second;
//     // }
//     total += MidAssembler::assemble(item.outputVector);
//     string md5 = MD5String(total.c_str(), total.size());
//     return md5;
// }
//
// string TaskMonitor::CalcMid(const ScriptItem &item) {
//     string total = item.name;
//     total += StringUtils::NumberToString(item.protocolVersion);
//     total += item.scheduleExpr;
//     total += item.collectUrl;
//     total += StringUtils::NumberToString(item.timeOut.count());
//     total += StringUtils::NumberToString(item.scheduleIntv.count());
//     total += item.scriptUser;
//     total += StringUtils::NumberToString(item.duration.count());
//     total += StringUtils::NumberToString(ToSeconds(item.firstSche));
//     total += "reportStatus" + StringUtils::NumberToString(item.reportStatus);
//     //total += StringUtils::NumberToString(item.isAliScript);
//     total += "resultFormat" + StringUtils::NumberToString(item.resultFormat);
//     // vector<pair<string,string>> outputVector=item.outputVector;
//     // sort(outputVector.begin(),outputVector.end(),[](const pair<string,string> &x,const pair<string,string> &y){return x.first>y.first;});
//     // for(const auto& itOutput: outputVector){
//     //     total += itOutput.first;
//     //     total += itOutput.second;
//     // }
//     total += MidAssembler::assemble(item.outputVector);
//     // vector<LabelAddInfo> labelAddInfos = item.labelAddInfos;
//     // sort(labelAddInfos.begin(),labelAddInfos.end());
//     // for(const auto& itLabelAddInfo :labelAddInfos){
//     //     total += itLabelAddInfo.name;
//     //     total += itLabelAddInfo.value;
//     //     total += StringUtils::NumberToString(itLabelAddInfo.type);
//     // }
//     total += MidAssembler::assemble(item.labelAddInfos);
//     // vector<MetricFilterInfo> metricFilterInfos = item.metricFilterInfos;
//     // sort(metricFilterInfos.begin(),metricFilterInfos.end());
//     // for(const auto& itMetricFilterInfo :metricFilterInfos){
//     //     total += itMetricFilterInfo.name;
//     //     total += itMetricFilterInfo.metricName;
//     //     vector<pair<string, string>> tags;
//     //     tags.reserve(itMetricFilterInfo.tagMap.size());
//     //     for (auto itTag: itMetricFilterInfo.tagMap) {
//     //         tags.emplace_back(itTag);
//     //     }
//     //     sort(tags.begin(), tags.end(), [](const pair<string, string> &x, const pair<string, string> &y) {
//     //         return x.first > y.first;
//     //     });
//     //     for(const auto& itTag :tags){
//     //         total += itTag.first;
//     //         total += itTag.second;
//     //     }
//     // }
//     total += MidAssembler::assemble(item.metricFilterInfos);
//     // vector<MetricMeta> metricMetas = item.metricMetas;
//     // sort(metricMetas.begin(),metricMetas.end());
//     for (const auto *itMetricMeta: copySortedPtr(item.metricMetas)) {
//         total += itMetricMeta->name;
//         total += "isMetric" + StringUtils::NumberToString(itMetricMeta->isMetric);
//         total += StringUtils::NumberToString(itMetricMeta->type);
//     }
//
//     string md5 = MD5String(total.c_str(), total.size());
//     return md5;
// }
//
// string TaskMonitor::GetExporterItemMid(const ExporterItem &item) {
//     string total = item.name;
//     total += item.target;
//     total += item.metricsPath;
//     total += StringUtils::NumberToString(item.interval.count());
//     total += StringUtils::NumberToString(item.timeout.count());
//     total += StringUtils::NumberToString(item.addStatus);
//     total += StringUtils::NumberToString(item.type);
//     // vector<pair<string,string>> outputVector=item.outputVector;
//     // sort(outputVector.begin(),outputVector.end(),[](const pair<string,string> &x,const pair<string,string> &y){return x.first>y.first;});
//     // for(const auto& itOutput: outputVector){
//     //     total += itOutput.first;
//     //     total += itOutput.second;
//     // }
//     total += MidAssembler::assemble(item.outputVector);
//     // vector<LabelAddInfo> labelAddInfos = item.labelAddInfos;
//     // sort(labelAddInfos.begin(),labelAddInfos.end());
//     // for(const auto& itLabelAddInfo :labelAddInfos){
//     //     total += itLabelAddInfo.name;
//     //     total += itLabelAddInfo.value;
//     //     total += StringUtils::NumberToString(itLabelAddInfo.type);
//     // }
//     total += MidAssembler::assemble(item.labelAddInfos);
//     // vector<MetricFilterInfo> metricFilterInfos = item.metricFilterInfos;
//     // sort(metricFilterInfos.begin(),metricFilterInfos.end());
//     // for(const auto& itMetricFilterInfo :metricFilterInfos){
//     //     total += itMetricFilterInfo.name;
//     //     total += itMetricFilterInfo.metricName;
//     //     vector<pair<string,string>> tags;
//     //     tags.reserve(itMetricFilterInfo.tagMap.size());
//     //     for (auto itTag: itMetricFilterInfo.tagMap) {
//     //         tags.emplace_back(itTag);
//     //     }
//     //     sort(tags.begin(), tags.end(), [](const pair<string, string> &x, const pair<string, string> &y) {
//     //         return x.first > y.first;
//     //     });
//     //     for(const auto& itTag :tags){
//     //         total += itTag.first;
//     //         total += itTag.second;
//     //     }
//     // }
//     total += MidAssembler::assemble(item.metricFilterInfos);
//     string md5 = MD5String(total.c_str(), total.size());
//     return md5;
// }
} // namespace argus