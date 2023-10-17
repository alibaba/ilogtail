#include "common/ParamExtractor.h"

#include "boost/regex.hpp"

using namespace std;

namespace logtail {
string ExtractCurrentKey(const string& key) {
    size_t pos = key.rfind('.');
    if (pos == string::npos) {
        return key;
    }
    return key.substr(0, pos);
}

bool GetOptionalBoolParam(const Json::Value& config, const string& key, bool& param, string& errorMsg) {
    errorMsg.clear();
    string curKey = ExtractCurrentKey(key);
    const Json::Value* itr = config.find(curKey.c_str(), curKey.c_str() + curKey.length());
    if (itr) {
        if (!itr->isBool()) {
            errorMsg = "param" + key + "is not of type bool";
            return false;
        }
        param = itr->asBool();
    }
    return true;
}

bool GetOptionalIntParam(const Json::Value& config, const string& key, int32_t& param, string& errorMsg) {
    errorMsg.clear();
    string curKey = ExtractCurrentKey(key);
    const Json::Value* itr = config.find(curKey.c_str(), curKey.c_str() + curKey.length());
    if (itr) {
        if (!itr->isInt()) {
            errorMsg = "param" + key + "is not of type int";
            return false;
        }
        param = itr->asInt();
    }
    return true;
}

bool GetOptionalUIntParam(const Json::Value& config, const string& key, uint32_t& param, string& errorMsg) {
    errorMsg.clear();
    string curKey = ExtractCurrentKey(key);
    const Json::Value* itr = config.find(curKey.c_str(), curKey.c_str() + curKey.length());
    if (itr) {
        if (!itr->isUInt()) {
            errorMsg = "param" + key + "is not of type uint";
            return false;
        }
        param = itr->asUInt();
    }
    return true;
}

bool GetOptionalStringParam(const Json::Value& config, const string& key, string& param, string& errorMsg) {
    errorMsg.clear();
    string curKey = ExtractCurrentKey(key);
    const Json::Value* itr = config.find(curKey.c_str(), curKey.c_str() + curKey.length());
    if (itr) {
        if (!itr->isString()) {
            errorMsg = "param" + key + "is not of type string";
            return false;
        }
        param = itr->asString();
    }
    return true;
}

bool GetMandatoryBoolParam(const Json::Value& config, const string& key, bool& param, string& errorMsg) {
    errorMsg.clear();
    if (!config.isMember(ExtractCurrentKey(key))) {
        errorMsg = "madatory param" + key + "is missing";
        return false;
    }
    return GetOptionalBoolParam(config, key, param, errorMsg);
}

bool GetMandatoryIntParam(const Json::Value& config, const string& key, int32_t& param, string& errorMsg) {
    errorMsg.clear();
    if (!config.isMember(ExtractCurrentKey(key))) {
        errorMsg = "madatory param" + key + "is missing";
        return false;
    }
    return GetOptionalIntParam(config, key, param, errorMsg);
}

bool GetMandatoryUIntParam(const Json::Value& config, const string& key, uint32_t& param, string& errorMsg) {
    errorMsg.clear();
    if (!config.isMember(ExtractCurrentKey(key))) {
        errorMsg = "madatory param" + key + "is missing";
        return false;
    }
    return GetOptionalUIntParam(config, key, param, errorMsg);
}

bool GetMandatoryStringParam(const Json::Value& config, const string& key, string& param, string& errorMsg) {
    errorMsg.clear();
    if (!config.isMember(ExtractCurrentKey(key))) {
        errorMsg = "madatory param" + key + "is missing";
        return false;
    }
    if (!GetOptionalStringParam(config, key, param, errorMsg)) {
        return false;
    }
    if (param.empty()) {
        errorMsg = "madatory string param" + key + "is empty";
        return false;
    }
    return true;
}

bool IsRegexValid(const string& regStr) {
    if (regStr.empty()) {
        return true;
    }
    try {
        boost::regex reg(regStr);
    } catch (...) {
        return false;
    }
    return true;
}
} // namespace logtail
