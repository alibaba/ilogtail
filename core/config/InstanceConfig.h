/*
 * Copyright 2023 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <json/json.h>
#include <re2/re2.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace logtail {

struct InstanceConfig {
    std::string mConfigName;
    std::string mDirName;
    std::unique_ptr<Json::Value> mDetail;

    // for alarm only

    InstanceConfig(const std::string& name, std::unique_ptr<Json::Value>&& detail, const std::string& dirName)
        : mConfigName(name), mDirName(dirName), mDetail(std::move(detail)) {}
    InstanceConfig(const logtail::InstanceConfig& config) : mConfigName(config.mConfigName), mDirName(config.mDirName) {
        mDetail = std::make_unique<Json::Value>(*config.mDetail);
    }

    InstanceConfig& operator=(InstanceConfig&& other) {
        if (this != &other) {
            mConfigName = std::move(other.mConfigName);
            mDetail = std::move(other.mDetail);
            mDirName = std::move(other.mDirName);
        }
        return *this;
    }

    InstanceConfig& operator=(const InstanceConfig& other) {
        if (this != &other) {
            mConfigName = other.mConfigName;
            mDetail = std::make_unique<Json::Value>(*other.mDetail);
            mDirName = other.mDirName;
        }
        return *this;
    }

    const Json::Value& GetConfig() const { return *mDetail; }
};

inline bool operator==(const InstanceConfig& lhs, const InstanceConfig& rhs) {
    return (lhs.mConfigName == rhs.mConfigName) && (*lhs.mDetail == *rhs.mDetail) && (lhs.mDirName == rhs.mDirName);
}

inline bool operator!=(const InstanceConfig& lhs, const InstanceConfig& rhs) {
    return !(lhs == rhs);
}

} // namespace logtail
