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

enum class ObserverType { PROCESS, FILE, NETWORK };

#define BOOL_DEFAULT false
#define STRING_DEFAULT ""

class ObserverProcess {
public:
    std::vector<std::string> mIncludeCmdRegex;
    std::vector<std::string> mExcludeCmdRegex;
};

class ObserverFile {
public:
    std::string mProfileRemoteServer = STRING_DEFAULT;
    bool mCpuSkipUpload = BOOL_DEFAULT;
    bool mMemSkipUpload = BOOL_DEFAULT;
};

class ObserverNetwork {
public:
    std::vector<std::string> mEnableProtocols;
    bool mDisableProtocolParse = BOOL_DEFAULT;
    bool mDisableConnStats = BOOL_DEFAULT;
    bool mEnableConnTrackerDump = BOOL_DEFAULT;
};


class ObserverOptions {
public:
    bool Init(ObserverType type, const Json::Value& config, const PipelineContext* mContext, const std::string& sName);

    std::variant<ObserverProcess, ObserverFile, ObserverNetwork> mObserver;
    ObserverType mType;
};

} // namespace logtail