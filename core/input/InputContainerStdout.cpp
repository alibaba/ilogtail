// Copyright 2023 iLogtail Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "input/InputContainerStdout.h"

#include <filesystem>

#include "app_config/AppConfig.h"
#include "common/JsonUtil.h"
#include "common/LogtailCommonFlags.h"
#include "common/ParamExtractor.h"
#include "config_manager/ConfigManager.h"
#include "file_server/FileServer.h"
#include "pipeline/Pipeline.h"

using namespace std;

DECLARE_FLAG_INT32(default_plugin_log_queue_size);

namespace logtail {

const string InputContainerStdout::sName = "input_container_stdout";

InputContainerStdout::InputContainerStdout() {
}

bool InputContainerStdout::Init(const Json::Value& config, Json::Value& optionalGoPipeline) {
    string errorMsg;

    if (!mFileDiscovery.InitStdout(config, *mContext, sName)) {
        return false;
    }

    // EnableContainerDiscovery
    mEnableContainerDiscovery = true;
    if (!AppConfig::GetInstance()->IsPurageContainerMode()) {
        PARAM_ERROR_RETURN(mContext->GetLogger(),
                           mContext->GetAlarm(),
                           "iLogtail is not in container, but container discovery is required",
                           sName,
                           mContext->GetConfigName(),
                           mContext->GetProjectName(),
                           mContext->GetLogstoreName(),
                           mContext->GetRegion());
    }

    mFileDiscovery.SetEnableContainerDiscoveryFlag(true);
    if (!mContainerDiscovery.Init(config, *mContext, sName)) {
        return false;
    }
    GenerateContainerMetaFetchingGoPipeline(optionalGoPipeline);

    if (!mFileReader.Init(config, *mContext, sName)) {
        return false;
    }

    mFileDiscovery.SetTailingAllMatchedFiles(mFileReader.mTailingAllMatchedFiles);

    // Multiline
    const char* key = "Multiline";
    const Json::Value* itr = config.find(key, key + strlen(key));
    if (itr) {
        if (!itr->isObject()) {
            PARAM_WARNING_IGNORE(mContext->GetLogger(),
                                 mContext->GetAlarm(),
                                 "param Multiline is not of type object",
                                 sName,
                                 mContext->GetConfigName(),
                                 mContext->GetProjectName(),
                                 mContext->GetLogstoreName(),
                                 mContext->GetRegion());
        } else if (!mMultiline.Init(*itr, *mContext, sName)) {
            return false;
        }
    }

    // IgnoringStdout
    if (!GetOptionalBoolParam(config, "IgnoringStdout", mIgnoringStdout, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStdout,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }

    // IgnoringStderr
    if (!GetOptionalBoolParam(config, "IgnoringStderr", mIgnoringStderr, errorMsg)) {
        PARAM_WARNING_DEFAULT(mContext->GetLogger(),
                              mContext->GetAlarm(),
                              errorMsg,
                              mIgnoringStderr,
                              sName,
                              mContext->GetConfigName(),
                              mContext->GetProjectName(),
                              mContext->GetLogstoreName(),
                              mContext->GetRegion());
    }
    return true;
}

bool InputContainerStdout::Start() {
    mFileDiscovery.SetContainerInfo(
        FileServer::GetInstance()->GetAndRemoveContainerInfo(mContext->GetPipeline().Name()));
    FileServer::GetInstance()->AddFileReaderConfig(mContext->GetConfigName(), &mFileReader, mContext);
    FileServer::GetInstance()->AddMultilineConfig(mContext->GetConfigName(), &mMultiline, mContext);
    return true;
}

bool InputContainerStdout::Stop(bool isPipelineRemoving) {
    if (!isPipelineRemoving) {
        FileServer::GetInstance()->SaveContainerInfo(mContext->GetPipeline().Name(), mFileDiscovery.GetContainerInfo());
    }
    FileServer::GetInstance()->RemoveFileReaderConfig(mContext->GetConfigName());
    FileServer::GetInstance()->RemoveMultilineConfig(mContext->GetConfigName());
    return true;
}

void InputContainerStdout::GenerateContainerMetaFetchingGoPipeline(Json::Value& res) const {
    Json::Value plugin(Json::objectValue), detail(Json::objectValue), object(Json::objectValue);
    auto ConvertMapToJsonObj = [&](const char* key, const unordered_map<string, string>& map) {
        if (!map.empty()) {
            object.clear();
            for (const auto& item : map) {
                object[item.first] = Json::Value(item.second);
            }
            detail[key] = object;
        }
    };

    // 传递给 metric_container_meta 的配置
    // 容器过滤
    if (!mContainerDiscovery.mContainerFilters.mK8sNamespaceRegex.empty()) {
        detail["K8sNamespaceRegex"] = Json::Value(mContainerDiscovery.mContainerFilters.mK8sNamespaceRegex);
    }
    if (!mContainerDiscovery.mContainerFilters.mK8sPodRegex.empty()) {
        detail["K8sPodRegex"] = Json::Value(mContainerDiscovery.mContainerFilters.mK8sPodRegex);
    }
    if (!mContainerDiscovery.mContainerFilters.mK8sContainerRegex.empty()) {
        detail["K8sContainerRegex"] = Json::Value(mContainerDiscovery.mContainerFilters.mK8sContainerRegex);
    }
    ConvertMapToJsonObj("IncludeK8sLabel", mContainerDiscovery.mContainerFilters.mIncludeK8sLabel);
    ConvertMapToJsonObj("ExcludeK8sLabel", mContainerDiscovery.mContainerFilters.mExcludeK8sLabel);
    ConvertMapToJsonObj("IncludeEnv", mContainerDiscovery.mContainerFilters.mIncludeEnv);
    ConvertMapToJsonObj("ExcludeEnv", mContainerDiscovery.mContainerFilters.mExcludeEnv);
    ConvertMapToJsonObj("IncludeContainerLabel", mContainerDiscovery.mContainerFilters.mIncludeContainerLabel);
    ConvertMapToJsonObj("ExcludeContainerLabel", mContainerDiscovery.mContainerFilters.mExcludeContainerLabel);
    // 日志标签富化
    ConvertMapToJsonObj("ExternalK8sLabelTag", mContainerDiscovery.mExternalK8sLabelTag);
    ConvertMapToJsonObj("ExternalEnvTag", mContainerDiscovery.mExternalEnvTag);
    // 启用容器元信息预览
    if (mContainerDiscovery.mCollectingContainersMeta) {
        detail["CollectingContainersMeta"] = Json::Value(true);
    }
    detail["InputType"] = Json::Value("stdout");
    plugin["type"] = Json::Value("metric_container_meta");
    plugin["detail"] = detail;

    res["inputs"].append(plugin);
    // these param will be overriden if the same param appears in the global module of config, which will be parsed
    // later.
    res["global"]["DefaultLogQueueSize"] = Json::Value(INT32_FLAG(default_plugin_log_queue_size));
    res["global"]["AlwaysOnline"] = Json::Value(true);
}

} // namespace logtail
