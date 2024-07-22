/*
 * Copyright 2024 iLogtail Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ScrapeJob.h"

#include <curl/curl.h>

#include "Common.h"
#include "Constants.h"
#include "CurlImp.h"
#include "Exception.h"
#include "ScrapeWork.h"
#include "common/FileSystemUtil.h"
#include "common/StringTools.h"
#include "common/TimeUtil.h"
#include "logger/Logger.h"

using namespace std;

namespace logtail {

const int sRefeshIntervalSeconds = 5;

string url_encode(const string& value) {
    ostringstream escaped;
    escaped.fill('0');
    escaped << hex;

    for (char c : value) {
        // Keep alphanumeric characters and other safe characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        // Any other characters are percent-encoded
        escaped << '%' << setw(2) << int((unsigned char)c);
    }

    return escaped.str();
}

ScrapeJob::ScrapeJob() {
    mClient = make_unique<sdk::CurlClient>();
}

/// @brief Construct from json config
bool ScrapeJob::Init(const Json::Value& scrapeConfig) {
    mScrapeConfig = scrapeConfig;
    if (mScrapeConfig.isMember(ilogtail::prometheus::JOB_NAME)
        && mScrapeConfig[ilogtail::prometheus::JOB_NAME].isString()) {
        mJobName = mScrapeConfig[ilogtail::prometheus::JOB_NAME].asString();
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::SCHEME)
        && mScrapeConfig[ilogtail::prometheus::SCHEME].isString()) {
        mScheme = mScrapeConfig[ilogtail::prometheus::SCHEME].asString();
    } else {
        mScheme = ilogtail::prometheus::HTTP_SCHEME;
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::METRICS_PATH)
        && mScrapeConfig[ilogtail::prometheus::METRICS_PATH].isString()) {
        mMetricsPath = mScrapeConfig[ilogtail::prometheus::METRICS_PATH].asString();
    } else {
        mMetricsPath = "/metrics";
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::SCRAPE_INTERVAL)
        && mScrapeConfig[ilogtail::prometheus::SCRAPE_INTERVAL].isString()) {
        mScrapeIntervalString = mScrapeConfig[ilogtail::prometheus::SCRAPE_INTERVAL].asString();
    } else {
        mScrapeIntervalString = "30s";
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::SCRAPE_TIMEOUT)
        && mScrapeConfig[ilogtail::prometheus::SCRAPE_TIMEOUT].isString()) {
        mScrapeTimeoutString = mScrapeConfig[ilogtail::prometheus::SCRAPE_TIMEOUT].asString();
    } else {
        mScrapeTimeoutString = "10s";
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::PARAMS)
        && mScrapeConfig[ilogtail::prometheus::PARAMS].isObject()) {
        const Json::Value& params = mScrapeConfig[ilogtail::prometheus::PARAMS];
        if (params.isObject()) {
            for (const auto& key : params.getMemberNames()) {
                const Json::Value& values = params[key];
                if (values.isArray()) {
                    vector<string> valueList;
                    for (const auto& value : values) {
                        valueList.push_back(value.asString());
                    }
                    mParams[key] = valueList;
                }
            }
        }
    }
    if (mScrapeConfig.isMember(ilogtail::prometheus::AUTHORIZATION)
        && mScrapeConfig[ilogtail::prometheus::AUTHORIZATION].isObject()) {
        string type = mScrapeConfig[ilogtail::prometheus::AUTHORIZATION][ilogtail::prometheus::TYPE].asString();
        string bearerToken;
        bool b = ReadFile(
            mScrapeConfig[ilogtail::prometheus::AUTHORIZATION][ilogtail::prometheus::CREDENTIALS_FILE].asString(),
            bearerToken);
        if (!b) {
            LOG_ERROR(sLogger,
                      ("read credentials_file failed, credentials_file",
                       mScrapeConfig[ilogtail::prometheus::AUTHORIZATION][ilogtail::prometheus::CREDENTIALS_FILE]
                           .asString()));
        }
        mHeaders[sdk::AUTHORIZATION] = type + " " + bearerToken;
        LOG_INFO(
            sLogger,
            ("read credentials_file success, credentials_file",
             mScrapeConfig[ilogtail::prometheus::AUTHORIZATION][ilogtail::prometheus::CREDENTIALS_FILE].asString())(
                sdk::AUTHORIZATION, mHeaders[sdk::AUTHORIZATION]));
    }

    // TODO: 实现服务发现
    vector<string> sdConfigs
        = {"azure_sd_configs",       "consul_sd_configs",     "digitalocean_sd_configs", "docker_sd_configs",
           "dockerswarm_sd_configs", "dns_sd_configs",        "ec2_sd_configs",          "eureka_sd_configs",
           "file_sd_configs",        "gce_sd_configs",        "hetzner_sd_configs",      "http_sd_configs",
           "ionos_sd_configs",       "kubernetes_sd_configs", "kuma_sd_configs",         "lightsail_sd_configs",
           "linode_sd_configs",      "marathon_sd_configs",   "nerve_sd_configs",        "nomad_sd_configs",
           "openstack_sd_configs",   "ovhcloud_sd_configs",   "puppetdb_sd_configs",     "scaleway_sd_configs",
           "serverset_sd_configs",   "triton_sd_configs",     "uyuni_sd_configs",        "static_configs"};
    Json::Value httpSDConfig;
    httpSDConfig["url"] = ilogtail::prometheus::HTTP_PREFIX + ToString(getenv(ilogtail::prometheus::OPERATOR_HOST))
        + ":" + ToString(getenv(ilogtail::prometheus::OPERATOR_PORT)) + "/jobs/" + url_encode(mJobName)
        + "/targets?collector_id=" + ToString(getenv(ilogtail::prometheus::POD_NAME));
    httpSDConfig["follow_redirects"] = false;
    for (const auto& sdConfig : sdConfigs) {
        mScrapeConfig.removeMember(sdConfig);
    }
    mScrapeConfig["http_sd_configs"].append(httpSDConfig);
    for (const auto& relabelConfig : mScrapeConfig[ilogtail::prometheus::RELABEL_CONFIGS]) {
        mRelabelConfigs.push_back(RelabelConfig(relabelConfig));
    }

    return Validation();
}


// TODO: completely validate logic
bool ScrapeJob::Validation() const {
    return !mJobName.empty();
}

/// @brief Sort by name
bool ScrapeJob::operator<(const ScrapeJob& other) const {
    return mJobName < other.mJobName;
}

void ScrapeJob::StartTargetsDiscoverLoop() {
    mFinished.store(false);
    if (!mTargetsDiscoveryLoopThread) {
        mTargetsDiscoveryLoopThread = CreateThread([this]() { TargetsDiscoveryLoop(); });
    }
}

void ScrapeJob::StopTargetsDiscoverLoop() {
    mFinished.store(true);
    mTargetsDiscoveryLoopThread->Wait(0ULL);
    mTargetsDiscoveryLoopThread.reset();
    mScrapeTargetsMap.clear();
}

unordered_map<string, ScrapeTarget> ScrapeJob::GetScrapeTargetsMapCopy() {
    lock_guard<mutex> lock(mMutex);

    unordered_map<string, ScrapeTarget> copy;

    for (const auto& [targetHash, targetPtr] : mScrapeTargetsMap) {
        copy.emplace(targetHash, *targetPtr);
    }

    return copy;
}

void ScrapeJob::TargetsDiscoveryLoop() {
    uint64_t nextTargetsDiscoveryLoopTime = GetCurrentTimeInNanoSeconds();
    while (!mFinished.load()) {
        uint64_t timeNow = GetCurrentTimeInNanoSeconds();
        if (timeNow < nextTargetsDiscoveryLoopTime) {
            this_thread::sleep_for(chrono::nanoseconds(nextTargetsDiscoveryLoopTime - timeNow));
        }
        nextTargetsDiscoveryLoopTime
            = GetCurrentTimeInNanoSeconds() + sRefeshIntervalSeconds * 1000ULL * 1000ULL * 1000ULL;

        string readBuffer;
        if (!FetchHttpData(readBuffer)) {
            continue;
        }
        unordered_map<string, unique_ptr<ScrapeTarget>> newScrapeTargetsMap;
        if (!ParseTargetGroups(readBuffer, newScrapeTargetsMap)) {
            continue;
        }
        mScrapeTargetsMap = std::move(newScrapeTargetsMap);
    }
}

bool ScrapeJob::FetchHttpData(string& readBuffer) const {
    map<string, string> httpHeader;
    httpHeader[ilogtail::prometheus::ACCEPT_HEADER] = "application/json";
    httpHeader[ilogtail::prometheus::PROMETHEUS_REFRESH_HEADER] = ToString(sRefeshIntervalSeconds);
    httpHeader[ilogtail::prometheus::USER_AGENT_HEADER]
        = "matrix_prometheus_" + ToString(getenv(ilogtail::prometheus::POD_NAME));
    sdk::HttpMessage httpResponse;
    // TODO: 等待框架删除对respond返回头的 X_LOG_REQUEST_ID 检查
    httpResponse.header[sdk::X_LOG_REQUEST_ID] = "PrometheusTargetsDiscover";

    bool httpsFlag = mScheme == ilogtail::prometheus::HTTPS_SCHEME;
    try {
        mClient->Send(sdk::HTTP_GET,
                      mOperatorHost,
                      mOperatorPort,
                      "/jobs/" + url_encode(mJobName) + "/targets",
                      "collector_id=" + mPodName,
                      httpHeader,
                      "",
                      sRefeshIntervalSeconds,
                      httpResponse,
                      "",
                      httpsFlag);
    } catch (const sdk::LOGException& e) {
        LOG_WARNING(sLogger,
                    ("http service discovery from operator failed",
                     e.GetMessage())("errCode", e.GetErrorCode())(ilogtail::prometheus::JOB, mJobName));
        return false;
    }
    readBuffer = httpResponse.content;
    return true;
}

bool ScrapeJob::ParseTargetGroups(const string& response,
                                  unordered_map<string, unique_ptr<ScrapeTarget>>& newScrapeTargetsMap) const {
    Json::CharReaderBuilder readerBuilder;
    JSONCPP_STRING errs;
    Json::Value root;
    istringstream s(response);
    if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
        LOG_ERROR(sLogger,
                  ("http service discovery from operator failed",
                   "Failed to parse JSON: " + errs)(ilogtail::prometheus::JOB, mJobName));
        return false;
    }
    for (const auto& element : root) {
        if (!element.isObject()) {
            LOG_ERROR(sLogger,
                      ("http service discovery from operator failed",
                       "Invalid target group item found")(ilogtail::prometheus::JOB, mJobName));
            return false;
        }

        // Parse targets
        vector<string> targets;
        if (element.isMember(ilogtail::prometheus::TARGETS) && element[ilogtail::prometheus::TARGETS].isArray()) {
            for (const auto& target : element[ilogtail::prometheus::TARGETS]) {
                if (target.isString()) {
                    targets.push_back(target.asString());
                } else {
                    LOG_ERROR(sLogger,
                              ("http service discovery from operator failed",
                               "Invalid target item found")(ilogtail::prometheus::JOB, mJobName));
                    return false;
                }
            }
        }

        // Parse labels
        Labels labels;
        labels.Push(Label{ilogtail::prometheus::JOB, mJobName});
        labels.Push(Label{ilogtail::prometheus::__ADDRESS__, targets[0]});
        labels.Push(Label{ilogtail::prometheus::__SCHEME__, mScheme});
        labels.Push(Label{ilogtail::prometheus::__METRICS_PATH__, mMetricsPath});
        labels.Push(Label{ilogtail::prometheus::__SCRAPE_INTERVAL__, mScrapeIntervalString});
        labels.Push(Label{ilogtail::prometheus::__SCRAPE_TIMEOUT__, mScrapeTimeoutString});
        for (const auto& pair : mParams) {
            labels.Push(Label{ilogtail::prometheus::__PARAM_ + pair.first, pair.second[0]});
        }

        if (element.isMember(ilogtail::prometheus::LABELS) && element[ilogtail::prometheus::LABELS].isObject()) {
            for (const auto& labelKey : element[ilogtail::prometheus::LABELS].getMemberNames()) {
                labels.Push(Label{labelKey, element[ilogtail::prometheus::LABELS][labelKey].asString()});
            }
        }

        // Relabel Config
        Labels result;
        bool keep = relabel::Process(labels, mRelabelConfigs, result);
        if (!keep) {
            continue;
        }
        result.RemoveMetaLabels();
        if (result.Size() == 0) {
            continue;
        }
        LOG_INFO(sLogger, ("target relabel keep", mJobName));

        ScrapeTarget st = ScrapeTarget(mJobName,
                                       mMetricsPath,
                                       mScheme,
                                       ConvertMapParamsToQueryString(),
                                       GetIntSeconds(mScrapeIntervalString),
                                       GetIntSeconds(mScrapeTimeoutString),
                                       mHeaders);
        if (!st.SetLabels(result)) {
            continue;
        }
        st.SetPipelineInfo(mQueueKey, mInputIndex);

        newScrapeTargetsMap[st.GetHash()] = make_unique<ScrapeTarget>(st);
    }
    return true;
}

// only support xxs, xxm
int ScrapeJob::GetIntSeconds(const string& str) const {
    int value = stoi(str.substr(0, str.size() - 1));
    if (str.back() == 's') {
        return value;
    } else if (str.back() == 'm') {
        return 60 * value;
    }
    return 30;
}

string ScrapeJob::ConvertMapParamsToQueryString() const {
    stringstream ss;
    for (auto it = mParams.begin(); it != mParams.end(); ++it) {
        const auto& key = it->first;
        const auto& values = it->second;
        for (const auto& value : values) {
            if (ss.tellp() != 0) {
                ss << "&";
            }
            ss << key << "=" << value;
        }
    }
    return ss.str();
}

} // namespace logtail
