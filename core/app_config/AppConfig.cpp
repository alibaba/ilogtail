// Copyright 2022 iLogtail Authors
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

#include "AppConfig.h"
#include "common/StringTools.h"
#include "logger/Logger.h"
#include "common/util.h"

using namespace std;

namespace logtail {

AppConfig::AppConfig() {
}

AppConfig::~AppConfig() {
}

void AppConfig::LoadAddrConfig(const Json::Value& confJson) {
    if (confJson.isMember("bind_interface") && confJson["bind_interface"].isString()) {
        mBindInterface = TrimString(confJson["bind_interface"].asString());
        if (ToLowerCaseString(mBindInterface) == "default")
            mBindInterface.clear();
        LOG_INFO(sLogger, ("bind_interface", mBindInterface));
    }

    // configserver path
    if (confJson.isMember("ilogtail_configserver_address") && confJson["ilogtail_configserver_address"].isObject()) {
        Json::Value::Members members = confJson["ilogtail_configserver_address"].getMemberNames();
        for (Json::Value::Members::iterator it = members.begin(); it != members.end(); it++) {
            vector<string> configServerAddress = SplitString(TrimString(confJson["ilogtail_configserver_address"][*it].asString()), ":");
            mConfigServerAddress.push_back(ConfigServerAddress(configServerAddress[0], atoi(configServerAddress[1].c_str())));
        }

        LOG_INFO(sLogger, ("ilogtail_configserver_address", confJson["ilogtail_configserver_address"].toStyledString()));
    }

    // tags for configserver
    if (confJson.isMember("ilogtail_tags") && confJson["ilogtail_tags"].isObject()) {
        Json::Value::Members members = confJson["ilogtail_tags"].getMemberNames();
        for (Json::Value::Members::iterator it = members.begin(); it != members.end(); it++) {
            mConfigServerTags.insert(pair<string, string>(*it, confJson["ilogtail_tags"][*it].asString()));
        }

        LOG_INFO(sLogger, ("ilogtail_configserver_tags", confJson["ilogtail_tags"].toStyledString()));
    }
}

const AppConfig::ConfigServerAddress& AppConfig::GetConfigServerAddress() const {
    if (0 == mConfigServerAddress.size()) return AppConfig::ConfigServerAddress("", -1);
    std::random_device rd; 
    return mConfigServerAddress[rd()%mConfigServerAddress.size()];
}

} // namespace logtail