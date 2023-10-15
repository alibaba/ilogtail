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
#include <boost/filesystem/operations.hpp>
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
#include "common/util.h"
#include "common/LogtailCommonFlags.h"
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
#include "common/GlobalPara.h"
#include "common/version.h"
#include "config/UserLogConfigParser.h"
#include "monitor/LogtailAlarm.h"
#include "monitor/LogFileProfiler.h"
#include "monitor/LogIntegrity.h"
#include "monitor/LogLineCount.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"
#include "app_config/AppConfig.h"
#include "checkpoint/CheckPointManager.h"
#include "event_handler/EventHandler.h"
#include "controller/EventDispatcher.h"
#include "sender/Sender.h"
#include "processor/LogProcess.h"
#include "processor/LogFilter.h"
#include <boost/filesystem.hpp>

using namespace std;
using namespace logtail;

DEFINE_FLAG_STRING(logtail_profile_aliuid, "default user's aliuid", "");
// DEFINE_FLAG_STRING(logtail_profile_access_key_id, "default user's accessKeyId", "");
// DEFINE_FLAG_STRING(logtail_profile_access_key, "default user's LogtailAccessKey", "");
// DEFINE_FLAG_STRING(default_access_key_id, "", "");
// DEFINE_FLAG_STRING(default_access_key, "", "");

// DEFINE_FLAG_INT32(config_update_interval, "second", 10);

DEFINE_FLAG_INT32(file_tags_update_interval, "second", 1);

namespace logtail {

ConfigManager::ConfigManager() {
    // SetDefaultProfileProjectName(STRING_FLAG(profile_project_name));
    // SetDefaultProfileRegion(STRING_FLAG(default_region_name));
}

ConfigManager::~ConfigManager() {
    // try {
    //     if (mCheckUpdateThreadPtr.get() != NULL)
    //         mCheckUpdateThreadPtr->GetValue(100);
    // } catch (...) {
    // }
}

bool ConfigManager::LoadConfig(const std::string& configFile) {
    return false;
}

std::string ConfigManager::CheckPluginFlusher(Json::Value& configJSON) {
    return configJSON.toStyledString();
}

Json::Value& ConfigManager::CheckPluginProcessor(Json::Value& pluginConfigJson, const Json::Value& rootConfigJson) {
    string logTypeStr = GetStringValue(rootConfigJson, "log_type", "plugin");
    if (pluginConfigJson.isMember("processors") // delete this branch if legacy log process can be removed
        && (pluginConfigJson["processors"].isObject() || pluginConfigJson["processors"].isArray())) {
        // patch enable_log_position_meta to split processor if exists ...
        if (rootConfigJson["advanced"] && rootConfigJson["advanced"]["enable_log_position_meta"]) {
            for (size_t i = 0; i < pluginConfigJson["processors"].size(); i++) {
                Json::Value& processorConfigJson = pluginConfigJson["processors"][int(i)];
                if (processorConfigJson["type"] == "processor_split_log_string"
                    || processorConfigJson["type"] == "processor_split_log_regex") {
                    if (processorConfigJson["detail"]) {
                        processorConfigJson["detail"]["EnableLogPositionMeta"]
                            = rootConfigJson["advanced"]["enable_log_position_meta"];
                    }
                    break;
                }
            }
        }
    }
    /*
    if (rootConfigJson["plugin"]
        && !(rootConfigJson["advanced"] && rootConfigJson["force_enable_pipeline"])) {
        return pluginConfigJson;
    }
    // when pipline is used, no need to split/parse/time using plugin
    if (pluginConfigJson.isMember("processors") && !pluginConfigJson.isMember("processors")
        && pluginConfigJson["processors"].size() > 0 && (pluginConfigJson["processors"][0]["type"] ==
    "processor_split_log_string"
            || pluginConfigJson["processors"][0]["type"] == "processor_split_log_regex")) {
        Json::Value& processorConfigJson = pluginConfigJson["processors"][0];
        Json::Value removed;
        pluginConfigJson["processors"].removeIndex(0, &removed);
        if (pluginConfigJson.isMember("processors") && !pluginConfigJson.isMember("processors")
            && pluginConfigJson["processors"].size() > 0 && (pluginConfigJson["processors"][0]["type"] ==
    "processor_regex"
                || pluginConfigJson["processors"][0]["type"] == "processor_json")) {
            Json::Value& processorConfigJson = pluginConfigJson["processors"][0];
            Json::Value removed;
            pluginConfigJson["processors"].removeIndex(0, &removed);
            if (pluginConfigJson.isMember("processors") && !pluginConfigJson.isMember("processors")
                && pluginConfigJson["processors"].size() > 0 && pluginConfigJson["processors"][0]["type"] ==
    "processor_strptime") { Json::Value& processorConfigJson = pluginConfigJson["processors"][0]; Json::Value removed;
                pluginConfigJson["processors"].removeIndex(0, &removed);
            }
        }
    }
    */
    return pluginConfigJson;
}

// ConfigServer


} // namespace logtail
