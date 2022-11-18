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

#include <curl/curl.h>
#include "SLSControl.h"
#include "app_config/AppConfig.h"
#include "config_manager/ConfigManager.h"
#include "common/LogtailCommonFlags.h"
#include "common/version.h"
#include "logger/Logger.h"
#include "profiler/LogFileProfiler.h"

using namespace sls_logs;

DEFINE_FLAG_STRING(custom_user_agent, "custom user agent appended at the end of the exsiting ones", "");

namespace logtail {

SLSControl::SLSControl() {
}

SLSControl* SLSControl::Instance() {
    static SLSControl* slsControl = new SLSControl();

    std::string os = LogFileProfiler::mOsDetail;
#if defined(__linux__)
    size_t pos = os.find(';');
    pos = os.find(';', pos + 1);
    os = os.substr(0, pos);
#endif
    std::string userAgent = slsControl->mUserAgent = std::string("ilogtail/") + ILOGTAIL_VERSION + " (" + os + ") ip/"
        + LogFileProfiler::mIpAddr + " env/" + slsControl->GetRunningEnvironment();
    if (!STRING_FLAG(custom_user_agent).empty()) {
        userAgent += " " + STRING_FLAG(custom_user_agent);
    }
    slsControl->mUserAgent = userAgent;

    return slsControl;
}

void SLSControl::SetSlsSendClientCommonParam(sdk::Client* sendClient) {
    sendClient->SetUserAgent(mUserAgent);
    sendClient->SetPort(AppConfig::GetInstance()->GetDataServerPort());
}

bool SLSControl::SetSlsSendClientAuth(const std::string aliuid,
                                      const bool init,
                                      sdk::Client* sendClient,
                                      int32_t& lastUpdateTime) {
    sendClient->SetAccessKeyId(STRING_FLAG(default_access_key_id));
    sendClient->SetAccessKey(STRING_FLAG(default_access_key));
    LOG_INFO(sLogger, ("SetAccessKeyId", STRING_FLAG(default_access_key_id)));
    return true;
}

std::string SLSControl::GetRunningEnvironment() {
    std::string env;
    if (getenv("ALIYUN_LOG_STATIC_CONTAINER_INFO")) {
        env = "ECI";
    } else if (getenv("ACK_NODE_LOCAL_DNS_ADMISSION_CONTROLLER_SERVICE_HOST")) {
        env = "ACK Daemonset";
    } else if (getenv("KUBERNETES_SERVICE_HOST")) {
        if (AppConfig::GetInstance()->IsPurageContainerMode()) {
            env = "K8S Daemonset";
        } else if (TryCurlEndpoint("http://100.100.100.200/latest/meta-data")) {
            env = "ACK Sidecar";
        } else {
            env = "K8S Sidecar";
        }
    } else if (AppConfig::GetInstance()->IsPurageContainerMode() || getenv("ALIYUN_LOGTAIL_CONFIG")) {
        env = "Docker";
    } else if (TryCurlEndpoint("http://100.100.100.200/latest/meta-data")) {
        env = "ECS";
    } else {
        env = "Others";
    }
    return env;
}

bool SLSControl::TryCurlEndpoint(const std::string& endpoint) {
    CURL* curl;
    for (size_t retryTimes = 1; retryTimes <= 5; retryTimes++) {
        curl = curl_easy_init();
        if (curl) {
            break;
        }
        sleep(1);
    }

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);

        if (curl_easy_perform(curl) != CURLE_OK) {
            curl_easy_cleanup(curl);
            return false;
        }
        curl_easy_cleanup(curl);
        return true;
    }

    LOG_WARNING(sLogger,
                ("problem", "curl handler cannot be initialized during user environment identification")(
                    "action", "use Unknown identification for user environment"));
    return false;
}


} // namespace logtail