#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "json/json.h"

#include "common/StringTools.h"
#include "logger/Logger.h"

#define PARAM_ERROR(logger, msg, module, config) \
    LOG_ERROR(logger, ("failed to init pipeline", msg)("module", module)("config", config)); \
    return false
#define PARAM_WARNING_IGNORE(logger, msg, module, config) \
    LOG_WARNING(logger, \
                ("problem encountered during pipeline initialization", \
                 msg)("action", "ignore param")("module", module)("config", config))
#define PARAM_WARNING_DEFAULT(logger, msg, val, module, config) \
    LOG_WARNING(logger, \
                ("problem encountered during pipeline initialization", msg)("action", "use default value instead")( \
                    "default value", ToString(val))("module", module)("config", config))

namespace logtail {
std::string ExtractCurrentKey(const std::string& key);

bool GetOptionalBoolParam(const Json::Value& config, const std::string& key, bool& param, std::string& errorMsg);

bool GetOptionalIntParam(const Json::Value& config, const std::string& key, int32_t& param, std::string& errorMsg);

bool GetOptionalUIntParam(const Json::Value& config, const std::string& key, uint32_t& param, std::string& errorMsg);

bool GetOptionalStringParam(const Json::Value& config,
                            const std::string& key,
                            std::string& param,
                            std::string& errorMsg);

template <class T>
bool GetOptionalListParam(const Json::Value& config,
                          const std::string& key,
                          std::vector<T>& param,
                          std::string& errorMsg) {
    errorMsg.clear();
    const Json::Value* itr = config.find(key.c_str(), key.c_str() + key.length());
    if (itr) {
        if (!itr->isArray()) {
            errorMsg = "param" + key + "is not of type list in plugin ";
            return false;
        }
        for (auto it = itr->begin(); it != itr->end(); ++it) {
            if (std::is_same_v<T, bool>) {
                if (!it->isBool()) {
                    errorMsg = "element in list param" + key + "is not of type bool in plugin ";
                    return false;
                }
                param.emplace_back(it->asBool());
            } else if (std::is_same_v<T, uint32_t>) {
                if (!it->isUInt()) {
                    errorMsg = "element in list param" + key + "is not of type uint in plugin ";
                    return false;
                }
                param.emplace_back(it->asUInt());
            } else if (std::is_same_v<T, int32_t>) {
                if (!it->isInt()) {
                    errorMsg = "element in list param" + key + "is not of type int in plugin ";
                    return false;
                }
                param.emplace_back(it->asInt());
            } else if (std::is_same_v<T, std::string>) {
                if (!it->isString()) {
                    errorMsg = "element in list param" + key + "is not of type string in plugin ";
                    return false;
                }
                param.emplace_back(it->asString());
            } else {
                errorMsg = "element in list param" + key + "is not supported in plugin ";
                return false;
            }
        }
    }
    return true;
}

template <class T>
bool GetOptionalMapParam(const Json::Value& config,
                         const std::string& key,
                         std::unordered_map<std::string, T>& param,
                         std::string& errorMsg) {
    errorMsg.clear();
    const Json::Value* itr = config.find(key.c_str(), key.c_str() + key.length());
    if (itr) {
        if (!itr->isObject()) {
            errorMsg = "param" + key + "is not of type map in plugin ";
            return false;
        }
        for (auto it = itr->begin(); it != itr->end(); ++it) {
            if (std::is_same_v<T, bool>) {
                if (!it->isBool()) {
                    errorMsg = "value in map param" + key + "is not of type bool in plugin ";
                    return false;
                }
                param[it.name()] = it.deref().asBool();
            } else if (std::is_same_v<T, uint32_t>) {
                if (!it->isUInt()) {
                    errorMsg = "value in map param" + key + "is not of type uint in plugin ";
                    return false;
                }
                param[it.name()] = it.deref().asUInt();
            } else if (std::is_same_v<T, int32_t>) {
                if (!it->isInt()) {
                    errorMsg = "value in map param" + key + "is not of type int in plugin ";
                    return false;
                }
                param[it.name()] = it.deref().asInt();
            } else if (std::is_same_v<T, std::string>) {
                if (!it->isString()) {
                    errorMsg = "value in map param" + key + "is not of type string in plugin ";
                    return false;
                }
                param[it.name()] = it.deref().asString();
            } else {
                errorMsg = "value in map param" + key + "is not supported in plugin ";
                return false;
            }
        }
    }
    return true;
}

bool GetMandatoryBoolParam(const Json::Value& config, const std::string& key, bool& param, std::string& errorMsg);

bool GetMandatoryIntParam(const Json::Value& config, const std::string& key, int32_t& param, std::string& errorMsg);

bool GetMandatoryUIntParam(const Json::Value& config, const std::string& key, uint32_t& param, std::string& errorMsg);

bool GetMandatoryStringParam(const Json::Value& config,
                             const std::string& key,
                             std::string& param,
                             std::string& errorMsg);

template <class T>
bool GetMandatoryListParam(const Json::Value& config,
                           const std::string& key,
                           std::vector<T>& param,
                           std::string& errorMsg) {
    errorMsg.clear();
    if (!config.isMember(key)) {
        errorMsg = "madatory param" + key + "is missing in plugin ";
        return false;
    }
    if (!GetOptionalListParam<T>(config, key, param, errorMsg)) {
        return false;
    }
    if (param.empty()) {
        errorMsg = "madatory list param" + key + "is empty in plugin ";
        return false;
    }
}

bool IsRegexValid(const std::string& regStr);
} // namespace logtail
