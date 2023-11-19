// Copyright 2022 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "ConfigManager.h"
#include <curl/curl.h>
#include <cctype>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(__linux__)
#include <unistd.h>
#include <fnmatch.h>
#endif
#include <limits.h>
#include <unordered_map>
#include <unordered_set>
#include <re2/re2.h>
#include <set>
#include <vector>
#include <fstream>
#include "common/JsonUtil.h"
#include "common/HashUtil.h"
#include "common/RuntimeUtil.h"
#include "common/FileSystemUtil.h"
#include "common/Constants.h"
#include "common/ExceptionBase.h"
#include "common/CompressTools.h"
#include "common/ErrorUtil.h"
#include "common/TimeUtil.h"
#include "common/StringTools.h"
#include "common/version.h"
#include "config/UserLogConfigParser.h"
#include "monitor/LogtailAlarm.h"
#include "monitor/LogFileProfiler.h"
#include "monitor/LogIntegrity.h"
#include "monitor/LogLineCount.h"
#include "app_config/AppConfig.h"
#include "checkpoint/CheckPointManager.h"
#include "event_handler/EventHandler.h"
#include "controller/EventDispatcher.h"
#include "sender/Sender.h"
#include "processor/daemon/LogProcess.h"
#include "file_server/FileServer.h"
#include "pipeline/Pipeline.h"
#include "pipeline/PipelineManager.h"

using namespace std;
using namespace logtail;

DEFINE_FLAG_BOOL(https_verify_peer, "set CURLOPT_SSL_VERIFYPEER, CURLOPT_SSL_VERIFYHOST option for libcurl", true);
#if defined(__linux__) || defined(__APPLE__)
DEFINE_FLAG_STRING(https_ca_cert, "set CURLOPT_CAINFO for libcurl", "ca-bundle.crt");
#elif defined(_MSC_VER)
DEFINE_FLAG_STRING(https_ca_cert, "set CURLOPT_CAINFO for libcurl", "cacert.pem");
#endif
DEFINE_FLAG_STRING(default_global_topic, "default is empty string", "");
// DEFINE_FLAG_STRING(profile_project_name, "profile project_name for logtail", "");
DEFINE_FLAG_INT32(request_access_key_interval, "control the frenquency of GetAccessKey, seconds", 60);
DEFINE_FLAG_INT32(logtail_sys_conf_update_interval, "control the frenquency of load local machine conf, seconds", 60);
DEFINE_FLAG_INT32(wildcard_max_sub_dir_count, "", 1000);
DEFINE_FLAG_INT32(config_match_max_cache_size, "", 1000000);
DEFINE_FLAG_INT32(multi_config_alarm_interval, "second", 600);
DECLARE_FLAG_INT32(delay_bytes_upperlimit);
DECLARE_FLAG_BOOL(global_network_success);
DECLARE_FLAG_STRING(ilogtail_user_defined_id_env_name);
DECLARE_FLAG_STRING(ilogtail_aliuid_env_name);
DEFINE_FLAG_STRING(ilogtail_docker_file_path_config, "ilogtail docker path config file", "docker_path_config.json");
DEFINE_FLAG_STRING(ilogtail_docker_path_version, "ilogtail docker path config file", "0.1.0");
DEFINE_FLAG_INT32(max_docker_config_update_times, "max times docker config update in 3 minutes", 10);
DEFINE_FLAG_INT32(docker_config_update_interval, "interval between docker config updates, seconds", 3);
DEFINE_FLAG_STRING(default_data_integrity_project, "default data integrity project", "data_integrity");
DEFINE_FLAG_STRING(default_data_integrity_log_store, "default data integrity log store", "data_integrity");
DEFINE_FLAG_INT32(default_data_integrity_time_pos, "default data integrity time pos", 0);
DEFINE_FLAG_STRING(default_log_time_reg,
                   "default log time reg",
                   "([0-9]{4})-(0[0-9]{1}|1[0-2])-(0[0-9]{1}|[12][0-9]{1}|3[01]) "
                   "(0[0-9]{1}|1[0-9]{1}|2[0-3]):[0-5][0-9]{1}:([0-5][0-9]{1})");
DEFINE_FLAG_STRING(default_line_count_project, "default line count project", "line_count");
DEFINE_FLAG_STRING(default_line_count_log_store, "default line count log store", "line_count");
DEFINE_FLAG_BOOL(default_fuse_mode, "default fuse mode", false);
DEFINE_FLAG_BOOL(default_mark_offset_flag, "default mark offset flag", false);
DEFINE_FLAG_BOOL(default_check_ulogfs_env, "default check ulogfs env", false);
DEFINE_FLAG_INT32(default_max_depth_from_root, "default max depth from root", 1000);
// DEFINE_FLAG_INT32(default_plugin_log_queue_size, "", 10);
DEFINE_FLAG_STRING(fuse_customized_config_name,
                   "name of specified config for fuse, should not be used by user",
                   "__FUSE_CUSTOMIZED_CONFIG__");
DEFINE_FLAG_BOOL(logtail_config_update_enable, "", true);

DECLARE_FLAG_BOOL(rapid_retry_update_config);
DECLARE_FLAG_BOOL(default_global_fuse_mode);
DECLARE_FLAG_BOOL(default_global_mark_offset_flag);
DECLARE_FLAG_BOOL(enable_collection_mark);
DECLARE_FLAG_BOOL(enable_env_ref_in_config);
DECLARE_FLAG_BOOL(sls_client_send_compress);
DECLARE_FLAG_INT32(default_tail_limit_kb);
DECLARE_FLAG_INT32(default_plugin_log_queue_size);

