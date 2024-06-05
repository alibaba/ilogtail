// Copyright 2024 iLogtail Authors
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

#include "config/provider/LegacyCommonConfigProvider.h"

#include <json/json.h>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <random>

#include "app_config/AppConfig.h"
#include "application/Application.h"
#include "common/FileSystemUtil.h"
#include "common/LogtailCommonFlags.h"
#include "common/StringTools.h"
#include "common/version.h"
#include "logger/Logger.h"
#include "monitor/LogFileProfiler.h"
#include "sdk/Common.h"
#include "sdk/CurlImp.h"
#include "sdk/Exception.h"

using namespace std;


DECLARE_FLAG_INT32(config_update_interval);
DECLARE_FLAG_STRING(ilogtail_local_yaml_config_dir);

namespace logtail {

void LegacyCommonConfigProvider::Init(const string& dir) {
    ConfigProvider::Init(dir);
    mLegacySourceDir.assign(AppConfig::GetInstance()->GetLogtailSysConfDir());
    mLegacySourceDir /= STRING_FLAG(ilogtail_local_yaml_config_dir);

    mThreadRes = async(launch::async, &LegacyCommonConfigProvider::CheckUpdateThread, this);
}

void LegacyCommonConfigProvider::Stop() {
    {
        lock_guard<mutex> lock(mThreadRunningMux);
        mIsThreadRunning = false;
    }
    mStopCV.notify_one();
    future_status s = mThreadRes.wait_for(chrono::seconds(1));
    if (s == future_status::ready) {
        LOG_INFO(sLogger, ("legacy common config provider", "stopped successfully"));
    } else {
        LOG_WARNING(sLogger, ("legacy common config provider", "forced to stopped"));
    }
}

void LegacyCommonConfigProvider::CheckUpdateThread() {
    LOG_INFO(sLogger, ("legacy common config provider", "started"));
    usleep((rand() % 10) * 100 * 1000);
    int32_t lastCheckTime = 0;
    unique_lock<mutex> lock(mThreadRunningMux);
    while (mIsThreadRunning) {
        int32_t curTime = time(NULL);
        if (curTime - lastCheckTime >= INT32_FLAG(config_update_interval)) {
            GetConfigUpdate();
            lastCheckTime = curTime;
        }
        if (mStopCV.wait_for(lock, chrono::seconds(3), [this]() { return !mIsThreadRunning; })) {
            break;
        }
    }
}


void LegacyCommonConfigProvider::GetConfigUpdate() {
    if (IsLegacyConfigDirExisted()) {
        for (const auto& entry : filesystem::directory_iterator(mLegacySourceDir)) {                        
            if (entry.is_regular_file() || entry.is_symlink()) { 
                string filename = entry.path().filename().string();
                // skip non-yaml files                
                if (!EndWith(filename,".yaml") && !EndWith(filename, ".yml")) {
                    continue;
                }

                auto fileTime = filesystem::last_write_time(entry);
                int64_t modTime = std::chrono::duration_cast<std::chrono::nanoseconds>(fileTime.time_since_epoch()).count();

                bool needUpdate = false;
                if (mConfigNameTimestampMap.find(filename) != mConfigNameTimestampMap.end()) {
                    auto stored_mod_time = mConfigNameTimestampMap[filename];
                    if (modTime != stored_mod_time) {                        
                        needUpdate = true;  // File has been modified, update the map
                    }
                } else {           
                    needUpdate = true; // File is new, add it to the map         
                }

                if (needUpdate) {
                    mConfigNameTimestampMap[filename] = modTime;
                    // Read the YAML content from the file
                    string inputFilePath = entry.path();
                    string outputFilePath =  (ConfigProvider::mSourceDir/filename).string();
                    ConvertLegacyYamlAndStore(inputFilePath, outputFilePath);
                }
            }
        }
    }
}



void ConvertLegacyYamlAndStore(const string& inputFilePath, const string& outputFilePath) {
    string content;
    if (!ReadFile(inputFilePath, content)) {
        LOG_WARNING(sLogger, ("failed to open config file", "skip current object")("filepath", inputFilePath));
        return;
    }

    YAML::Node yamlContent;
    string errorMsg;
    if (!ParseYamlTable(content, yamlContent, errorMsg)) {
        LOG_ERROR(sLogger,("failed to parse yaml", errorMsg));
        return;
    }
    
    // is yamContent is nil or empty, return
    if (!yamlContent.IsDefined() || yamlContent.IsNull()) {
        return;
    }

    bool success = UpdateLegacyConfigYaml(yamlContent, errorMsg);
    if (!success) {
        LOG_ERROR(sLogger,("failed to update legacy config yaml", errorMsg));
        return;
    }

    YAML::Emitter out;
    EmitYamlWithQuotes(yamlContent, out);

    // Store the YAML content to the tmp output file
    string tempOutputFilePath = outputFilePath + ".new";
    ofstream tempOutputFile(tempOutputFilePath);

    if (!tempOutputFile.is_open()) {
        LOG_ERROR(sLogger,("failed to open the output file", tempOutputFilePath));
        return;
    }

    tempOutputFile <<  out.c_str();
    tempOutputFile.close();

    // Rename the tmp output file to the final output file
    error_code errorCode;
    filesystem::rename(tempOutputFilePath, outputFilePath, errorCode);

    if (errorCode) {
        LOG_ERROR(sLogger,("failed to rename the output file", tempOutputFilePath)(
            "error code", errorCode.value())("error msg", errorCode.message()));
    }

    filesystem::remove(tempOutputFilePath);
}

} // namespace logtail
