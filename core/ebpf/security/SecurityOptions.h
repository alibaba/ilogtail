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

#include <string>
#include <vector>

#include "pipeline/PipelineContext.h"

namespace logtail {

enum class SecurityFilterType { PROCESS, FILE, NETWORK };

class SecurityFilter {
public:
    // type of filter: process, file, network
    SecurityFilterType mFilterType;
    virtual ~SecurityFilter() = default;
};

// file
class SecurityFileFilterItem {
public:
    std::string mFilePath;
    std::string mFileName;
};
class SecurityFileFilter : public SecurityFilter {
public:
    std::vector<SecurityFileFilterItem*> mFileFilterItem;

private:
    ~SecurityFileFilter() override {
        for (auto item : mFileFilterItem) {
            delete item;
        }
    }
};

// process
class SecurityProcessNamespaceFilter {
public:
    // type of securityNamespaceFilter
    std::string mType;
    std::vector<std::string> mValueList;
};
class SecurityProcessFilter : public SecurityFilter {
public:
    std::vector<SecurityProcessNamespaceFilter*> mNamespaceFilter;
    std::vector<SecurityProcessNamespaceFilter*> mNamespaceBlackFilter;
    // std::vector<std::string> mIp;
private:
    ~SecurityProcessFilter() override {
        for (auto ns : mNamespaceFilter) {
            delete ns;
        }
        for (auto ns : mNamespaceBlackFilter) {
            delete ns;
        }
    }
};

// network
class SecurityNetworkFilter : public SecurityFilter {
public:
    std::vector<std::string> mDestAddrList;
    std::vector<uint32_t> mDestPortList;
    std::vector<std::string> mDestAddrBlackList;
    std::vector<uint32_t> mDestPortBlackList;
    std::vector<std::string> mSourceAddrList;
    std::vector<uint32_t> mSourcePortList;
    std::vector<std::string> mSourceAddrBlackList;
    std::vector<uint32_t> mSourcePortBlackList;
};

class SecurityOption {
public:
    bool Init(SecurityFilterType filterType,
              const Json::Value& config,
              const PipelineContext* mContext,
              const std::string& sName);
    // todo app_config中定义的进程级别配置获取

    std::vector<std::string> mCallName;
    SecurityFilter* mFilter;
    // std::vector<SecurityReturnType> mReturnType;
    ~SecurityOption() { delete mFilter; }
    bool IsProcessNamespaceFilterTypeValid(std::string type);
};

// class SecurityReturnType{};

class SecurityOptions {
public:
    bool Init(SecurityFilterType filterType,
              const Json::Value& config,
              const PipelineContext* mContext,
              const std::string& sName);

    std::vector<SecurityOption*> mOptionList;
};

} // namespace logtail