namespace logtail {

ParseConfResult ParseConfig(const std::string& configName, Json::Value& jsonRoot) {
    // Get full path, if it is a relative path, prepend process execution dir.
    std::string fullPath = configName;
    if (IsRelativePath(fullPath)) {
        fullPath = GetProcessExecutionDir() + configName;
    }

    ifstream is;
    is.open(fullPath.c_str());
    if (!is.good()) {
        return CONFIG_NOT_EXIST;
    }
    std::string buffer((std::istreambuf_iterator<char>(is)), (std::istreambuf_iterator<char>()));
    if (!IsValidJson(buffer.c_str(), buffer.length())) {
        return CONFIG_INVALID_FORMAT;
    }

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    std::unique_ptr<Json::CharReader> jsonReader(builder.newCharReader());
    std::string jsonParseErrs;
    if (!jsonReader->parse(buffer.data(), buffer.data() + buffer.size(), &jsonRoot, &jsonParseErrs)) {
        LOG_WARNING(sLogger, ("ConfigName", configName)("ParseConfig error", jsonParseErrs));
        return CONFIG_INVALID_FORMAT;
    }
    return CONFIG_OK;
}

// LoadSingleUserConfig constructs new Config object according to @value, and insert it into
// mNameConfigMap with name @logName.
// void ConfigManager::LoadSingleUserConfig(const std::string& logName, const Json::Value& rawValue, bool localFlag) {
//     // // MIX_PROCESS_MODE used to tag the config using CGO interface to process logs.
//     // // Different value maybe means different optimizing strategies, such as adjust the size of channel between
//     // // goroutines. But because of historical compatibility, the raw logs would not transfer this flag. The golang
//     // part
//     // // should retain no flag scenario as the default flag.
//     // static const std::string MIX_PROCESS_MODE = "mix_process_mode";
//     Config* config = NULL;
//     string projectName, category, errorMessage;
//     LOG_DEBUG(sLogger, ("message", "load single user config")("json", rawValue.toStyledString()));
//     const Json::Value* valuePtr = &rawValue;
//     // Json::Value replacedValue = rawValue;
//     // if (BOOL_FLAG(enable_env_ref_in_config)) {
//     //     // replace environment variable reference in config string
//     //     ReplaceEnvVarRefInConf(replacedValue);
//     //     valuePtr = &replacedValue;
//     //     LOG_DEBUG(
//     //         sLogger,
//     //         ("message", "user config after environment variable replacement")("json", replacedValue.toStyledString()));
//     // }
//     const Json::Value& value = *valuePtr;
//     try {
//         if (value["enable"].asBool()) {
//             int version = GetIntValue(value, USER_CONFIG_VERSION, 0);
//             if (version == -1) {
//                 return;
//             }
//             // projectName = GetStringValue(value, "project_name", "");
//             // category = GetStringValue(value, "category", "");
//             string logTypeStr = GetStringValue(value, "log_type", "plugin");
//             // auto region = GetStringValue(value, "region", AppConfig::GetInstance()->GetDefaultRegion());
//             LogType logType;
//             if (!CheckLogType(logTypeStr, logType)) {
//                 throw ExceptionBase(std::string("The logType is invalid : ") + logTypeStr);
//             }
//             bool discardUnmatch = GetBoolValue(value, "discard_unmatch", true);

//             // // this field is for ant
//             // // all configuration are included in "customized" field
//             // bool dataIntegritySwitch = false;
//             // string dataIntegrityProjectName, dataIntegrityLogstore, logTimeReg;
//             // int32_t timePos = 0;
//             // bool lineCountSwitch = false;
//             // string lineCountProjectName, lineCountLogstore;
//             // bool isFuseMode = BOOL_FLAG(default_global_fuse_mode);
//             // bool markOffsetFlag = BOOL_FLAG(default_global_mark_offset_flag);
//             // bool collectBackwardTillBootTime = false;
//             // if (value.isMember("customized_fields") && value["customized_fields"].isObject()) {
//             //     // parse data integrity and line count fields
//             //     const Json::Value& customizedFieldsValue = value["customized_fields"];
//             //     if (customizedFieldsValue.isMember("data_integrity")
//             //         && customizedFieldsValue["data_integrity"].isObject()) {
//             //         const Json::Value& dataIntegrityValue = customizedFieldsValue["data_integrity"];
//             //         dataIntegritySwitch = GetBoolValue(dataIntegrityValue, "switch", false);
//             //         if (dataIntegritySwitch) {
//             //             dataIntegrityProjectName = GetStringValue(
//             //                 dataIntegrityValue, "project_name", STRING_FLAG(default_data_integrity_project));
//             //             dataIntegrityLogstore = GetStringValue(
//             //                 dataIntegrityValue, "logstore", STRING_FLAG(default_data_integrity_log_store));
//             //             logTimeReg
//             //                 = GetStringValue(dataIntegrityValue, "log_time_reg", STRING_FLAG(default_log_time_reg));
//             //             timePos
//             //                 = GetIntValue(dataIntegrityValue, "time_pos",
//             //                 INT32_FLAG(default_data_integrity_time_pos));
//             //         }
//             //     }
//             //     if (customizedFieldsValue.isMember("line_count") && customizedFieldsValue["line_count"].isObject()) {
//             //         const Json::Value& lineCountValue = customizedFieldsValue["line_count"];
//             //         lineCountSwitch = GetBoolValue(lineCountValue, "switch", false);
//             //         if (lineCountSwitch) {
//             //             lineCountProjectName
//             //                 = GetStringValue(lineCountValue, "project_name",
//             //                 STRING_FLAG(default_line_count_project));
//             //             lineCountLogstore
//             //                 = GetStringValue(lineCountValue, "logstore", STRING_FLAG(default_line_count_log_store));
//             //         }
//             //     }

//             //     if (customizedFieldsValue.isMember("check_ulogfs_env")
//             //         && customizedFieldsValue["check_ulogfs_env"].isBool()) {
//             //         bool checkUlogfsEnv
//             //             = GetBoolValue(customizedFieldsValue, "check_ulogfs_env",
//             //             BOOL_FLAG(default_check_ulogfs_env));
//             //         bool hasCollectionMarkFileFlag
//             //             = BOOL_FLAG(enable_collection_mark) ? GetCollectionMarkFileExistFlag() : false;
//             //         if (checkUlogfsEnv && !hasCollectionMarkFileFlag) {
//             //             // only set in pod's app container
//             //             const char* ulogfsEnabledEnv = getenv("ULOGFS_ENABLED");
//             //             if (ulogfsEnabledEnv) {
//             //                 if (strcmp(ulogfsEnabledEnv, "true") == 0) {
//             //                     LOG_WARNING(sLogger,
//             //                                 ("load conifg", logName)("skip config", category)("project",
//             //                                 projectName)(
//             //                                     "check_ulogfs_env", "true")("env of ULOGFS_ENABLED", "true"));
//             //                     // if ULOGFS_ENABLED in env is true and check_ulogfs_env in config json is true
//             //                     // it means this logtail instance should skip this fuse config, we should no load it
//             //                     return;
//             //                 }
//             //             }
//             //         }
//             //     }

//             //     isFuseMode
//             //         = isFuseMode && GetBoolValue(customizedFieldsValue, "fuse_mode", BOOL_FLAG(default_fuse_mode));
//             //     markOffsetFlag = markOffsetFlag && GetBoolValue(customizedFieldsValue, "mark_offset", false);
//             //     collectBackwardTillBootTime
//             //         = GetBoolValue(customizedFieldsValue, "collect_backward_till_boot_time", false);
//             // }

//             // string pluginConfig;
//             // bool flusher_exists = false;
//             // if (value.isMember("plugin")) {
//             //     if (value["plugin"].isObject()) {
//             //         pluginConfig = value["plugin"].toStyledString();
//             //         if (value["plugin"].isMember("flushers")) {
//             //             flusher_exists = true;
//             //         }
//             //     } else if (value["plugin"].isString()) {
//             //         pluginConfig = value["plugin"].asString();
//             //         if (pluginConfig.find("\"flushers\"")) {
//             //             flusher_exists = true;
//             //         }
//             //     }
//             // }
//             // if ((projectName == "" || category == "") && !flusher_exists) {
//             //     throw ExceptionBase(std::string("Neither project/logstore or flusher exists"));
//             // }
//             // // prevent invalid input like "{}" "null" ...
//             // if (pluginConfig.size() < 20) {
//             //     pluginConfig.clear();
//             // }

//             // Json::Value pluginConfigJson;
//             // Json::CharReaderBuilder builder;
//             // builder["collectComments"] = false;
//             // std::unique_ptr<Json::CharReader> jsonReader(builder.newCharReader());
//             // std::string jsonParseErrs;
//             // if (!pluginConfig.empty()
//             //     && !jsonReader->parse(pluginConfig.data(),
//             //                           pluginConfig.data() + pluginConfig.size(),
//             //                           &pluginConfigJson,
//             //                           &jsonParseErrs)) {
//             //     LOG_WARNING(
//             //         sLogger,
//             //         ("invalid plugin config, plugin config json parse error", pluginConfig)("project", projectName)(
//             //             "logstore", category)("config", logName)("error", jsonParseErrs));
//             // }

//             if (logType == PLUGIN_LOG) {
//                 config = new Config("",
//                                     "",
//                                     logType,
//                                     logName,
//                                     "",
//                                     "",
//                                     "",
//                                     projectName,
//                                     false,
//                                     0,
//                                     0,
//                                     category,
//                                     false,
//                                     "",
//                                     discardUnmatch);
//                 if (pluginConfig.empty()) {
//                     throw ExceptionBase(std::string("The plugin log type is invalid"));
//                 }
//                 // if (!pluginConfigJson.isNull()) {
//                 //     config->mPluginProcessFlag = true;
//                 //     if (pluginConfig.find("\"observer_ilogtail_") != string::npos) {
//                 //         if (pluginConfigJson.isMember("inputs")) {
//                 //             if (pluginConfigJson["inputs"].isObject() || pluginConfigJson["inputs"].isArray()) {
//                 //                 config->mObserverConfig = pluginConfigJson["inputs"].toStyledString();
//                 //                 config->mObserverFlag = true;
//                 //                 pluginConfigJson.removeMember("inputs");
//                 //             }
//                 //             if (pluginConfigJson.isMember("processors")
//                 //                 && (pluginConfigJson["processors"].isObject()
//                 //                     || pluginConfigJson["processors"].isArray())) {
//                 //                 SetNotFoundJsonMember(pluginConfigJson, MIX_PROCESS_MODE, "observer");
//                 //             }
//                 //         } else {
//                 //             LOG_WARNING(sLogger,
//                 //                         ("observer config is not a legal JSON object",
//                 //                          logName)("project", projectName)("logstore", category));
//                 //             throw ExceptionBase(std::string("observer config is not a legal JSON object"));
//                 //         }
//                 //     }
//                 //     pluginConfigJson = ConfigManager::GetInstance()->CheckPluginProcessor(pluginConfigJson, value);
//                 //     pluginConfig = ConfigManager::GetInstance()->CheckPluginFlusher(pluginConfigJson);
//                 //     config->mPluginConfig = pluginConfig;
//                 // }
//             } else if (logType == STREAM_LOG) {
//                 config = new Config("",
//                                     "",
//                                     logType,
//                                     logName,
//                                     "",
//                                     "",
//                                     "",
//                                     projectName,
//                                     false,
//                                     0,
//                                     0,
//                                     category,
//                                     false,
//                                     GetStringValue(value, "tag"),
//                                     discardUnmatch);
//             } else {
//                 // bool isPreserve = GetBoolValue(value, "preserve", true);
//                 // int preserveDepth = 0;
//                 // if (!isPreserve) {
//                 //     preserveDepth = GetIntValue(value, "preserve_depth");
//                 // }
//                 // int maxDepth = -1;
//                 // if (value.isMember("max_depth"))
//                 //     maxDepth = GetIntValue(value, "max_depth");
//                 // string logPath = GetStringValue(value, "log_path");

//                 // 废弃
//                 // if (logPath.find(LOG_LOCAL_DEFINED_PATH_PREFIX) == 0) {
//                 //     mHaveMappingPathConfig = true;
//                 //     string tmpLogPath = GetMappingPath(logPath);
//                 //     if (!tmpLogPath.empty())
//                 //         logPath = tmpLogPath;
//                 // }

//                 // if (IsRelativePath(logPath)) {
//                 //     logPath = NormalizePath(AbsolutePath(logPath,
//                 //     AppConfig::GetInstance()->GetProcessExecutionDir()));
//                 // }

//                 // // one may still make mistakes, teminate logPath by '/'
//                 // size_t size = logPath.size();
//                 // if (size > 0 && PATH_SEPARATOR[0] == logPath[size - 1])
//                 //     logPath = logPath.substr(0, size - 1);

//                 // string logBeginReg = GetStringValue(value, "log_begin_reg", "");
//                 // if (logBeginReg != "" && CheckRegFormat(logBeginReg) == false) {
//                 //     throw ExceptionBase("The log begin line is not value regex : " + logBeginReg);
//                 // }
//                 // string logContinueReg = GetStringValue(value, "log_continue_reg", "");
//                 // if (logContinueReg != "" && CheckRegFormat(logContinueReg) == false) {
//                 //     throw ExceptionBase("The log continue line is not value regex : " + logContinueReg);
//                 // }
//                 // string logEndReg = GetStringValue(value, "log_end_reg", "");
//                 // if (logEndReg != "" && CheckRegFormat(logEndReg) == false) {
//                 //     throw ExceptionBase("The log end line is not value regex : " + logEndReg);
//                 // }
//                 // int readerFlushTimeout = 5;
//                 // if (value.isMember("reader_flush_timeout"))
//                 //     readerFlushTimeout = GetIntValue(value, "reader_flush_timeout");

//                 // string filePattern = GetStringValue(value, "file_pattern");
//                 // raw log flag
//                 bool rawLogFlag = false;
//                 if (value.isMember("raw_log") && value["raw_log"].isBool()) {
//                     rawLogFlag = value["raw_log"].asBool();
//                 }

//                 config = new Config(logPath,
//                                     filePattern,
//                                     logType,
//                                     logName,
//                                     logBeginReg,
//                                     logContinueReg,
//                                     logEndReg,
//                                     projectName,
//                                     isPreserve,
//                                     preserveDepth,
//                                     maxDepth,
//                                     category,
//                                     rawLogFlag,
//                                     "",
//                                     discardUnmatch,
//                                     readerFlushTimeout);

//                 // // normal log file config can have plugin too
//                 // // Boolean force_enable_pipeline.
//                 // if (value.isMember("force_enable_pipeline") && value["force_enable_pipeline"].isBool()
//                 //     && value["force_enable_pipeline"].asBool()) {
//                 //     config->mForceEnablePipeline = true;
//                 //     LOG_INFO(sLogger,
//                 //              ("set force enable pipeline",
//                 //               config->mForceEnablePipeline)("project", projectName)("config", logName));
//                 // }
//                 // if (!pluginConfig.empty() && !pluginConfigJson.isNull()) {
//                 //     if (pluginConfigJson.isMember("processors")
//                 //         && (pluginConfigJson["processors"].isObject() || pluginConfigJson["processors"].isArray())
//                 //         && !pluginConfigJson["processors"].empty()) {
//                 //         config->mPluginProcessFlag = true;
//                 //     }
//                 //     if (pluginConfigJson.isMember("flushers")
//                 //         && (pluginConfigJson["flushers"].isObject() || pluginConfigJson["flushers"].isArray())
//                 //         && !pluginConfigJson["flushers"].empty() &&
//                 //         IsMeaningfulFlusher(pluginConfigJson["flushers"])) { config->mPluginProcessFlag = true;
//                 //     }
//                 //     // check processors
//                 //     // set process flag when config have processors
//                 //     if (pluginConfigJson.isMember("processors")
//                 //         && (pluginConfigJson["processors"].isObject() || pluginConfigJson["processors"].isArray())) {
//                 //         // patch enable_log_position_meta to split processor if exists ...
//                 //         pluginConfigJson = ConfigManager::GetInstance()->CheckPluginProcessor(pluginConfigJson, value);
//                 //         pluginConfig = ConfigManager::GetInstance()->CheckPluginFlusher(pluginConfigJson);
//                 //     }
//                 //     config->mPluginConfig = pluginConfig;
//                 // }
//                 // if (value.isMember("docker_file") && value["docker_file"].isBool() && value["docker_file"].asBool())
//                 // {
//                 //     if (AppConfig::GetInstance()->IsPurageContainerMode()) {
//                 //         // 废弃
//                 //         // docker file is not supported in Logtail's container mode
//                 //         if (AppConfig::GetInstance()->IsContainerMode()) {
//                 //             throw ExceptionBase(
//                 //                 std::string("docker file is not supported in Logtail's container mode "));
//                 //         }

//                 //         // load saved container path
//                 //         auto iter = mAllDockerContainerPathMap.find(logName);
//                 //         if (iter != mAllDockerContainerPathMap.end()) {
//                 //             config->mDockerContainerPaths = iter->second;
//                 //             mAllDockerContainerPathMap.erase(iter);
//                 //         }
//                 //         if (!config->SetDockerFileFlag(true)) {
//                 //             // should not happen
//                 //             throw ExceptionBase(std::string("docker file do not support wildcard path"));
//                 //         }
//                 //         MappingPluginConfig(value, config, pluginConfigJson);
//                 //     } else {
//                 //         LOG_WARNING(sLogger,
//                 //                     ("config is docker_file mode, but logtail is not a purage container",
//                 //                      "the flag is ignored")("project", projectName)("logstore", category));
//                 //         LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM,
//                 //                                                string("config is docker_file mode, but logtail is not
//                 //                                                "
//                 //                                                       "a purage container, the flag is ignored"),
//                 //                                                projectName,
//                 //                                                category,
//                 //                                                region);
//                 //     }
//                 // }
//                 // 废弃
//                 // if (AppConfig::GetInstance()->IsContainerMode()) {
//                 //     // mapping config's path to real filePath
//                 //     // use docker file flag
//                 //     if (!config->SetDockerFileFlag(true)) {
//                 //         // should not happen
//                 //         throw ExceptionBase(std::string("docker file do not support wildcard path"));
//                 //     }
//                 //     string realPath;
//                 //     string basePath
//                 //         = config->mWildcardPaths.size() > (size_t)0 ? config->mWildcardPaths[0] : config->mBasePath;
//                 //     mDockerMountPathsLock.lock();
//                 //     bool findRst = mDockerMountPaths.FindBestMountPath(basePath, realPath);
//                 //     mDockerMountPathsLock.unlock();
//                 //     if (!findRst) {
//                 //         throw ExceptionBase(std::string("invalid mount path, basePath : ") + basePath);
//                 //     }

//                 //     // add containerPath
//                 //     DockerContainerPath containerPath;
//                 //     containerPath.mContainerPath = realPath;
//                 //     config->mDockerContainerPaths->push_back(containerPath);
//                 // }

//                 // config->mTopicFormat = GetStringValue(value, "topic_format", "default");
//                 // if (!config->mTopicFormat.empty()) {
//                 //     // Constant prefix to indicate use customized topic name,
//                 //     // it might be an invalid regex, so compare directly.
//                 //     const std::string customizedPrefix = "customized://";
//                 //     if (StartWith(config->mTopicFormat, customizedPrefix)) {
//                 //         config->mCustomizedTopic = config->mTopicFormat.substr(customizedPrefix.length());
//                 //         config->mTopicFormat = "customized";
//                 //     } else if (CheckTopicRegFormat(config->mTimeFormat) == false) {
//                 //         throw ExceptionBase("The topic format is not valid regex");
//                 //     }
//                 // }
//                 // string fileEncoding = GetStringValue(value, "file_encoding", "");
//                 // if (ToLowerCaseString(fileEncoding) == "gbk")
//                 //     config->mFileEncoding = ENCODING_GBK;
//                 // else
//                 //     config->mFileEncoding = ENCODING_UTF8;
//                 if (value.isMember("filter_keys") && value.isMember("filter_regs")) {
//                     config->mFilterRule.reset(GetFilterFule(value["filter_keys"], value["filter_regs"]));
//                 }
//                 // 废弃
//                 // if (value.isMember("dir_pattern_black_list")) {
//                 //     config->mUnAcceptDirPattern = GetStringVector(value["dir_pattern_black_list"]);
//                 // }

//                 // TODO: the following codes (line 673 - line 724) seem to be the same as the codes in line 816 - line
//                 // 874. Remove the following codes in the future.
//                 // if (value.isMember("merge_type") && value["merge_type"].isString()) {
//                 //     string mergeType = value["merge_type"].asString();

//                 //     if (mergeType == "logstore") {
//                 //         config->mMergeType = MERGE_BY_LOGSTORE;
//                 //         LOG_INFO(sLogger, ("set config merge type to MERGE_BY_LOGSTORE", config->mConfigName));
//                 //     } else {
//                 //         config->mMergeType = MERGE_BY_TOPIC;
//                 //     }
//                 // }

//                 if (value.isMember("tz_adjust") && value["tz_adjust"].isBool() && value.isMember("log_tz")
//                     && value["log_tz"].isString()) {
//                     string logTZ = value["log_tz"].asString();
//                     int logTZSecond = 0;
//                     bool adjustFlag = value["tz_adjust"].asBool();
//                     if (adjustFlag && !ParseTimeZoneOffsetSecond(logTZ, logTZSecond)) {
//                         LOG_ERROR(sLogger, ("invalid log time zone set", logTZ));
//                         config->mTimeZoneAdjust = false;
//                     } else {
//                         config->mTimeZoneAdjust = adjustFlag;
//                         config->mLogTimeZoneOffsetSecond = logTZSecond;
//                         if (adjustFlag) {
//                             LOG_INFO(
//                                 sLogger,
//                                 ("set log timezone adjust, project", config->mProjectName)(
//                                     "logstore", config->mCategory)("time zone", logTZ)("offset seconds", logTZSecond));
//                         }
//                     }
//                 }

//                 // create time
//                 // int32_t createTime = 0;
//                 // if (value.isMember("create_time") && value["create_time"].isInt()) {
//                 //     createTime = value["create_time"].asInt();
//                 // }
//                 // config->mCreateTime = createTime;

//                 // if (value.isMember("max_send_rate") && value["max_send_rate"].isInt()
//                 //     && value.isMember("send_rate_expire") && value["send_rate_expire"].isInt()) {
//                 //     int32_t maxSendBytesPerSecond = value["max_send_rate"].asInt();
//                 //     int32_t expireTime = value["send_rate_expire"].asInt();
//                 //     config->mMaxSendBytesPerSecond = maxSendBytesPerSecond;
//                 //     config->mSendRateExpireTime = expireTime;
//                 //     if (maxSendBytesPerSecond >= 0) {
//                 //         LOG_INFO(sLogger,
//                 //                  ("set logstore flow control, project", config->mProjectName)(
//                 //                      "logstore", config->mCategory)("max send byteps",
//                 //                      config->mMaxSendBytesPerSecond)( "expire", expireTime - (int32_t)time(NULL)));
//                 //     }
//                 //     Sender::Instance()->SetLogstoreFlowControl(config->mLogstoreKey, maxSendBytesPerSecond,
//                 //     expireTime);
//                 // }
//                 // config->mPriority = 0;
//                 // if (value.isMember("priority") && value["priority"].isInt()) {
//                 //     int32_t priority = value["priority"].asInt();
//                 //     if (priority < 0 || priority > MAX_CONFIG_PRIORITY_LEVEL) {
//                 //         LOG_ERROR(
//                 //             sLogger,
//                 //             ("invalid config priority, project", config->mProjectName)("logstore",
//                 //             config->mCategory));
//                 //     } else {
//                 //         config->mPriority = priority;
//                 //         if (priority > 0) {
//                 //             LogProcess::GetInstance()->SetPriorityWithHoldOn(config->mLogstoreKey, priority);
//                 //             LOG_INFO(sLogger,
//                 //                      ("set logstore priority, project",
//                 //                       config->mProjectName)("logstore", config->mCategory)("priority", priority));
//                 //         }
//                 //     }
//                 // }
//                 // if (config->mPriority == 0) {
//                 //     // if mPriority is 0, try to delete high level queue
//                 //     LogProcess::GetInstance()->DeletePriorityWithHoldOn(config->mLogstoreKey);
//                 // }

//                 if (value.isMember("sensitive_keys") && value["sensitive_keys"].isArray()) {
//                     GetSensitiveKeys(value["sensitive_keys"], config);
//                 }

//                 // if (value.isMember("delay_alarm_bytes") && value["delay_alarm_bytes"].isInt()) {
//                 //     config->mLogDelayAlarmBytes = value["delay_alarm_bytes"].asInt64();
//                 // }
//                 // if (config->mLogDelayAlarmBytes <= 0) {
//                 //     config->mLogDelayAlarmBytes = INT32_FLAG(delay_bytes_upperlimit);
//                 // }

//                 // if (value.isMember("delay_skip_bytes") && value["delay_skip_bytes"].isInt()) {
//                 //     config->mLogDelaySkipBytes = value["delay_skip_bytes"].asInt64();
//                 // }


//                 if (logType == REGEX_LOG) {
//                     config->mTimeFormat = GetStringValue(value, "timeformat", "");
//                     GetRegexAndKeys(value, config);
//                     if (config->mRegs && config->mKeys && config->mRegs->size() == (size_t)1
//                         && config->mKeys->size() == (size_t)1) {
//                         if ((!config->IsMultiline()) && *(config->mKeys->begin()) == DEFAULT_CONTENT_KEY
//                             && *(config->mRegs->begin()) == DEFAULT_REG) {
//                             LOG_DEBUG(sLogger,
//                                       ("config is simple mode", config->GetProjectName())(config->GetCategory(),
//                                                                                           config->mLogBeginReg));
//                             config->mSimpleLogFlag = true;
//                         }
//                     }
//                 }

//                 config->mTimeKey = GetStringValue(value, "time_key", "");

//                 // config->mTailExisted = GetBoolValue(value, "tail_existed", false);
//                 // 废弃
//                 // int32_t tailLimit = GetIntValue(value, "tail_limit", INT32_FLAG(default_tail_limit_kb));
//                 // config->SetTailLimit(tailLimit);
//             }

//             UserLogConfigParser::ParseAdvancedConfig(value, *config);
//             // if (pluginConfigJson.isMember("global")) {
//             //     SetNotFoundJsonMember(pluginConfigJson["global"],
//             //                           "EnableTimestampNanosecond",
//             //                           config->mAdvancedConfig.mEnableTimestampNanosecond);
//             //     SetNotFoundJsonMember(pluginConfigJson["global"],
//             //                           "UsingOldContentTag",
//             //                           config->mAdvancedConfig.mUsingOldContentTag);
//             // } else {
//             //     Json::Value pluginGlobalConfigJson;
//             //     SetNotFoundJsonMember(pluginGlobalConfigJson,
//             //                           "EnableTimestampNanosecond",
//             //                           config->mAdvancedConfig.mEnableTimestampNanosecond);
//             //     SetNotFoundJsonMember(pluginGlobalConfigJson,
//             //                           "UsingOldContentTag",
//             //                           config->mAdvancedConfig.mUsingOldContentTag);
//             //     pluginConfigJson["global"] = pluginGlobalConfigJson;
//             // }
//             // config->mPluginConfig = pluginConfigJson.toStyledString();

//             if (logType == DELIMITER_LOG) {
//                 config->mTimeFormat = GetStringValue(value, "timeformat", "");
//                 string separatorStr = GetStringValue(value, "delimiter_separator");
//                 if (separatorStr == "\\t")
//                     separatorStr = '\t';
//                 if (separatorStr.size() == 1) {
//                     config->mSeparator = separatorStr;
//                     string quoteStr = GetStringValue(value, "delimiter_quote", "\"");
//                     if (quoteStr.size() == 1)
//                         config->mQuote = quoteStr[0];
//                     else
//                         throw ExceptionBase("quote for Delimiter Log only support single char(like \")");
//                 } else if (separatorStr.size() > 1)
//                     config->mSeparator = separatorStr;
//                 else
//                     throw ExceptionBase("separator for Delimiter Log should not be empty");

//                 const Json::Value& columnKeys = value["column_keys"];
//                 for (uint32_t cIdx = 0; cIdx < columnKeys.size(); ++cIdx)
//                     config->mColumnKeys.push_back(columnKeys[cIdx].asString());

//                 config->mTimeKey = GetStringValue(value, "time_key", "");
//                 config->mAutoExtend = GetBoolValue(value, "auto_extend", true);
//                 config->mAcceptNoEnoughKeys = GetBoolValue(value, "accept_no_enough_keys", false);
//             } else if (logType == JSON_LOG) {
//                 config->mTimeFormat = GetStringValue(value, "timeformat", "");
//                 config->mTimeKey = GetStringValue(value, "time_key", "");
//             } else if (logType == APSARA_LOG) {
//                 // APSARA_LOG is a fix format, setting timeformat and timekey to pass adjust time zone validation
//                 config->mTimeFormat = "%Y-%m-%d %H%:M:%S";
//                 config->mTimeKey = "time";
//             }

//             // if (value.isMember("merge_type") && value["merge_type"].isString()) {
//             //     string mergeType = value["merge_type"].asString();

//             //     if (mergeType == "logstore") {
//             //         config->mMergeType = MERGE_BY_LOGSTORE;
//             //         LOG_INFO(sLogger, ("set config merge type to MERGE_BY_LOGSTORE", config->mConfigName));
//             //     } else {
//             //         config->mMergeType = MERGE_BY_TOPIC;
//             //     }
//             // }

//             if (value.isMember("tz_adjust") && value["tz_adjust"].isBool() && value.isMember("log_tz")
//                 && value["log_tz"].isString()) {
//                 string logTZ = value["log_tz"].asString();
//                 int logTZSecond = 0;
//                 bool adjustFlag = value["tz_adjust"].asBool();
//                 if (adjustFlag) {
//                     if (config->mTimeFormat.empty()) {
//                         LOG_WARNING(sLogger,
//                                     ("choose to adjust log time zone, but time format is not specified",
//                                      "use system time as log time instead")("project", config->mProjectName)(
//                                         "logstore", config->mCategory)("config", config->mConfigName));
//                         config->mTimeZoneAdjust = false;
//                     } else if ((logType == DELIMITER_LOG || logType == JSON_LOG) && config->mTimeKey.empty()) {
//                         LOG_WARNING(sLogger,
//                                     ("choose to adjust log time zone, but time key is not specified",
//                                      "use system time as log time instead")("project", config->mProjectName)(
//                                         "logstore", config->mCategory)("config", config->mConfigName));
//                         config->mTimeZoneAdjust = false;
//                     } else if (!ParseTimeZoneOffsetSecond(logTZ, logTZSecond)) {
//                         LOG_WARNING(sLogger,
//                                     ("invalid log time zone specified, will parse log time without time zone adjusted",
//                                      logTZ)("project", config->mProjectName)("logstore", config->mCategory)(
//                                         "config", config->mConfigName));
//                         config->mTimeZoneAdjust = false;
//                     } else {
//                         config->mTimeZoneAdjust = adjustFlag;
//                         config->mLogTimeZoneOffsetSecond = logTZSecond;
//                         LOG_INFO(sLogger,
//                                  ("set log time zone", logTZ)("project", config->mProjectName)(
//                                      "logstore", config->mCategory)("config", config->mConfigName));
//                     }
//                 } else {
//                     config->mTimeZoneAdjust = adjustFlag;
//                     config->mLogTimeZoneOffsetSecond = logTZSecond;
//                 }
//             }

//             // // create time
//             // int32_t createTime = 0;
//             // if (value.isMember("create_time") && value["create_time"].isInt()) {
//             //     createTime = value["create_time"].asInt();
//             // }
//             // config->mCreateTime = createTime;

//             // if (value.isMember("max_send_rate") && value["max_send_rate"].isInt() &&
//             // value.isMember("send_rate_expire")
//             //     && value["send_rate_expire"].isInt()) {
//             //     int32_t maxSendBytesPerSecond = value["max_send_rate"].asInt();
//             //     int32_t expireTime = value["send_rate_expire"].asInt();
//             //     config->mMaxSendBytesPerSecond = maxSendBytesPerSecond;
//             //     config->mSendRateExpireTime = expireTime;
//             //     LOG_INFO(
//             //         sLogger,
//             //         ("set logstore flow control, project", config->mProjectName)("logstore", config->mCategory)(
//             //             "max send byteps", config->mMaxSendBytesPerSecond)("expire", expireTime -
//             //             (int32_t)time(NULL)));
//             //     Sender::Instance()->SetLogstoreFlowControl(config->mLogstoreKey, maxSendBytesPerSecond, expireTime);
//             // }

//             if (value.isMember("sensitive_keys") && value["sensitive_keys"].isArray()) {
//                 GetSensitiveKeys(value["sensitive_keys"], config);
//             }

//             // config->mGroupTopic = GetStringValue(value, USER_CONFIG_GROUPTOPIC, "");
//             // 废弃
//             // config->mLocalStorage = true; // Must be true, parameter local_storage is desperated.
//             // config->mVersion = version;

//             // // default compress type is lz4. none if disable by gflag
//             // config->mCompressType
//             //     = BOOL_FLAG(sls_client_send_compress) ? GetStringValue(value, "compressType", "") : "none";
//             config->mDiscardNoneUtf8 = GetBoolValue(value, "discard_none_utf8", false);

//             // config->mAliuid = GetStringValue(value, "aliuid", "");
//             // if (AppConfig::GetInstance()->IsDataServerPrivateCloud())
//             //     config->mRegion = STRING_FLAG(default_region_name);
//             // else {
//             //     config->mRegion = region;
//             //     std::string defaultEndpoint = TrimString(GetStringValue(value, "defaultEndpoint", ""), ' ', ' ');
//             //     if (defaultEndpoint.size() > 0)
//             //         Sender::Instance()->AddEndpointEntry(config->mRegion,
//             //                                              CheckAddress(defaultEndpoint, defaultEndpoint));
//             // 废弃
//             // if (value.isMember("endpoint_list")) {
//             //     const Json::Value& epArray = value["endpoint_list"];
//             //     if (epArray.isArray()) {
//             //         for (uint32_t epIdx = 0; epIdx < epArray.size(); ++epIdx) {
//             //             if (!(epArray[epIdx].isString()))
//             //                 continue;
//             //             string ep = TrimString(epArray[epIdx].asString(), ' ', ' ');
//             //             if (ep.size() > 0)
//             //                 Sender::Instance()->AddEndpointEntry(config->mRegion, CheckAddress(ep, ep));
//             //         }
//             //     }
//             // }
//             // }
//             // InsertRegionAliuidMap(config->mRegion, config->mAliuid);

//             // config->mShardHashKey.clear();
//             // if (value.isMember("shard_hash_key")) {
//             //     const Json::Value& shardHashKey = value["shard_hash_key"];
//             //     if (shardHashKey.isArray()) {
//             //         for (uint32_t sIdx = 0; sIdx < shardHashKey.size(); ++sIdx)
//             //             config->mShardHashKey.push_back(shardHashKey[sIdx].asString());
//             //     }
//             // }
//             // 废弃
//             // config->mLocalFlag = localFlag;

//             // 废弃
//             // config->mIntegrityConfig.reset(new IntegrityConfig(config->mAliuid,
//             //                                                    dataIntegritySwitch,
//             //                                                    dataIntegrityProjectName,
//             //                                                    dataIntegrityLogstore,
//             //                                                    logTimeReg,
//             //                                                    config->mTimeFormat,
//             //                                                    timePos));
//             // // if integrity switch is off, erase corresponding item in integrity map
//             // if (!dataIntegritySwitch) {
//             //     LogIntegrity::GetInstance()->EraseItemInMap(config->mRegion, config->mProjectName,
//             //     config->mCategory);
//             // }
//             // config->mLineCountConfig.reset(
//             //     new LineCountConfig(config->mAliuid, lineCountSwitch, lineCountProjectName, lineCountLogstore));
//             // // if line count switch is off, erase corresponding item in line count map
//             // if (!lineCountSwitch) {
//             //     LogLineCount::GetInstance()->EraseItemInMap(config->mRegion, config->mProjectName,
//             //     config->mCategory);
//             // }
//             // // set fuse mode
//             // config->mIsFuseMode = isFuseMode;
//             // // if fuse mode is true, then we should mark offset
//             // config->mMarkOffsetFlag = isFuseMode || markOffsetFlag;
//             // mHaveFuseConfigFlag = mHaveFuseConfigFlag || isFuseMode;
//             // config->mCollectBackwardTillBootTime = collectBackwardTillBootTime;
//             // // time format should not be blank here
//             // if ((collectBackwardTillBootTime || dataIntegritySwitch) && config->mTimeFormat.empty()) {
//             //     LOG_ERROR(sLogger,
//             //               ("time format should not be blank if collect backward or open fata integrity function",
//             //               ""));
//             // }

//             // auto configIter = mNameConfigMap.find(logName);
//             // if (mNameConfigMap.end() != configIter) {
//             //     LOG_ERROR(sLogger,
//             //               ("duplicated config name, last will be deleted",
//             //                logName)("last", configIter->second->mBasePath)("new", config->mBasePath));
//             //     delete configIter->second;
//             //     configIter->second = config;
//             // } else {
//             //     mNameConfigMap[logName] = config;
//             // }
//             // InsertProject(config->mProjectName);
//             // InsertRegion(config->mRegion);
//             // UpdatePluginStats(rawValue);
//         }
//     } catch (const ExceptionBase& e) {
//         LOG_ERROR(sLogger, ("failed to parse config logname", logName)("Reason", e.GetExceptionMessage()));
//         errorMessage = logName + string(" is invalid, reason:") + e.GetExceptionMessage();
//     } catch (const exception& e) {
//         LOG_ERROR(sLogger, ("failed to parse config logname", logName)("Reason", e.what()));
//         errorMessage = logName + string(" is invalid, reason:") + e.what();
//     } catch (...) {
//         LOG_ERROR(sLogger, ("failed to parse config logname", logName)("Reason", "Unknown"));
//         errorMessage = logName + " is invalid, reason:unknow";
//     }
//     if (!errorMessage.empty()) {
//         if (config)
//             delete config;
//         if (!projectName.empty())
//             LogtailAlarm::GetInstance()->SendAlarm(CATEGORY_CONFIG_ALARM, errorMessage, projectName, category);
//         else
//             LogtailAlarm::GetInstance()->SendAlarm(USER_CONFIG_ALARM, errorMessage);
//     }
// }

// dir which is not timeout will be registerd recursively
// if checkTimeout, will not register the dir which is timeout
// if not checkTimeout, will register the dir which is timeout and add it to the timeout list
bool ConfigManager::RegisterHandlersRecursively(const std::string& path,
                                                    const FileDiscoveryConfig& config,
                                                    bool checkTimeout) {
    if (AppConfig::GetInstance()->IsHostPathMatchBlacklist(path)) {
        LOG_INFO(sLogger, ("ignore path matching host path blacklist", path));
        return false;
    }
    bool result = false;
    if (checkTimeout && config.first->IsTimeout(path))
        return result;

    if (!config.first->IsDirectoryInBlacklist(path))
        result = EventDispatcher::GetInstance()->RegisterEventHandler(path.c_str(), config, mSharedHandler);

    if (!result)
        return result;

    fsutil::Dir dir(path);
    if (!dir.Open()) {
        auto err = GetErrno();
        LogtailAlarm::GetInstance()->SendAlarm(LOGDIR_PERMINSSION_ALARM,
                                               string("Failed to open dir : ") + path + ";\terrno : " + ToString(err),
                                               config.second->GetProjectName(),
                                               config.second->GetLogstoreName(),
                                               config.second->GetRegion());

        LOG_ERROR(sLogger, ("Open dir fail", path.c_str())("errno", ErrnoToString(err)));
        return false;
    }
    fsutil::Entry ent;
    while (ent = dir.ReadNext()) {
        string item = PathJoin(path, ent.Name());
        if (ent.IsDir()) {
            if (false == RegisterHandlersRecursively(item, config, checkTimeout)) {
                result = false;
            }
        }
    }
    return result;
}

ConfigManager::ConfigManager() {
    // mEnvFlag = false;
    // mStartTime = time(NULL);
    // mRemoveConfigFlag = false;
    // mHaveMappingPathConfig = false;
    // mMappingPathsChanged = false;
    mSharedHandler = NULL;
    // mThreadIsRunning = true;
    // mUpdateStat = NORMAL;
    // mRegionType = REGION_CORP;

    // mLogtailSysConfUpdateTime = 0;
    // mUserDefinedId.clear();
    // mUserDefinedIdSet.clear();
    // mAliuidSet.clear();

    // SetDefaultPubAccessKeyId(STRING_FLAG(default_access_key_id));
    // SetDefaultPubAccessKey(STRING_FLAG(default_access_key));
    // SetDefaultPubAliuid("");
    // SetUserAK(STRING_FLAG(logtail_profile_aliuid),
    //           STRING_FLAG(logtail_profile_access_key_id),
    //           STRING_FLAG(logtail_profile_access_key));
    srand(time(NULL));
    // CorrectionLogtailSysConfDir(); // first create dir then rewrite system-uuid file in GetSystemUUID
    // use a thread to get uuid, work around for CalculateDmiUUID hang
    // mUUID = CalculateDmiUUID();
    // mInstanceId = CalculateRandomUUID() + "_" + LogFileProfiler::mIpAddr + "_" + ToString(time(NULL));
    // ReloadMappingConfig();
}

ConfigManager::~ConfigManager() {
    // unordered_map<string, Config*>::iterator itr = mNameConfigMap.begin();
    // for (; itr != mNameConfigMap.end(); ++itr) {
    //     delete itr->second;
    // }

    unordered_map<std::string, EventHandler*>::iterator itr1 = mDirEventHandlerMap.begin();
    for (; itr1 != mDirEventHandlerMap.end(); ++itr1) {
        if (itr1->second != mSharedHandler)
            delete itr1->second;
    }
    mDirEventHandlerMap.clear();
    delete mSharedHandler;
    // mThreadIsRunning = false;
    // try {
    //     if (mUUIDthreadPtr.get() != NULL)
    //         mUUIDthreadPtr->GetValue(100);
    // } catch (...) {
    // }
}

void ConfigManager::RemoveHandler(const string& dir, bool delete_flag) {
    unordered_map<string, EventHandler*>::iterator itr = mDirEventHandlerMap.find(dir);
    if (itr != mDirEventHandlerMap.end()) {
        if (itr->second != mSharedHandler && delete_flag) {
            delete itr->second;
        }
        mDirEventHandlerMap.erase(itr);
    }
}

// this functions should only be called when register base dir
bool ConfigManager::RegisterHandlers() {
    if (mSharedHandler == NULL) {
        mSharedHandler = new NormalEventHandler();
    }
    vector<FileDiscoveryConfig> sortedConfigs;
    vector<FileDiscoveryConfig> wildcardConfigs;
    auto nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    for (auto itr = nameConfigMap.begin(); itr != nameConfigMap.end(); ++itr) {
        if (itr->second.first->GetWildcardPaths().empty())
            sortedConfigs.push_back(itr->second);
        else
            wildcardConfigs.push_back(itr->second);
    }
    sort(sortedConfigs.begin(), sortedConfigs.end(), FileDiscoveryOptions::CompareByPathLength);
    bool result = true;
    for (auto itr = sortedConfigs.begin(); itr != sortedConfigs.end(); ++itr) {
        const FileDiscoveryOptions* config = itr->first;
        if (!config->IsContainerDiscoveryEnabled()) {
            result &= RegisterHandlers(config->GetBasePath(), *itr);
        } else {
            for (size_t i = 0; i < config->GetContainerInfo()->size(); ++i) {
                result &= RegisterHandlers((*config->GetContainerInfo())[i].mContainerPath, *itr);
            }
        }
    }
    for (auto itr = wildcardConfigs.begin(); itr != wildcardConfigs.end(); ++itr) {
        const FileDiscoveryOptions* config = itr->first;
        if (!config->IsContainerDiscoveryEnabled()) {
            RegisterWildcardPath(*itr, config->GetWildcardPaths()[0], 0);
        } else {
            for (size_t i = 0; i < config->GetContainerInfo()->size(); ++i) {
                RegisterWildcardPath(*itr, (*config->GetContainerInfo())[i].mContainerPath, 0);
            }
        }
    }
    return result;
}

void ConfigManager::RegisterWildcardPath(const FileDiscoveryConfig& config,
                                             const std::string& path,
                                             int32_t depth) {
    if (AppConfig::GetInstance()->IsHostPathMatchBlacklist(path)) {
        LOG_INFO(sLogger, ("ignore path matching host path blacklist", path));
        return;
    }
    bool finish;
    if ((depth + 1) == ((int)config.first->GetWildcardPaths().size() - 1))
        finish = true;
    else if ((depth + 1) < ((int)config.first->GetWildcardPaths().size() - 1))
        finish = false;
    else
        return;

    // const path
    if (!config.first->GetConstWildcardPaths()[depth].empty()) {
        // stat directly
        string item = PathJoin(path, config.first->GetConstWildcardPaths()[depth]);
        fsutil::PathStat baseDirStat;
        if (!fsutil::PathStat::stat(item, baseDirStat)) {
            LOG_DEBUG(sLogger,
                      ("get wildcard dir info error: ",
                       item)(config.second->GetProjectName(),
                             config.second->GetLogstoreName())("error", ErrnoToString(GetErrno())));
            return;
        }
        if (!baseDirStat.IsDir())
            return;
        if (finish) {
            DirRegisterStatus registerStatus = EventDispatcher::GetInstance()->IsDirRegistered(item);
            if (registerStatus == GET_REGISTER_STATUS_ERROR) {
                return;
            }
            if (EventDispatcher::GetInstance()->RegisterEventHandler(item.c_str(), config, mSharedHandler)) {
                RegisterDescendants(
                    item, config, config.first->mMaxDirSearchDepth < 0 ? 100 : config.first->mMaxDirSearchDepth);
            }
        } else {
            RegisterWildcardPath(config, item, depth + 1);
        }
        return;
    }

    fsutil::Dir dir(path);
    if (!dir.Open()) {
        auto err = GetErrno();
        LogtailAlarm::GetInstance()->SendAlarm(LOGDIR_PERMINSSION_ALARM,
                                               string("Failed to open dir : ") + path + ";\terrno : " + ToString(err),
                                               config.second->GetProjectName(),
                                               config.second->GetLogstoreName(),
                                               config.second->GetRegion());
        LOG_WARNING(sLogger, ("Open dir fail", path.c_str())("errno", err));
        return;
    }
    fsutil::Entry ent;
    int32_t dirCount = 0;
    while (ent = dir.ReadNext()) {
        if (dirCount >= INT32_FLAG(wildcard_max_sub_dir_count)) {
            LOG_WARNING(sLogger,
                        ("too many sub directoried for path", path)("dirCount", dirCount)("basePath",
                                                                                          config.first->GetBasePath()));
            break;
        }
        if (!ent.IsDir())
            continue;

        ++dirCount;
        string item = PathJoin(path, ent.Name());

        // we should check match and then check finsh
        size_t dirIndex = config.first->GetWildcardPaths()[depth].size() + 1;
#if defined(_MSC_VER)
        // Backward compatibility: the inner condition never happen.
        if (!BOOL_FLAG(enable_root_path_collection)) {
            if (dirIndex == (size_t)2) {
                dirIndex = 1;
            }
        }
        // For wildcard Windows path C:\*, mWildcardPaths[0] will be C:\,
        //   so dirIndex should be adjusted by minus 1.
        else if (0 == depth) {
            const auto& firstWildcardPath = config.first->GetWildcardPaths()[0];
            const auto pathSize = firstWildcardPath.size();
            if (pathSize >= 2 && firstWildcardPath[pathSize - 1] == PATH_SEPARATOR[0]
                && firstWildcardPath[pathSize - 2] == ':') {
                --dirIndex;
            }
        }
#else
        // if mWildcardPaths[depth] is '/', we should set dirIndex = 1
        if (dirIndex == (size_t)2) {
            dirIndex = 1;
        }
#endif

        if (fnmatch(&(config.first->GetWildcardPaths()[depth + 1].at(dirIndex)), ent.Name().c_str(), FNM_PATHNAME)
            == 0) {
            if (finish) {
                DirRegisterStatus registerStatus = EventDispatcher::GetInstance()->IsDirRegistered(item);
                if (registerStatus == GET_REGISTER_STATUS_ERROR) {
                    return;
                }
                if (EventDispatcher::GetInstance()->RegisterEventHandler(item.c_str(), config, mSharedHandler)) {
                    RegisterDescendants(
                        item, config, config.first->mMaxDirSearchDepth < 0 ? 100 : config.first->mMaxDirSearchDepth);
                }
            } else {
                RegisterWildcardPath(config, item, depth + 1);
            }
        }
    }
}

// this functions should only be called when register base dir
bool ConfigManager::RegisterHandlers(const string& basePath, const FileDiscoveryConfig& config) {
    bool result = true;
    // static set<string> notExistDirs;
    // static int32_t lastErrorLogTime = time(NULL);
    // if (notExistDirs.size() > 0 && (time(NULL) - lastErrorLogTime > 3600)) {
    //     string dirs;
    //     for (set<string>::iterator iter = notExistDirs.begin(); iter != notExistDirs.end(); ++iter) {
    //         dirs.append(*iter);
    //         dirs.append(" ");
    //     }
    //     lastErrorLogTime = time(NULL);
    //     notExistDirs.clear();
    //     LOG_WARNING(sLogger, ("logPath in config not exist", dirs));
    // }
    if (!CheckExistance(basePath)) {
        // if (!(config->mLogType == APSARA_LOG && basePath.find("/tmp") == 0)
        //     && basePath.find(LOG_LOCAL_DEFINED_PATH_PREFIX) != 0)
        //     notExistDirs.insert(basePath);
        if (EventDispatcher::GetInstance()->IsRegistered(basePath.c_str())) {
            EventDispatcher::GetInstance()->UnregisterAllDir(basePath);
            LOG_DEBUG(sLogger, ("logPath in config not exist, unregister existed monitor", basePath));
        }
        return result;
    }
    DirRegisterStatus registerStatus = EventDispatcher::GetInstance()->IsDirRegistered(basePath);
    if (registerStatus == GET_REGISTER_STATUS_ERROR)
        return result;
    // dir in config is valid by default, do not call pathValidator
    result = EventDispatcher::GetInstance()->RegisterEventHandler(basePath.c_str(), config, mSharedHandler);
    // if we come into a failure, do not try to register others, there must be something wrong!
    if (!result)
        return result;

    if (config.first->mPreservedDirDepth < 0)
        result = RegisterDescendants(
            basePath, config, config.first->mMaxDirSearchDepth < 0 ? 100 : config.first->mMaxDirSearchDepth);
    else {
        // preserve_depth register
        int depth = config.first->mPreservedDirDepth;
        result = RegisterHandlersWithinDepth(basePath, config, depth);
    }
    return result;
}

bool ConfigManager::RegisterDirectory(const std::string& source, const std::string& object) {
    // TODO��A potential bug: FindBestMatch will test @object with filePattern, which has very
    // low possibility to match a sub directory name, so here will return false in most cases.
    // e.g.: source: /path/to/monitor, file pattern: *.log, object: subdir.
    // Match(subdir, *.log) = false.
    FileDiscoveryConfig config = FindBestMatch(source, object);
    if (config.first && !config.first->IsDirectoryInBlacklist(source))
        return EventDispatcher::GetInstance()->RegisterEventHandler(source.c_str(), config, mSharedHandler);
    return false;
}

bool ConfigManager::RegisterHandlersWithinDepth(const std::string& path,
                                                    const FileDiscoveryConfig& config,
                                                    int depth) {
    if (AppConfig::GetInstance()->IsHostPathMatchBlacklist(path)) {
        LOG_INFO(sLogger, ("ignore path matching host path blacklist", path));
        return false;
    }
    if (depth <= 0) {
        DirCheckPointPtr dirCheckPoint;
        if (CheckPointManager::Instance()->GetDirCheckPoint(path, dirCheckPoint) == false)
            return true;
        // path had dircheckpoint means it was watched before, so it is valid
        const set<string>& subdir = dirCheckPoint.get()->mSubDir;
        for (set<string>::iterator it = subdir.begin(); it != subdir.end(); it++) {
            if (EventDispatcher::GetInstance()->RegisterEventHandler((*it).c_str(), config, mSharedHandler))
                RegisterHandlersWithinDepth(*it, config, depth - 1);
        }
        return true;
    }
    bool result = true;

    fsutil::Dir dir(path);
    if (!dir.Open()) {
        int err = GetErrno();
        LogtailAlarm::GetInstance()->SendAlarm(LOGDIR_PERMINSSION_ALARM,
                                               string("Failed to open dir : ") + path + ";\terrno : " + ToString(err),
                                               config.second->GetProjectName(),
                                               config.second->GetLogstoreName(),
                                               config.second->GetRegion());

        LOG_ERROR(sLogger, ("Open dir error: ", path.c_str())("errno", err));
        return false;
    }
    fsutil::Entry ent;
    while (ent = dir.ReadNext()) {
        string item = PathJoin(path, ent.Name());
        if (ent.IsDir() && !config.first->IsDirectoryInBlacklist(item)) {
            if (!(EventDispatcher::GetInstance()->RegisterEventHandler(item.c_str(), config, mSharedHandler))) {
                // break;// fail early, do not try to register others
                result = false;
            } else // sub dir will not be registered if parent dir fails
                RegisterHandlersWithinDepth(item, config, depth - 1);
        }
    }

    return result;
}

// path not terminated by '/', path already registered
bool ConfigManager::RegisterDescendants(const string& path, const FileDiscoveryConfig& config, int withinDepth) {
    if (AppConfig::GetInstance()->IsHostPathMatchBlacklist(path)) {
        LOG_INFO(sLogger, ("ignore path matching host path blacklist", path));
        return false;
    }
    if (withinDepth <= 0) {
        return true;
    }

    fsutil::Dir dir(path);
    if (!dir.Open()) {
        auto err = GetErrno();
        LogtailAlarm::GetInstance()->SendAlarm(LOGDIR_PERMINSSION_ALARM,
                                               string("Failed to open dir : ") + path + ";\terrno : " + ToString(err),
                                               config.second->GetProjectName(),
                                               config.second->GetLogstoreName(),
                                               config.second->GetRegion());
        LOG_ERROR(sLogger, ("Open dir error: ", path.c_str())("errno", err));
        return false;
    }
    fsutil::Entry ent;
    bool result = true;
    while (ent = dir.ReadNext()) {
        string item = PathJoin(path, ent.Name());
        if (ent.IsDir() && !config.first->IsDirectoryInBlacklist(item)) {
            result = EventDispatcher::GetInstance()->RegisterEventHandler(item.c_str(), config, mSharedHandler);
            if (result)
                RegisterDescendants(item, config, withinDepth - 1);
        }
    }
    return result;
}

void ConfigManager::ClearConfigMatchCache() {
    static const int32_t FORCE_CLEAR_INTERVAL = 6 * 3600;
    static int32_t s_lastClearTime = (int32_t)time(NULL) - rand() % 600;
    static int32_t s_lastClearAllTime = (int32_t)time(NULL) - rand() % 600;
    int32_t curTime = (int32_t)time(NULL);

    ScopedSpinLock cachedLock(mCacheFileConfigMapLock);
    if (curTime - s_lastClearTime > FORCE_CLEAR_INTERVAL
        || mCacheFileConfigMap.size() > (size_t)INT32_FLAG(config_match_max_cache_size)) {
        s_lastClearTime = curTime;
        mCacheFileConfigMap.clear();
    }

    ScopedSpinLock allCachedLock(mCacheFileAllConfigMapLock);
    if (curTime - s_lastClearAllTime > FORCE_CLEAR_INTERVAL
        || mCacheFileAllConfigMap.size() > (size_t)INT32_FLAG(config_match_max_cache_size)) {
        s_lastClearAllTime = curTime;
        mCacheFileAllConfigMap.clear();
    }
}

FileDiscoveryConfig ConfigManager::FindBestMatch(const string& path, const string& name) {
    string cachedFileKey(path);
    cachedFileKey.push_back('<');
    cachedFileKey.append(name);
    bool acceptMultiConfig = AppConfig::GetInstance()->IsAcceptMultiConfig();
    {
        ScopedSpinLock cachedLock(mCacheFileConfigMapLock);
        auto iter = mCacheFileConfigMap.find(cachedFileKey);
        if (iter != mCacheFileConfigMap.end()) {
            // if need report alarm, do not return, just continue to find all match and send alarm
            if (acceptMultiConfig || iter->second.second == 0
                || time(NULL) - iter->second.second < INT32_FLAG(multi_config_alarm_interval)) {
                return iter->second.first;
            }
        }
    }
    const auto& nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    auto itr = nameConfigMap.begin();
    FileDiscoveryConfig prevMatch(nullptr, nullptr);
    size_t prevLen = 0;
    size_t curLen = 0;
    uint32_t nameRepeat = 0;
    string logNameList;
    vector<FileDiscoveryConfig> multiConfigs;
    for (; itr != nameConfigMap.end(); ++itr) {
        const FileDiscoveryOptions* config = itr->second.first;
        // exclude __FUSE_CONFIG__
        if (itr->first == STRING_FLAG(fuse_customized_config_name)) {
            continue;
        }

        bool match = config->IsMatch(path, name);
        if (match) {
            // if force multi config, do not send alarm
            if (!name.empty() && !config->mAllowingIncludedByMultiConfigs) {
                nameRepeat++;
                logNameList.append("logstore:");
                logNameList.append(itr->second.second->GetLogstoreName());
                logNameList.append(",config:");
                logNameList.append(itr->second.second->GetConfigName());
                logNameList.append(" ");
                multiConfigs.push_back(itr->second);
            }

            // note: best config is the one which length is longest and create time is nearest
            curLen = config->GetBasePath().size();
            if (prevLen < curLen) {
                prevMatch = itr->second;
                prevLen = curLen;
            } else if (prevLen == curLen && prevMatch.first) {
                if (prevMatch.second->GetCreateTime() > itr->second.second->GetCreateTime()) {
                    prevMatch = itr->second;
                    prevLen = curLen;
                }
            }
        }
    }

    // file include in multi config, find config for path will not trigger this alarm
    // do not send alarm to config with mForceMultiConfig
    if (nameRepeat > 1 && !name.empty() && !acceptMultiConfig) {
        LOG_ERROR(sLogger,
                  ("file", path + '/' + name)("include in multi config",
                                              logNameList)("best", prevMatch.second->GetConfigName()));
        for (auto iter = multiConfigs.begin(); iter != multiConfigs.end(); ++iter) {
            LogtailAlarm::GetInstance()->SendAlarm(
                MULTI_CONFIG_MATCH_ALARM,
                path + '/' + name + " include in multi config, best and oldest match: "
                    + prevMatch.second->GetProjectName() + ',' + prevMatch.second->GetLogstoreName() + ','
                    + prevMatch.second->GetConfigName() + ", allmatch: " + logNameList,
                (*iter).second->GetProjectName(),
                (*iter).second->GetLogstoreName(),
                (*iter).second->GetRegion());
        }
    }
    {
        ScopedSpinLock cachedLock(mCacheFileConfigMapLock);
        // use operator [], force update time
        mCacheFileConfigMap[cachedFileKey]
            = std::make_pair(prevMatch, nameRepeat > 1 && !name.empty() ? (int32_t)time(NULL) : (int32_t)0);
    }
    return prevMatch;
}


int32_t ConfigManager::FindAllMatch(vector<FileDiscoveryConfig>& allConfig,
                                        const std::string& path,
                                        const std::string& name /*= ""*/) {
    static AppConfig* appConfig = AppConfig::GetInstance();
    string cachedFileKey(path);
    cachedFileKey.push_back('<');
    cachedFileKey.append(name);
    const int32_t maxMultiConfigSize = appConfig->GetMaxMultiConfigSize();
    {
        ScopedSpinLock cachedLock(mCacheFileAllConfigMapLock);
        auto iter = mCacheFileAllConfigMap.find(cachedFileKey);
        if (iter != mCacheFileAllConfigMap.end()) {
            if (iter->second.second == 0
                || time(NULL) - iter->second.second < INT32_FLAG(multi_config_alarm_interval)) {
                allConfig = iter->second.first;
                return (int32_t)allConfig.size();
            }
        }
    }
    bool alarmFlag = false;
    auto nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    auto itr = nameConfigMap.begin();
    for (; itr != nameConfigMap.end(); ++itr) {
        const FileDiscoveryOptions* config = itr->second.first;
        // exclude __FUSE_CONFIG__
        if (itr->first == STRING_FLAG(fuse_customized_config_name)) {
            continue;
        }

        bool match = config->IsMatch(path, name);
        if (match) {
            allConfig.push_back(itr->second);
        }
    }

    if (!name.empty() && allConfig.size() > static_cast<size_t>(maxMultiConfigSize)) {
        // only report log file alarm
        alarmFlag = true;
        sort(allConfig.begin(), allConfig.end(), FileDiscoveryOptions::CompareByDepthAndCreateTime);
        SendAllMatchAlarm(path, name, allConfig, maxMultiConfigSize);
        allConfig.resize(maxMultiConfigSize);
    }
    {
        ScopedSpinLock cachedLock(mCacheFileAllConfigMapLock);
        // use operator [], force update time
        mCacheFileAllConfigMap[cachedFileKey] = std::make_pair(allConfig, alarmFlag ? (int32_t)time(NULL) : (int32_t)0);
    }
    return (int32_t)allConfig.size();
}

int32_t ConfigManager::FindMatchWithForceFlag(std::vector<FileDiscoveryConfig>& allConfig,
                                                  const string& path,
                                                  const string& name) {
    const bool acceptMultiConfig = AppConfig::GetInstance()->IsAcceptMultiConfig();
    string cachedFileKey(path);
    cachedFileKey.push_back('<');
    cachedFileKey.append(name);
    {
        ScopedSpinLock cachedLock(mCacheFileAllConfigMapLock);
        auto iter = mCacheFileAllConfigMap.find(cachedFileKey);
        if (iter != mCacheFileAllConfigMap.end()) {
            if (iter->second.second == 0
                || time(NULL) - iter->second.second < INT32_FLAG(multi_config_alarm_interval)) {
                allConfig = iter->second.first;
                return (int32_t)allConfig.size();
            }
        }
    }
    auto nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    auto itr = nameConfigMap.begin();
    FileDiscoveryConfig prevMatch = make_pair(nullptr, nullptr);
    size_t prevLen = 0;
    size_t curLen = 0;
    uint32_t nameRepeat = 0;
    string logNameList;
    vector<FileDiscoveryConfig> multiConfigs;
    for (; itr != nameConfigMap.end(); ++itr) {
        FileDiscoveryConfig config = itr->second;
        // exclude __FUSE_CONFIG__
        if (itr->first == STRING_FLAG(fuse_customized_config_name)) {
            continue;
        }

        bool match = config.first->IsMatch(path, name);
        if (match) {
            // if force multi config, do not send alarm
            if (!name.empty() && !config.first->mAllowingIncludedByMultiConfigs) {
                nameRepeat++;
                logNameList.append("logstore:");
                logNameList.append(config.second->GetLogstoreName());
                logNameList.append(",config:");
                logNameList.append(config.second->GetConfigName());
                logNameList.append(" ");
                multiConfigs.push_back(config);
            }
            if (!config.first->mAllowingIncludedByMultiConfigs) {
                // if not ForceMultiConfig, find best match in normal cofigs
                // note: best config is the one which length is longest and create time is nearest
                curLen = config.first->GetBasePath().size();
                if (prevLen < curLen) {
                    prevMatch = config;
                    prevLen = curLen;
                } else if (prevLen == curLen && prevMatch.first) {
                    if (prevMatch.second->GetCreateTime() > config.second->GetCreateTime()) {
                        prevMatch = config;
                        prevLen = curLen;
                    }
                }
            } else {
                // save ForceMultiConfig
                allConfig.push_back(config);
            }
        }
    }

    bool alarmFlag = false;
    // file include in multi config, find config for path will not trigger this alarm
    // do not send alarm to config with mForceMultiConfig
    if (nameRepeat > 1 && !acceptMultiConfig && prevMatch.first) {
        alarmFlag = true;
        LOG_ERROR(sLogger,
                  ("file", path + '/' + name)("include in multi config",
                                              logNameList)("best", prevMatch.second->GetConfigName()));
        for (auto iter = multiConfigs.begin(); iter != multiConfigs.end(); ++iter) {
            LogtailAlarm::GetInstance()->SendAlarm(
                MULTI_CONFIG_MATCH_ALARM,
                path + '/' + name + " include in multi config, best and oldest match: "
                    + prevMatch.second->GetProjectName() + ',' + prevMatch.second->GetLogstoreName() + ','
                    + prevMatch.second->GetConfigName() + ", allmatch: " + logNameList,
                (*iter).second->GetProjectName(),
                (*iter).second->GetLogstoreName(),
                (*iter).second->GetRegion());
        }
    }
    if (prevMatch.first) {
        allConfig.push_back(prevMatch);
    }
    {
        ScopedSpinLock cachedLock(mCacheFileAllConfigMapLock);
        // use operator [], force update time
        mCacheFileAllConfigMap[cachedFileKey] = std::make_pair(allConfig, alarmFlag ? (int32_t)time(NULL) : (int32_t)0);
    }
    return (int32_t)allConfig.size();
}

void ConfigManager::SendAllMatchAlarm(const string& path,
                                          const string& name,
                                          vector<FileDiscoveryConfig>& allConfig,
                                          int32_t maxMultiConfigSize) {
    string allConfigNames;
    for (auto iter = allConfig.begin(); iter != allConfig.end(); ++iter) {
        allConfigNames.append("[")
            .append((*iter).second->GetProjectName())
            .append(" : ")
            .append((*iter).second->GetLogstoreName())
            .append(" : ")
            .append((*iter).second->GetConfigName())
            .append("]");
    }
    LOG_ERROR(sLogger,
              ("file", path + '/' + name)("include in too many configs", allConfig.size())(
                  "max multi config size", maxMultiConfigSize)("allconfigs", allConfigNames));
    for (auto iter = allConfig.begin(); iter != allConfig.end(); ++iter)
        LogtailAlarm::GetInstance()->SendAlarm(
            TOO_MANY_CONFIG_ALARM,
            path + '/' + name + " include in too many configs:" + ToString(allConfig.size())
                + ", max multi config size : " + ToString(maxMultiConfigSize) + ", allmatch: " + allConfigNames,
            (*iter).second->GetProjectName(),
            (*iter).second->GetLogstoreName(),
            (*iter).second->GetRegion());
}

void ConfigManager::AddHandlerToDelete(EventHandler* handler) {
    mHandlersToDelete.push_back(handler);
}

void ConfigManager::DeleteHandlers() {
    for (size_t i = 0; i < mHandlersToDelete.size(); ++i) {
        if (mHandlersToDelete[i] != mSharedHandler && mHandlersToDelete[i] != NULL)
            delete mHandlersToDelete[i];
    }
    mHandlersToDelete.clear();
}

// find config domain socket data, find postfix like "_category"
const FlusherSLS* ConfigManager::FindDSConfigByCategory(const std::string& dsCtegory) {
    auto nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    for (auto iter = nameConfigMap.begin(); iter != nameConfigMap.end(); ++iter) {
        const Flusher* plugin = iter->second.second->GetPipeline().GetFlushers()[0]->GetPlugin();
        if (plugin->Name() == "flusher_sls") {
            const FlusherSLS* flusherSLS = static_cast<const FlusherSLS*>(plugin);
            if (dsCtegory == flusherSLS->mLogstore) {
                return flusherSLS;
            }
        }
    }
    return nullptr;
}

// GetRelatedConfigs calculates related configs of @path.
// Two kind of relations:
// 1. No wildcard path: the base path of Config is the prefix of @path and within depth.
// 2. Wildcard path: @path matches and within depth.
void ConfigManager::GetRelatedConfigs(const std::string& path, std::vector<FileDiscoveryConfig>& configs) {
    const auto& nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    for (auto iter = nameConfigMap.begin(); iter != nameConfigMap.end(); ++iter) {
        if (iter->second.first->IsMatch(path, "")) {
            configs.push_back(iter->second);
        }
    }
}

bool ConfigManager::UpdateContainerPath(DockerContainerPathCmd* cmd) {
    mDockerContainerPathCmdLock.lock();
    mDockerContainerPathCmdVec.push_back(cmd);
    mDockerContainerPathCmdLock.unlock();
    return true;
}

bool ConfigManager::DoUpdateContainerPaths() {
    mDockerContainerPathCmdLock.lock();
    std::vector<DockerContainerPathCmd*> tmpPathCmdVec = mDockerContainerPathCmdVec;
    mDockerContainerPathCmdVec.clear();
    mDockerContainerPathCmdLock.unlock();
    LOG_INFO(sLogger, ("update container path", tmpPathCmdVec.size()));
    for (size_t i = 0; i < tmpPathCmdVec.size(); ++i) {
        FileDiscoveryConfig config = FileServer::GetInstance()->GetFileDiscoveryConfig(tmpPathCmdVec[i]->mConfigName);
        if (!config.first) {
            LOG_ERROR(sLogger,
                      ("invalid container path update cmd", tmpPathCmdVec[i]->mConfigName)("params",
                                                                                           tmpPathCmdVec[i]->mParams));
            continue;
        }
        if (tmpPathCmdVec[i]->mDeleteFlag) {
            if (config.first->DeleteDockerContainerPath(tmpPathCmdVec[i]->mParams)) {
                LOG_DEBUG(sLogger,
                          ("container path delete cmd success",
                           tmpPathCmdVec[i]->mConfigName)("params", tmpPathCmdVec[i]->mParams));
            } else {
                LOG_ERROR(sLogger,
                          ("container path delete cmd fail", tmpPathCmdVec[i]->mConfigName)("params",
                                                                                            tmpPathCmdVec[i]->mParams));
            }
        } else {
            if (config.first->UpdateDockerContainerPath(tmpPathCmdVec[i]->mParams, tmpPathCmdVec[i]->mUpdateAllFlag)) {
                LOG_DEBUG(sLogger,
                          ("container path update cmd success", tmpPathCmdVec[i]->mConfigName)(
                              "params", tmpPathCmdVec[i]->mParams)("all", tmpPathCmdVec[i]->mUpdateAllFlag));
            } else {
                LOG_ERROR(sLogger,
                          ("container path update cmd fail", tmpPathCmdVec[i]->mConfigName)(
                              "params", tmpPathCmdVec[i]->mParams)("all", tmpPathCmdVec[i]->mUpdateAllFlag));
            }
        }
        delete tmpPathCmdVec[i];
    }
    return true;
}

bool ConfigManager::IsUpdateContainerPaths() {
    mDockerContainerPathCmdLock.lock();
    bool rst = false;
    for (size_t i = 0; i < mDockerContainerPathCmdVec.size(); ++i) {
        DockerContainerPathCmd* pCmd = mDockerContainerPathCmdVec[i];
        if (pCmd->mDeleteFlag) {
            rst = true;
            break;
        }
        FileDiscoveryConfig pConfig = FileServer::GetInstance()->GetFileDiscoveryConfig(pCmd->mConfigName);
        if (!pConfig.first) {
            continue;
        }
        if (!pConfig.first->IsSameDockerContainerPath(pCmd->mParams, pCmd->mUpdateAllFlag)) {
            rst = true;
            break;
        }
    }
    if (mDockerContainerPathCmdVec.size() > 0) {
        LOG_INFO(sLogger, ("check container path update flag", rst)("size", mDockerContainerPathCmdVec.size()));
    }
    if (rst == false) {
        for (size_t i = 0; i < mDockerContainerPathCmdVec.size(); ++i) {
            delete mDockerContainerPathCmdVec[i];
        }
        mDockerContainerPathCmdVec.clear();
    }
    mDockerContainerPathCmdLock.unlock();

    /********** qps limit : only update docker config INT32_FLAG(max_docker_config_update_times) times in 3 minutes
     * ********/
    static int32_t s_lastUpdateTime = 0;
    static int32_t s_lastUpdateCount = 0;
    if (!rst) {
        return rst;
    }
    int32_t nowTime = time(NULL);
    // not in 3 minutes window
    if (nowTime / 180 != s_lastUpdateTime / 180) {
        s_lastUpdateCount = 1;
        s_lastUpdateTime = nowTime;
        return rst;
    }
    // this window's update times < INT32_FLAG(max_docker_config_update_times)
    // min interval : 10 seconds
    // For case with frequent container update (eg. K8s short job), adjust this parameter.
    if (s_lastUpdateCount < INT32_FLAG(max_docker_config_update_times)
        && nowTime - s_lastUpdateTime >= INT32_FLAG(docker_config_update_interval)) {
        ++s_lastUpdateCount;
        s_lastUpdateTime = nowTime;
        return rst;
    }
    // return false if s_lastUpdateCount >= INT32_FLAG(max_docker_config_update_times) and last update time is in same
    // window
    return false;
    /************************************************************************************************************************/
}

bool ConfigManager::UpdateContainerStopped(DockerContainerPathCmd* cmd) {
    PTScopedLock lock(mDockerContainerStoppedCmdLock);
    mDockerContainerStoppedCmdVec.push_back(cmd);
    return true;
}

void ConfigManager::GetContainerStoppedEvents(std::vector<Event*>& eventVec) {
    std::vector<DockerContainerPathCmd*> cmdVec;
    {
        PTScopedLock lock(mDockerContainerStoppedCmdLock);
        cmdVec.swap(mDockerContainerStoppedCmdVec);
    }
    for (auto& cmd : cmdVec) {
        // find config and container's path, then emit stopped event
        FileDiscoveryConfig config = FileServer::GetInstance()->GetFileDiscoveryConfig(cmd->mConfigName);
        if (!config.first) {
            continue;
        }
        DockerContainerPath dockerContainerPath;
        if (!DockerContainerPath::ParseByJSONStr(cmd->mParams, dockerContainerPath)) {
            continue;
        }
        std::vector<DockerContainerPath>::iterator iter = config.first->GetContainerInfo()->begin();
        std::vector<DockerContainerPath>::iterator iend = config.first->GetContainerInfo()->end();
        for (; iter != iend; ++iter) {
            if (iter->mContainerID == dockerContainerPath.mContainerID) {
                break;
            }
        }
        if (iter == iend) {
            continue;
        }
        Event* pStoppedEvent = new Event(iter->mContainerPath, "", EVENT_ISDIR | EVENT_CONTAINER_STOPPED, -1, 0);
        LOG_DEBUG(
            sLogger,
            ("GetContainerStoppedEvent Type", pStoppedEvent->GetType())("Source", pStoppedEvent->GetSource())(
                "Object", pStoppedEvent->GetObject())("Config", pStoppedEvent->GetConfigName())(
                "IsDir", pStoppedEvent->IsDir())("IsCreate", pStoppedEvent->IsCreate())("IsModify",
                                                                                        pStoppedEvent->IsModify())(
                "IsDeleted", pStoppedEvent->IsDeleted())("IsMoveFrom", pStoppedEvent->IsMoveFrom())(
                "IsContainerStopped", pStoppedEvent->IsContainerStopped())("IsMoveTo", pStoppedEvent->IsMoveTo()));
        eventVec.push_back(pStoppedEvent);
    }
    for (auto cmd : cmdVec) {
        delete cmd;
    }
}

void ConfigManager::SaveDockerConfig() {
    string dockerPathConfigName = AppConfig::GetInstance()->GetDockerFilePathConfig();
    Json::Value dockerPathValueRoot;
    dockerPathValueRoot["version"] = Json::Value(STRING_FLAG(ilogtail_docker_path_version));
    Json::Value dockerPathValueDetail;
    mDockerContainerPathCmdLock.lock();
    const auto& nameConfigMap = FileServer::GetInstance()->GetAllFileDiscoveryConfigs();
    for (auto it = nameConfigMap.begin(); it != nameConfigMap.end(); ++it) {
        if (it->second.first->GetContainerInfo()) {
            std::vector<DockerContainerPath>& containerPathVec = *(it->second.first->GetContainerInfo());
            for (size_t i = 0; i < containerPathVec.size(); ++i) {
                Json::Value dockerPathValue;
                dockerPathValue["config_name"] = Json::Value(it->first);
                dockerPathValue["container_id"] = Json::Value(containerPathVec[i].mContainerID);
                dockerPathValue["params"] = Json::Value(containerPathVec[i].mJsonStr);
                dockerPathValueDetail.append(dockerPathValue);
            }
        }
    }
    mDockerContainerPathCmdLock.unlock();
    dockerPathValueRoot["detail"] = dockerPathValueDetail;
    string dockerInfo = dockerPathValueRoot.toStyledString();
    OverwriteFile(dockerPathConfigName, dockerInfo);
    LOG_INFO(sLogger, ("dump docker path info", dockerPathConfigName));
}

void ConfigManager::LoadDockerConfig() {
    string dockerPathConfigName = AppConfig::GetInstance()->GetDockerFilePathConfig();
    Json::Value dockerPathValueRoot;
    ParseConfResult rst = ParseConfig(dockerPathConfigName, dockerPathValueRoot);
    if (rst == CONFIG_INVALID_FORMAT) {
        LOG_ERROR(sLogger, ("invalid docker config json", rst)("file path", dockerPathConfigName));
    }
    if (rst != CONFIG_OK) {
        return;
    }
    if (!dockerPathValueRoot.isMember("detail")) {
        return;
    }
    Json::Value dockerPathValueDetail = dockerPathValueRoot["detail"];
    if (!dockerPathValueDetail.isArray()) {
        return;
    }
    std::vector<DockerContainerPathCmd*> localPaths;
    for (Json::Value::iterator iter = dockerPathValueDetail.begin(); iter != dockerPathValueDetail.end(); ++iter) {
        const Json::Value& dockerPathItem = *iter;
        string configName = dockerPathItem.isMember("config_name") && dockerPathItem["config_name"].isString()
            ? dockerPathItem["config_name"].asString()
            : string();
        string containerID = dockerPathItem.isMember("container_id") && dockerPathItem["container_id"].isString()
            ? dockerPathItem["container_id"].asString()
            : string();
        string params = dockerPathItem.isMember("params") && dockerPathItem["params"].isString()
            ? dockerPathItem["params"].asString()
            : string();
        LOG_INFO(sLogger, ("load docker path, config", configName)("container id", containerID)("params", params));
        if (configName.empty() || containerID.empty() || params.empty()) {
            continue;
        }

        DockerContainerPathCmd* cmd = new DockerContainerPathCmd(configName, false, params, false);
        localPaths.push_back(cmd);
    }
    mDockerContainerPathCmdLock.lock();
    localPaths.insert(localPaths.end(), mDockerContainerPathCmdVec.begin(), mDockerContainerPathCmdVec.end());
    mDockerContainerPathCmdVec = localPaths;
    mDockerContainerPathCmdLock.unlock();

    DoUpdateContainerPaths();
}

void ConfigManager::ClearFilePipelineMatchCache() {
    ScopedSpinLock lock(mCacheFileConfigMapLock);
    mCacheFileConfigMap.clear();
    ScopedSpinLock allLock(mCacheFileAllConfigMapLock);
    mCacheFileAllConfigMap.clear();
}

#ifdef APSARA_UNIT_TEST_MAIN
void ConfigManager::CleanEnviroments() {
    for (std::unordered_map<std::string, EventHandler*>::iterator iter = mDirEventHandlerMap.begin();
         iter != mDirEventHandlerMap.end();
         ++iter) {
        if (iter->second != mSharedHandler && iter->second)
            delete iter->second;
    }
    mDirEventHandlerMap.clear();
}
#endif

} // namespace logtail
