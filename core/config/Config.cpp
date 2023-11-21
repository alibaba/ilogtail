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

#include "config/Config.h"

#include <string>

#include "common/Flags.h"
#include "common/JsonUtil.h"
#include "common/ParamExtractor.h"
#include "common/YamlUtil.h"
#include "plugin/PluginRegistry.h"

DEFINE_FLAG_BOOL(enable_env_ref_in_config, "enable environment variable reference replacement in configuration", false);

using namespace std;

namespace logtail {

static string UnescapeDollar(string::const_iterator beginIt, string::const_iterator endIt) {
    string outStr;
    string::const_iterator lastMatchEnd = beginIt;
    static boost::regex reg(R"(\$\$|\$})");
    boost::regex_iterator<string::const_iterator> it{beginIt, endIt, reg};
    boost::regex_iterator<string::const_iterator> end;
    for (; it != end; ++it) {
        outStr.append(lastMatchEnd, (*it)[0].first); // original part
        outStr.append((*it)[0].first + 1, (*it)[0].second); // skip $ char
        lastMatchEnd = (*it)[0].second;
    }
    outStr.append(lastMatchEnd, endIt); // original part
    return outStr;
}

static string ReplaceEnvVarRefInStr(const string& inStr) {
    string outStr;
    string::const_iterator lastMatchEnd = inStr.begin();
    static boost::regex reg(R"((?<!\$)\${([\w]+)(:(.*?))?(?<!\$)})");
    boost::regex_iterator<string::const_iterator> it{inStr.begin(), inStr.end(), reg};
    boost::regex_iterator<string::const_iterator> end;
    for (; it != end; ++it) {
        outStr.append(UnescapeDollar(lastMatchEnd, (*it)[0].first)); // original part
        char* env = getenv((*it)[1].str().c_str());
        if (env != NULL) // replace to enviroment variable
        {
            outStr.append(env);
        } else if ((*it).size() == 4) // replace to default value
        {
            outStr.append((*it)[3].first, (*it)[3].second);
        }
        // else replace to empty string (do nothing)
        lastMatchEnd = (*it)[0].second;
    }
    outStr.append(UnescapeDollar(lastMatchEnd, inStr.end())); // original part
    return outStr;
}

static void ReplaceEnvVarRef(Json::Value& value) {
    if (value.isString()) {
        Json::Value tempValue{ReplaceEnvVarRefInStr(value.asString())};
        value.swapPayload(tempValue);
    } else if (value.isArray()) {
        Json::ValueIterator it = value.begin();
        Json::ValueIterator end = value.end();
        for (; it != end; ++it) {
            ReplaceEnvVarRef(*it);
        }
    } else if (value.isObject()) {
        Json::ValueIterator it = value.begin();
        Json::ValueIterator end = value.end();
        for (; it != end; ++it) {
            ReplaceEnvVarRef(*it);
        }
    }
}

bool Config::Parse() {
    if (BOOL_FLAG(enable_env_ref_in_config)) {
        ReplaceEnvVar();
    }

    string errorMsg;
    if (!GetOptionalUIntParam(mDetail, "createTime", mCreateTime, errorMsg)) {
        PARAM_WARNING_DEFAULT(sLogger, errorMsg, 0, noModule, mName);
    }

    string key = "global";
    const Json::Value* itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (itr) {
        if (!itr->isObject()) {
            PARAM_ERROR_RETURN(sLogger, "global module is not of type object", noModule, mName);
        }
        mGlobal = itr;
    }

    // inputs, processors and flushers module must be parsed first and parsed by order, since aggregators and
    // extensions module parsing will rely on their results.
    bool hasObserverInput = false;
#ifdef __ENTERPRISE__
    bool hasStreamInput = false;
#endif
    key = "inputs";
    itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (!itr) {
        PARAM_ERROR_RETURN(sLogger, "mandatory inputs module is missing", noModule, mName);
    }
    if (!itr->isArray()) {
        PARAM_ERROR_RETURN(sLogger, "mandatory inputs module is not of type array", noModule, mName);
    }
    if (itr->empty()) {
        PARAM_ERROR_RETURN(sLogger, "mandatory inputs module has no plugin", noModule, mName);
    }
    for (Json::Value::ArrayIndex i = 0; i < itr->size(); ++i) {
        const Json::Value& plugin = (*itr)[i];
        if (!plugin.isObject()) {
            PARAM_ERROR_RETURN(sLogger, "param inputs[" + ToString(i) + "] is not of type object", noModule, mName);
        }
        key = "Type";
        const Json::Value* it = plugin.find(key.c_str(), key.c_str() + key.size());
        if (it == nullptr) {
            PARAM_ERROR_RETURN(sLogger, "param inputs[" + ToString(i) + "].Type is missing", noModule, mName);
        }
        if (!it->isString()) {
            PARAM_ERROR_RETURN(
                sLogger, "param inputs[" + ToString(i) + "].Type is not of type string", noModule, mName);
        }
        const string pluginName = it->asString();
        if (i == 0) {
            if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                mHasGoInput = true;
            } else if (PluginRegistry::GetInstance()->IsValidNativeInputPlugin(pluginName)) {
                mHasNativeInput = true;
            } else {
                PARAM_ERROR_RETURN(sLogger, "unsupported input plugin", pluginName, mName);
            }
        } else {
            if (mHasGoInput) {
                if (PluginRegistry::GetInstance()->IsValidNativeInputPlugin(pluginName)) {
                    PARAM_ERROR_RETURN(sLogger, "native and extended input plugins coexist", noModule, mName);
                } else if (!PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                    PARAM_ERROR_RETURN(sLogger, "unsupported input plugin", pluginName, mName);
                }
            } else {
                if (PluginRegistry::GetInstance()->IsValidNativeInputPlugin(pluginName)) {
                    PARAM_ERROR_RETURN(sLogger, "more than 1 native input plugin is given", noModule, mName);
                } else if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                    PARAM_ERROR_RETURN(sLogger, "native and extended input plugins coexist", noModule, mName);
                } else {
                    PARAM_ERROR_RETURN(sLogger, "unsupported input plugin", pluginName, mName);
                }
            }
        }
        mInputs.push_back(&plugin);
        if (pluginName == "input_observer_network") {
            hasObserverInput = true;
#ifdef __ENTERPRISE__
        } else if (pluginName == "input_stream") {
            hasStreamInput = true;
#endif
        }
    }

    key = "processors";
    itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (itr) {
        if (!itr->isArray()) {
            PARAM_ERROR_RETURN(sLogger, "processors module is not of type array", noModule, mName);
        }
#ifdef __ENTERPRISE__
        if (hasStreamInput && !itr->empty()) {
            PARAM_ERROR_RETURN(sLogger, "processor plugins coexist with input_stream", noModule, mName);
        }
#endif
        bool isCurrentPluginNative = true;
        for (Json::Value::ArrayIndex i = 0; i < itr->size(); ++i) {
            const Json::Value& plugin = (*itr)[i];
            if (!plugin.isObject()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param processors[" + ToString(i) + "] is not of type object", noModule, mName);
            }
            key = "Type";
            const Json::Value* it = plugin.find(key.c_str(), key.c_str() + key.size());
            if (it == nullptr) {
                PARAM_ERROR_RETURN(sLogger, "param processors[" + ToString(i) + "].Type is missing", noModule, mName);
            }
            if (!it->isString()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param processors[" + ToString(i) + "].Type is not of type string", noModule, mName);
            }
            const string pluginName = it->asString();
            if (mHasGoInput) {
                if (PluginRegistry::GetInstance()->IsValidNativeProcessorPlugin(pluginName)) {
                    PARAM_ERROR_RETURN(
                        sLogger, "native processor plugins coexist with extended input plugins", noModule, mName);
                } else if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                    mHasGoProcessor = true;
                } else {
                    PARAM_ERROR_RETURN(sLogger, "unsupported processor plugin", pluginName, mName);
                }
            } else {
                if (isCurrentPluginNative) {
                    if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                        if (hasObserverInput) {
                            PARAM_ERROR_RETURN(sLogger,
                                               "native processor plugins coexist with input_observer_network",
                                               noModule,
                                               mName);
                        }
                        isCurrentPluginNative = false;
                        mHasGoProcessor = true;
                    } else if (!PluginRegistry::GetInstance()->IsValidNativeProcessorPlugin(pluginName)) {
                        PARAM_ERROR_RETURN(sLogger, "unsupported processor plugin", pluginName, mName);
                    } else if (pluginName == "processor_spl" && (i != 0 || itr->size() != 1)) {
                        PARAM_ERROR_RETURN(
                            sLogger, "native processor plugins coexist with spl processor", noModule, mName);
                    } else {
                        mHasNativeProcessor = true;
                    }
                } else {
                    if (PluginRegistry::GetInstance()->IsValidNativeProcessorPlugin(pluginName)) {
                        PARAM_ERROR_RETURN(sLogger,
                                           "native processor plugin comes after extended processor plugin",
                                           pluginName,
                                           mName);
                    } else if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                        mHasGoProcessor = true;
                    } else {
                        PARAM_ERROR_RETURN(sLogger, "unsupported processor plugin", pluginName, mName);
                    }
                }
            }
            mProcessors.push_back(&plugin);
            if (i == 0) {
                if (pluginName == "processor_parse_json_native" || pluginName == "processor_json") {
                    mIsFirstProcessorJson = true;
                }
            }
        }
    }

    key = "flushers";
    itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (!itr) {
        PARAM_ERROR_RETURN(sLogger, "mandatory flushers module is missing", noModule, mName);
    }
    if (!itr->isArray()) {
        PARAM_ERROR_RETURN(sLogger, "mandatory flushers module is not of type array", noModule, mName);
    }
    if (itr->empty()) {
        PARAM_ERROR_RETURN(sLogger, "mandatory flushers module has no plugin", noModule, mName);
    }
    for (Json::Value::ArrayIndex i = 0; i < itr->size(); ++i) {
        const Json::Value& plugin = (*itr)[i];
        if (!plugin.isObject()) {
            PARAM_ERROR_RETURN(sLogger, "param flushers[" + ToString(i) + "] is not of type object", noModule, mName);
        }
        key = "Type";
        const Json::Value* it = plugin.find(key.c_str(), key.c_str() + key.size());
        if (it == nullptr) {
            PARAM_ERROR_RETURN(sLogger, "param flushers[" + ToString(i) + "].Type is missing", noModule, mName);
        }
        if (!it->isString()) {
            PARAM_ERROR_RETURN(
                sLogger, "param flushers[" + ToString(i) + "].Type is not of type string", noModule, mName);
        }
        const string pluginName = it->asString();
        if (PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
            mHasGoFlusher = true;
        } else if (PluginRegistry::GetInstance()->IsValidNativeFlusherPlugin(pluginName)) {
            mHasNativeFlusher = true;
        } else {
            PARAM_ERROR_RETURN(sLogger, "unsupported flusher plugin", pluginName, mName);
        }
#ifdef __ENTERPRISE__
        if (hasStreamInput && pluginName != "flusher_sls") {
            PARAM_ERROR_RETURN(
                sLogger, "flusher plugins other than flusher_sls coexist with input_stream", noModule, mName);
        }
#endif
        mFlushers.push_back(&plugin);
    }

    key = "aggregators";
    itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (itr) {
        if (!IsFlushingThroughGoPipelineExisted()) {
            PARAM_ERROR_RETURN(sLogger, "aggregator plugins exist in native flushing mode", noModule, mName);
        }
        if (!itr->isArray()) {
            PARAM_ERROR_RETURN(sLogger, "aggregators module is not of type array", noModule, mName);
        }
        if (itr->size() != 1) {
            PARAM_ERROR_RETURN(sLogger, "more than 1 aggregator is given", noModule, mName);
        }
        for (Json::Value::ArrayIndex i = 0; i < itr->size(); ++i) {
            const Json::Value& plugin = (*itr)[i];
            if (!plugin.isObject()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param aggregators[" + ToString(i) + "] is not of type object", noModule, mName);
            }
            key = "Type";
            const Json::Value* it = plugin.find(key.c_str(), key.c_str() + key.size());
            if (it == nullptr) {
                PARAM_ERROR_RETURN(sLogger, "param aggregators[" + ToString(i) + "].Type is missing", noModule, mName);
            }
            if (!it->isString()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param aggregators[" + ToString(i) + "].Type is not of type string", noModule, mName);
            }
            const string pluginName = it->asString();
            if (!PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                PARAM_ERROR_RETURN(sLogger, "unsupported aggregator plugin", pluginName, mName);
            }
            mAggregators.push_back(&plugin);
        }
    }

    key = "extensions";
    itr = mDetail.find(key.c_str(), key.c_str() + key.size());
    if (itr) {
        if (!HasGoPlugin()) {
            PARAM_ERROR_RETURN(sLogger, "extension plugins exist when no extended plugin is given", noModule, mName);
        }
        if (!itr->isArray()) {
            PARAM_ERROR_RETURN(sLogger, "extensions module is not of type array", noModule, mName);
        }
        for (Json::Value::ArrayIndex i = 0; i < itr->size(); ++i) {
            const Json::Value& plugin = (*itr)[i];
            if (!plugin.isObject()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param extensions[" + ToString(i) + "] is not of type object", noModule, mName);
            }
            key = "Type";
            const Json::Value* it = plugin.find(key.c_str(), key.c_str() + key.size());
            if (it == nullptr) {
                PARAM_ERROR_RETURN(sLogger, "param extensions[" + ToString(i) + "].Type is missing", noModule, mName);
            }
            if (!it->isString()) {
                PARAM_ERROR_RETURN(
                    sLogger, "param extensions[" + ToString(i) + "].Type is not of type string", noModule, mName);
            }
            const string pluginName = it->asString();
            if (!PluginRegistry::GetInstance()->IsValidGoPlugin(pluginName)) {
                PARAM_ERROR_RETURN(sLogger, "unsupported extension plugin", pluginName, mName);
            }
            mExtensions.push_back(&plugin);
        }
    }

    return true;
}

void Config::ReplaceEnvVar() {
    ReplaceEnvVarRef(mDetail);
}

bool ParseConfigDetail(const string& content, const string& extension, Json::Value& detail, string& errorMsg) {
    if (extension == ".json") {
        return ParseJsonTable(content, detail, errorMsg);
    } else if (extension == ".yaml" || extension == ".yml") {
        YAML::Node yamlRoot;
        if (!ParseYamlTable(content, yamlRoot, errorMsg)){
            return false;
        }
        detail = ConvertYamlToJson(yamlRoot);
        return true;
    }
    return false;
}

} // namespace logtail
