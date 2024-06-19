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
#include <variant>
#include <vector>

#include "pipeline/PipelineContext.h"

namespace logtail {

enum class SecurityFilterType { PROCESS, FILE, NETWORK };
// TODO:重命名具体到每个参数
#define BOOL_DEFAULT false
#define STRING_DEFAULT ""

// file
struct SecurityFileFilterItem {
    std::string mFilePath = STRING_DEFAULT;
    std::string mFileName = STRING_DEFAULT;
};
struct SecurityFileFilter {
    std::vector<SecurityFileFilterItem> mFileFilterItem;
};

// process
struct SecurityProcessNamespaceFilter {
    // type of securityNamespaceFilter
    std::string mNamespaceType = STRING_DEFAULT;
    std::vector<std::string> mValueList;
};
struct SecurityProcessFilter {
    std::vector<SecurityProcessNamespaceFilter> mNamespaceFilter;
    std::vector<SecurityProcessNamespaceFilter> mNamespaceBlackFilter;
    // std::vector<std::string> mIp;
};

// network
struct SecurityNetworkFilter {
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

    std::vector<std::string> mCallName;
    std::variant<SecurityFileFilter, SecurityNetworkFilter, SecurityProcessFilter> mFilter;
    static bool IsProcessNamespaceFilterTypeValid(std::string type);
};

class SecurityOptions {
public:
    bool Init(SecurityFilterType filterType,
              const Json::Value& config,
              const PipelineContext* mContext,
              const std::string& sName);

    std::vector<SecurityOption> mOptionList;
    SecurityFilterType mFilterType;
};

using SecurityConfig = std::pair<const SecurityOptions*, const PipelineContext*>;

} // namespace logtail
