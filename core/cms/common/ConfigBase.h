//
// Created by 韩呈杰 on 2023/2/19.
//

#ifndef ARGUSAGENT_CONFIG_BASE_H
#define ARGUSAGENT_CONFIG_BASE_H

#include <string>
#include "common/StringUtils.h"

class ConfigBase {
public:
    virtual ~ConfigBase() {}

    virtual std::string GetValue(const std::string &key, const std::string &defaultValue) const = 0;

    // NOTE: 需要在子类中加上如下语句，以免名称遮盖
    // using ConfigBase::GetValue;

    // true: 使用了配置值，false: 使用了默认值
    template<typename T, typename TCompatible, typename std::enable_if<
            !std::is_same<bool, T>::value && !std::is_same<bool, TCompatible>::value, int>::type = 0>
    bool GetValue(const std::string &key, T &value, const TCompatible &defaultValue) const {
        static_assert(!std::is_pointer<T>::value, "T should not be a pointer");
        static_assert(std::is_convertible<TCompatible, T>::value, "TCompatible and T not compatible");

        value = defaultValue;
        std::string strValue = this->GetValue(key, std::string{});
        return !strValue.empty() && convert(strValue, value);
    }

    // !bool
    template<typename T, typename std::enable_if<!std::is_same<bool, T>::value, int>::type = 0>
    T GetValue(const std::string &key, const T &defaultValue) const {
        T ret;
        this->GetValue(key, ret, defaultValue);
        return ret;
    }

    // bool
    template<typename T, typename std::enable_if<std::is_same<bool, T>::value, int>::type = 0>
    T GetValue(const std::string &key, const T &defaultValue) const {
        std::string v = GetValue<std::string>(key, std::string{});
        return v.empty() ? defaultValue : convert<bool>(v);
    }

    std::string GetValue(const std::string &key, const char *defaultValue) const {
        return GetValue<std::string>(key, std::string(defaultValue ? defaultValue : ""));
    }
};

#endif //ARGUSAGENT_CONFIG_BASE_H
